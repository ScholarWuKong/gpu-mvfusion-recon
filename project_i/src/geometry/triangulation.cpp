#include "geometry/triangulation.h"
#include <opencv2/core.hpp>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <numeric>

namespace sfm::geometry
{
bool projectPoint(const Vector3& point, const CameraIntrinsics& camera,
                  const CameraPose& pose, PixelPoint& pixel)
{
    Vector3 transformed = pose.translation_;
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            transformed[row] += pose.rotation_[3 * row + column] * point[column];
        }
    }
    if (!camera.isValid() || !std::isfinite(transformed[2]) || transformed[2] <= 1e-9)
    {
        return false;
    }
    pixel.x_ = camera.fx_ * transformed[0] / transformed[2] + camera.cx_;
    pixel.y_ = camera.fy_ * transformed[1] / transformed[2] + camera.cy_;
    return std::isfinite(pixel.x_) && std::isfinite(pixel.y_);
}

double computeReprojectionError(const Vector3& point, const TriangulationView& view)
{
    PixelPoint projected;
    if (!projectPoint(point, view.camera_, view.pose_, projected))
    {
        return std::numeric_limits<double>::infinity();
    }
    return std::hypot(projected.x_ - view.pixel_.x_, projected.y_ - view.pixel_.y_);
}

bool triangulatePoint(const std::vector<TriangulationView>& views, Vector3& point,
                      const double maxReprojectionError, const double minAngleDegrees)
{
    if (!std::isfinite(maxReprojectionError) || maxReprojectionError <= 0.0 ||
        !std::isfinite(minAngleDegrees) || minAngleDegrees < 0.0 || minAngleDegrees >= 180.0)
    {
        throw std::invalid_argument("Invalid triangulation thresholds.");
    }
    if (views.size() < 2)
    {
        return false;
    }
    cv::Mat system(static_cast<int>(views.size() * 2), 4, CV_64F);
    for (std::size_t index = 0; index < views.size(); ++index)
    {
        const auto& view = views[index];
        if (!view.camera_.isValid() || !std::isfinite(view.pixel_.x_) || !std::isfinite(view.pixel_.y_))
        {
            throw std::invalid_argument("Invalid triangulation view.");
        }
        const double x = (view.pixel_.x_ - view.camera_.cx_) / view.camera_.fx_;
        const double y = (view.pixel_.y_ - view.camera_.cy_) / view.camera_.fy_;
        for (int column = 0; column < 4; ++column)
        {
            const double last = column == 3 ? view.pose_.translation_[2] : view.pose_.rotation_[6 + column];
            const double first = column == 3 ? view.pose_.translation_[0] : view.pose_.rotation_[column];
            const double second = column == 3 ? view.pose_.translation_[1] : view.pose_.rotation_[3 + column];
            system.at<double>(static_cast<int>(2 * index), column) = x * last - first;
            system.at<double>(static_cast<int>(2 * index + 1), column) = y * last - second;
        }
    }
    cv::Mat homogeneous;
    cv::SVD::solveZ(system, homogeneous);
    const double scale = homogeneous.at<double>(3);
    if (!std::isfinite(scale) || std::abs(scale) < 1e-12)
    {
        return false;
    }
    for (int axis = 0; axis < 3; ++axis)
    {
        point[axis] = homogeneous.at<double>(axis) / scale;
    }
    std::vector<Vector3> rays;
    for (const auto& view : views)
    {
        if (computeReprojectionError(point, view) > maxReprojectionError)
        {
            return false;
        }
        Vector3 ray = point;
        for (int axis = 0; axis < 3; ++axis)
        {
            for (int row = 0; row < 3; ++row)
            {
                ray[axis] += view.pose_.rotation_[3 * row + axis] * view.pose_.translation_[row];
            }
        }
        const double length = std::sqrt(ray[0] * ray[0] + ray[1] * ray[1] + ray[2] * ray[2]);
        if (!std::isfinite(length) || length < 1e-12)
        {
            return false;
        }
        for (auto& value : ray)
        {
            value /= length;
        }
        rays.push_back(ray);
    }
    double maxAngle = 0.0;
    for (std::size_t first = 0; first < rays.size(); ++first)
    {
        for (std::size_t second = first + 1; second < rays.size(); ++second)
        {
            const double dot = rays[first][0] * rays[second][0] + rays[first][1] * rays[second][1] +
                               rays[first][2] * rays[second][2];
            maxAngle = std::max(maxAngle, std::acos(std::clamp(dot, -1.0, 1.0)) * 180.0 / 3.141592653589793);
        }
    }
    return maxAngle >= minAngleDegrees;
}

bool triangulatePointRobust(const std::vector<TriangulationView>& views, Vector3& point,
                            std::vector<std::size_t>& inlierViewIndices,
                            const double maxReprojectionError, const double minAngleDegrees)
{
    inlierViewIndices.clear();
    if (triangulatePoint(views, point, maxReprojectionError, minAngleDegrees))
    {
        inlierViewIndices.resize(views.size());
        std::iota(inlierViewIndices.begin(), inlierViewIndices.end(), std::size_t{0});
        return true;
    }
    if (views.size() < 3)
    {
        return false;
    }
    struct HypothesisPair
    {
        std::size_t first_;
        std::size_t second_;
        double angle_;
    };
    std::vector<Vector3> rays;
    for (const auto& view : views)
    {
        const Vector3 cameraRay{(view.pixel_.x_ - view.camera_.cx_) / view.camera_.fx_,
                                (view.pixel_.y_ - view.camera_.cy_) / view.camera_.fy_, 1.0};
        Vector3 ray{};
        for (int axis = 0; axis < 3; ++axis)
        {
            for (int row = 0; row < 3; ++row)
            {
                ray[axis] += view.pose_.rotation_[3 * row + axis] * cameraRay[row];
            }
        }
        const double length = std::sqrt(ray[0] * ray[0] + ray[1] * ray[1] + ray[2] * ray[2]);
        for (auto& value : ray)
        {
            value /= length;
        }
        rays.push_back(ray);
    }
    std::vector<HypothesisPair> pairs;
    for (std::size_t first = 0; first < views.size(); ++first)
    {
        for (std::size_t second = first + 1; second < views.size(); ++second)
        {
            const double dot = rays[first][0] * rays[second][0] + rays[first][1] * rays[second][1] +
                               rays[first][2] * rays[second][2];
            pairs.push_back({first, second, std::acos(std::clamp(dot, -1.0, 1.0))});
        }
    }
    std::stable_sort(pairs.begin(), pairs.end(), [](const auto& first, const auto& second)
    {
        return first.angle_ > second.angle_;
    });
    constexpr std::size_t MAX_HYPOTHESES = 64;
    const auto minimumInliers = std::max<std::size_t>(2, (3 * views.size() + 4) / 5);
    double bestSquaredError = std::numeric_limits<double>::infinity();
    for (std::size_t trial = 0; trial < std::min(MAX_HYPOTHESES, pairs.size()); ++trial)
    {
        const auto& pair = pairs[trial];
        Vector3 candidate;
        if (!triangulatePoint({views[pair.first_], views[pair.second_]}, candidate,
                               maxReprojectionError, minAngleDegrees))
        {
            continue;
        }
        std::vector<std::size_t> inliers;
        double squaredError = 0.0;
        for (std::size_t index = 0; index < views.size(); ++index)
        {
            const double error = computeReprojectionError(candidate, views[index]);
            if (error <= maxReprojectionError)
            {
                inliers.push_back(index);
                squaredError += error * error;
            }
        }
        if (inliers.size() >= minimumInliers && (inliers.size() > inlierViewIndices.size() ||
            (inliers.size() == inlierViewIndices.size() && squaredError < bestSquaredError)))
        {
            point = candidate;
            inlierViewIndices = std::move(inliers);
            bestSquaredError = squaredError;
        }
    }
    if (inlierViewIndices.empty())
    {
        return false;
    }
    std::vector<TriangulationView> consensus;
    for (const auto index : inlierViewIndices)
    {
        consensus.push_back(views[index]);
    }
    Vector3 refined;
    if (triangulatePoint(consensus, refined, maxReprojectionError, minAngleDegrees))
    {
        double refinedError = 0.0;
        for (const auto& view : consensus)
        {
            const double error = computeReprojectionError(refined, view);
            refinedError += error * error;
        }
        if (refinedError <= bestSquaredError)
        {
            point = refined;
        }
    }
    return true;
}
}
