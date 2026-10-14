#include "geometry/pose_estimation.h"
#include "geometry/triangulation.h"
#include "opencv_internal.h"
#include <opencv2/calib3d.hpp>

namespace sfm::geometry
{
PoseEstimate recoverRelativePose(const Matrix3& essential,
                                 const std::vector<PixelPoint>& first,
                                 const std::vector<PixelPoint>& second,
                                 const CameraIntrinsics& firstCamera,
                                 const CameraIntrinsics& secondCamera,
                                 const std::vector<std::size_t>& essentialInliers)
{
    if (first.size() != second.size())
    {
        throw std::invalid_argument("Mismatched relative-pose correspondence counts.");
    }
    PoseEstimate result;
    if (essentialInliers.size() < 5)
    {
        return result;
    }
    cv::Mat mask = cv::Mat::zeros(static_cast<int>(first.size()), 1, CV_8U);
    for (const auto index : essentialInliers)
    {
        if (index >= first.size())
        {
            throw std::out_of_range("Invalid essential-matrix inlier index.");
        }
        mask.at<unsigned char>(static_cast<int>(index)) = 1;
    }
    cv::Mat rotation;
    cv::Mat translation;
    const int count = cv::recoverPose(detail::matrix(essential), detail::pixels(first, &firstCamera),
                                      detail::pixels(second, &secondCamera), rotation, translation,
                                      1.0, cv::Point2d(0.0, 0.0), mask);
    if (count >= 5)
    {
        result.valid_ = true;
        result.pose_.rotation_ = detail::values(rotation);
        for (int axis = 0; axis < 3; ++axis)
        {
            result.pose_.translation_[axis] = translation.at<double>(axis);
        }
        result.inlierIndices_ = detail::inliers(mask);
    }
    return result;
}

PoseEstimate estimateAbsolutePose(const std::vector<Vector3>& points,
                                  const std::vector<PixelPoint>& pixels,
                                  const CameraIntrinsics& camera, const double threshold)
{
    if (points.size() != pixels.size() || !std::isfinite(threshold) || threshold <= 0.0)
    {
        throw std::invalid_argument("Invalid PnP inputs.");
    }
    PoseEstimate result;
    if (points.size() < 6)
    {
        return result;
    }
    std::vector<cv::Point3d> objectPoints;
    for (const auto& point : points)
    {
        for (const auto value : point)
        {
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Non-finite PnP world point.");
            }
        }
        objectPoints.emplace_back(point[0], point[1], point[2]);
    }
    const auto imagePoints = detail::pixels(pixels);
    const auto intrinsics = detail::cameraMatrix(camera);
    cv::Mat rotationVector;
    cv::Mat translation;
    cv::Mat inliers;
    if (!cv::solvePnPRansac(objectPoints, imagePoints, intrinsics, cv::noArray(),
                            rotationVector, translation, false, 200,
                            static_cast<float>(threshold), 0.999, inliers, cv::SOLVEPNP_EPNP) ||
        inliers.total() < 6)
    {
        return result;
    }
    std::vector<cv::Point3d> refinedObjects;
    std::vector<cv::Point2d> refinedPixels;
    for (std::size_t index = 0; index < inliers.total(); ++index)
    {
        const auto source = static_cast<std::size_t>(inliers.at<int>(static_cast<int>(index)));
        refinedObjects.push_back(objectPoints.at(source));
        refinedPixels.push_back(imagePoints.at(source));
        result.inlierIndices_.push_back(source);
    }
    cv::solvePnPRefineLM(refinedObjects, refinedPixels, intrinsics, cv::noArray(),
                         rotationVector, translation);
    cv::Mat rotation;
    cv::Rodrigues(rotationVector, rotation);
    result.pose_.rotation_ = detail::values(rotation);
    for (int axis = 0; axis < 3; ++axis)
    {
        result.pose_.translation_[axis] = translation.at<double>(axis);
    }
    const auto ransacInliers = result.inlierIndices_;
    result.inlierIndices_.clear();
    for (const auto index : ransacInliers)
    {
        if (computeReprojectionError(points[index], {camera, result.pose_, pixels[index]}) <= threshold)
        {
            result.inlierIndices_.push_back(index);
        }
    }
    result.valid_ = result.inlierIndices_.size() >= 6;
    return result;
}
}
