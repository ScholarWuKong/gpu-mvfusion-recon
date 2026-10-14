#include "optimizer_internal.h"
#include <opencv2/calib3d.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace sfm::ba::detail
{
namespace
{
struct IndexedObservation
{
    PointId pointId_;
    ImageId imageId_;
    int cameraIndex_;
    double x_;
    double y_;
};

struct PointSystem
{
    cv::Mat hessian_ = cv::Mat::zeros(3, 3, CV_64F);
    cv::Mat gradient_ = cv::Mat::zeros(3, 1, CV_64F);
    cv::Mat inverse_;
    std::map<int, cv::Mat> cross_;
};

std::vector<ResidualInput> makeInputs(const scene::Reconstruction& reconstruction,
                                      const std::vector<IndexedObservation>& observations)
{
    std::vector<ResidualInput> inputs;
    inputs.reserve(observations.size());
    for (const auto& observation : observations)
    {
        const auto& image = reconstruction.images_.at(observation.imageId_);
        ResidualInput input{};
        std::copy(image.pose_.rotation_.begin(), image.pose_.rotation_.end(), input.rotation_);
        std::copy(image.pose_.translation_.begin(), image.pose_.translation_.end(), input.translation_);
        const auto& position = reconstruction.points_.at(observation.pointId_).position_;
        std::copy(position.begin(), position.end(), input.point_);
        input.intrinsics_[0] = image.camera_.fx_;
        input.intrinsics_[1] = image.camera_.fy_;
        input.intrinsics_[2] = image.camera_.cx_;
        input.intrinsics_[3] = image.camera_.cy_;
        input.pixel_[0] = observation.x_;
        input.pixel_[1] = observation.y_;
        inputs.push_back(input);
    }
    return inputs;
}

double robustCost(const std::vector<ResidualBlock>& blocks, const double delta)
{
    double cost = 0.0;
    for (const auto& block : blocks)
    {
        if (!block.valid_)
        {
            return std::numeric_limits<double>::infinity();
        }
        const double norm = std::hypot(block.residual_[0], block.residual_[1]);
        if (!std::isfinite(norm))
        {
            return std::numeric_limits<double>::infinity();
        }
        cost += norm <= delta ? 0.5 * norm * norm : delta * (norm - 0.5 * delta);
    }
    return cost;
}

void applyCameraStep(scene::CameraPose& pose, const cv::Mat& step)
{
    cv::Mat incrementalRotation;
    cv::Rodrigues(step.rowRange(0, 3), incrementalRotation);
    const cv::Mat oldRotation(3, 3, CV_64F, pose.rotation_.data());
    const cv::Mat oldTranslation(3, 1, CV_64F, pose.translation_.data());
    const cv::Mat newRotation = incrementalRotation * oldRotation;
    const cv::Mat newTranslation = incrementalRotation * oldTranslation + step.rowRange(3, 6);
    for (int row = 0; row < 3; ++row)
    {
        pose.translation_[row] = newTranslation.at<double>(row);
        for (int column = 0; column < 3; ++column)
        {
            pose.rotation_[3 * row + column] = newRotation.at<double>(row, column);
        }
    }
}
}

BundleAdjustmentResult runOptimization(scene::Reconstruction& reconstruction,
                                       const BundleAdjusterConfig& config, ResidualEvaluator evaluator)
{
    config.validate();
    reconstruction.validate();
    BundleAdjustmentResult result;
    if (reconstruction.points_.empty())
    {
        return result;
    }
    const std::set<ImageId> fixed(reconstruction.fixedImageIds_.begin(), reconstruction.fixedImageIds_.end());
    if (fixed.size() < 2)
    {
        throw std::invalid_argument("BA requires two fixed seed cameras to constrain scale and pose.");
    }
    std::vector<ImageId> variableCameras;
    std::map<ImageId, int> cameraIndices;
    for (const auto& entry : reconstruction.images_)
    {
        if (entry.second.registered_ && fixed.count(entry.first) == 0)
        {
            cameraIndices[entry.first] = static_cast<int>(variableCameras.size());
            variableCameras.push_back(entry.first);
        }
    }
    // This initial implementation uses a dense camera Schur system, not a dense point system.
    // Bound memory explicitly rather than allocating an unbounded N-camera matrix.
    constexpr std::size_t MAX_VARIABLE_CAMERAS = 256;
    if (variableCameras.size() > MAX_VARIABLE_CAMERAS)
    {
        throw std::runtime_error("Dense BA currently supports at most 256 variable cameras.");
    }
    std::vector<IndexedObservation> observations;
    for (const auto& point : reconstruction.points_)
    {
        std::vector<IndexedObservation> pointObservations;
        for (const auto& observation : reconstruction.tracks_.at(point.trackId_).observations_)
        {
            if (!observation.enabled_ || !reconstruction.images_.at(observation.imageId_).registered_)
            {
                continue;
            }
            const auto camera = cameraIndices.find(observation.imageId_);
            pointObservations.push_back({point.id_, observation.imageId_,
                camera == cameraIndices.end() ? -1 : camera->second, observation.x_, observation.y_});
        }
        if (pointObservations.size() >= 2)
        {
            observations.insert(observations.end(), pointObservations.begin(), pointObservations.end());
        }
    }
    if (observations.empty())
    {
        return result;
    }
    result.attempted_ = true;
    result.observationCount_ = observations.size();
    std::vector<ResidualBlock> blocks;
    evaluator(makeInputs(reconstruction, observations), blocks);
    if (blocks.size() != observations.size())
    {
        throw std::runtime_error("Invalid residual evaluator output.");
    }
    result.initialCost_ = robustCost(blocks, config.huberDelta_);
    result.finalCost_ = result.initialCost_;
    if (!std::isfinite(result.initialCost_))
    {
        throw std::runtime_error("BA received a non-finite or behind-camera observation.");
    }
    double damping = config.initialDamping_;
    const int cameraDimension = static_cast<int>(6 * variableCameras.size());
    for (int iteration = 0; iteration < config.maxIterations_; ++iteration)
    {
        result.iterations_ = iteration + 1;
        if (result.finalCost_ < config.relativeTolerance_)
        {
            result.converged_ = true;
            break;
        }
        cv::Mat cameraHessian = cv::Mat::zeros(cameraDimension, cameraDimension, CV_64F);
        cv::Mat cameraGradient = cv::Mat::zeros(cameraDimension, 1, CV_64F);
        std::map<PointId, PointSystem> pointSystems;
        for (std::size_t index = 0; index < observations.size(); ++index)
        {
            const auto& observation = observations[index];
            auto& block = blocks[index];
            const double norm = std::hypot(block.residual_[0], block.residual_[1]);
            const double weight = norm <= config.huberDelta_ ? 1.0 : std::sqrt(config.huberDelta_ / norm);
            cv::Mat residual(2, 1, CV_64F, block.residual_);
            cv::Mat pointJacobian(2, 3, CV_64F, block.pointJacobian_);
            cv::Mat cameraJacobian(2, 6, CV_64F, block.cameraJacobian_);
            const cv::Mat weightedResidual = residual * weight;
            const cv::Mat weightedPoint = pointJacobian * weight;
            const cv::Mat weightedCamera = cameraJacobian * weight;
            auto& system = pointSystems[observation.pointId_];
            system.hessian_ += weightedPoint.t() * weightedPoint;
            system.gradient_ += weightedPoint.t() * weightedResidual;
            if (observation.cameraIndex_ >= 0)
            {
                const int offset = 6 * observation.cameraIndex_;
                cameraHessian(cv::Rect(offset, offset, 6, 6)) += weightedCamera.t() * weightedCamera;
                cameraGradient.rowRange(offset, offset + 6) += weightedCamera.t() * weightedResidual;
                auto& cross = system.cross_[observation.cameraIndex_];
                if (cross.empty())
                {
                    cross = cv::Mat::zeros(6, 3, CV_64F);
                }
                cross += weightedCamera.t() * weightedPoint;
            }
        }
        cv::Mat schur = cameraHessian.clone();
        cv::Mat right;
        if (cameraDimension > 0)
        {
            right = -cameraGradient;
        }
        for (int axis = 0; axis < cameraDimension; ++axis)
        {
            schur.at<double>(axis, axis) += damping * std::max(1.0, cameraHessian.at<double>(axis, axis));
        }
        for (auto& entry : pointSystems)
        {
            auto& system = entry.second;
            cv::Mat dampedPoint = system.hessian_.clone();
            for (int axis = 0; axis < 3; ++axis)
            {
                dampedPoint.at<double>(axis, axis) += damping * std::max(1.0, system.hessian_.at<double>(axis, axis));
            }
            if (!cv::invert(dampedPoint, system.inverse_, cv::DECOMP_CHOLESKY))
            {
                throw std::runtime_error("Could not invert damped BA point block.");
            }
            for (const auto& left : system.cross_)
            {
                const cv::Mat product = left.second * system.inverse_;
                right.rowRange(6 * left.first, 6 * left.first + 6) += product * system.gradient_;
                for (const auto& other : system.cross_)
                {
                    schur(cv::Rect(6 * other.first, 6 * left.first, 6, 6)) -= product * other.second.t();
                }
            }
        }
        cv::Mat cameraStep;
        if (cameraDimension > 0 && !cv::solve(schur, right, cameraStep, cv::DECOMP_CHOLESKY))
        {
            damping *= 10.0;
            continue;
        }
        auto trial = reconstruction;
        for (std::size_t index = 0; index < variableCameras.size(); ++index)
        {
            applyCameraStep(trial.images_.at(variableCameras[index]).pose_,
                             cameraStep.rowRange(static_cast<int>(6 * index), static_cast<int>(6 * index + 6)));
        }
        double stepSquared = cameraDimension > 0 ? cv::norm(cameraStep, cv::NORM_L2SQR) : 0.0;
        for (const auto& entry : pointSystems)
        {
            const auto& system = entry.second;
            cv::Mat pointRight = system.gradient_.clone();
            for (const auto& cross : system.cross_)
            {
                pointRight += cross.second.t() * cameraStep.rowRange(6 * cross.first, 6 * cross.first + 6);
            }
            const cv::Mat pointStep = -system.inverse_ * pointRight;
            stepSquared += cv::norm(pointStep, cv::NORM_L2SQR);
            for (int axis = 0; axis < 3; ++axis)
            {
                trial.points_.at(entry.first).position_[axis] += pointStep.at<double>(axis);
            }
        }
        std::vector<ResidualBlock> trialBlocks;
        evaluator(makeInputs(trial, observations), trialBlocks);
        if (trialBlocks.size() != observations.size())
        {
            throw std::runtime_error("Invalid residual evaluator output during line search.");
        }
        const double trialCost = robustCost(trialBlocks, config.huberDelta_);
        if (trialCost < result.finalCost_)
        {
            const double improvement = result.finalCost_ - trialCost;
            reconstruction = std::move(trial);
            blocks = std::move(trialBlocks);
            result.finalCost_ = trialCost;
            damping = std::max(1e-12, damping / 3.0);
            if (improvement <= config.relativeTolerance_ * std::max(1.0, result.finalCost_) || stepSquared < 1e-16)
            {
                result.converged_ = true;
                break;
            }
        }
        else
        {
            if (stepSquared < 1e-16)
            {
                result.converged_ = true;
                break;
            }
            damping *= 10.0;
        }
    }
    reconstruction.validate();
    return result;
}
}
