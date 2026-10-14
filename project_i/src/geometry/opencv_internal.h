#pragma once
#include "geometry/types.h"
#include <opencv2/core.hpp>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace sfm::geometry::detail
{
inline cv::Mat matrix(const Matrix3& values)
{
    return cv::Mat(3, 3, CV_64F, const_cast<double*>(values.data())).clone();
}

inline Matrix3 values(const cv::Mat& matrix)
{
    cv::Mat converted;
    matrix.convertTo(converted, CV_64F);
    Matrix3 result;
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 3; ++column)
        {
            result[3 * row + column] = converted.at<double>(row, column);
        }
    }
    return result;
}

inline cv::Mat cameraMatrix(const CameraIntrinsics& camera)
{
    if (!camera.isValid())
    {
        throw std::invalid_argument("Invalid pinhole camera intrinsics.");
    }
    return (cv::Mat_<double>(3, 3) << camera.fx_, 0.0, camera.cx_,
            0.0, camera.fy_, camera.cy_, 0.0, 0.0, 1.0);
}

inline std::vector<cv::Point2d> pixels(const std::vector<PixelPoint>& points,
                                      const CameraIntrinsics* camera = nullptr)
{
    if (camera != nullptr && !camera->isValid())
    {
        throw std::invalid_argument("Invalid camera intrinsics.");
    }
    std::vector<cv::Point2d> result;
    result.reserve(points.size());
    for (const auto& point : points)
    {
        if (!std::isfinite(point.x_) || !std::isfinite(point.y_))
        {
            throw std::invalid_argument("Non-finite pixel coordinate.");
        }
        result.emplace_back(camera == nullptr ? point.x_ : (point.x_ - camera->cx_) / camera->fx_,
                            camera == nullptr ? point.y_ : (point.y_ - camera->cy_) / camera->fy_);
    }
    return result;
}

inline void validatePairs(const std::vector<PixelPoint>& first,
                           const std::vector<PixelPoint>& second,
                           const double threshold, const double confidence)
{
    if (first.size() != second.size() || !std::isfinite(threshold) || threshold <= 0.0 ||
        !std::isfinite(confidence) || confidence <= 0.0 || confidence >= 1.0)
    {
        throw std::invalid_argument("Invalid RANSAC inputs.");
    }
}

inline std::vector<std::size_t> inliers(const cv::Mat& mask)
{
    std::vector<std::size_t> result;
    for (std::size_t index = 0; index < mask.total(); ++index)
    {
        if (mask.ptr<unsigned char>()[index] != 0)
        {
            result.push_back(index);
        }
    }
    return result;
}
}
