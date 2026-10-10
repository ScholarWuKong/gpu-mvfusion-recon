#pragma once

#include "feature/feature_extractor.h"
#include "scene/feature.h"
#include "scene/image.h"

#include <opencv2/core.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/imgproc.hpp>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

namespace sfm::feature::detail
{

inline cv::Mat makeGrayMat(const scene::Image& image)
{
    if (!image.isValid())
    {
        throw std::invalid_argument("Feature extraction received an invalid image.");
    }

    const int imageType = image.pixelFormat_ == scene::PixelFormat::GRAY8
                              ? CV_8UC1
                              : CV_8UC3;
    cv::Mat source(
        image.height_,
        image.width_,
        imageType,
        const_cast<std::uint8_t*>(image.pixels_.data()),
        image.rowStrideBytes_);

    if (image.pixelFormat_ == scene::PixelFormat::GRAY8)
    {
        return source;
    }

    cv::Mat grayImage;
    cv::cvtColor(source, grayImage, cv::COLOR_BGR2GRAY);
    return grayImage;
}

inline scene::FeatureSet makeFeatureSet(
    const std::uint32_t imageId,
    const std::vector<cv::KeyPoint>& keypoints,
    const cv::Mat& descriptors,
    const int expectedDescriptorSize)
{
    scene::FeatureSet featureSet;
    featureSet.imageId_ = imageId;
    featureSet.descriptorSize_ = static_cast<std::size_t>(expectedDescriptorSize);

    if (keypoints.empty())
    {
        if (!descriptors.empty())
        {
            throw std::runtime_error(
                "ORB returned descriptors without corresponding keypoints.");
        }
        return featureSet;
    }

    if (descriptors.empty() || descriptors.type() != CV_8UC1 ||
        descriptors.rows != static_cast<int>(keypoints.size()) ||
        descriptors.cols != expectedDescriptorSize)
    {
        throw std::runtime_error(
            "ORB returned incompatible keypoint and descriptor data.");
    }

    featureSet.features_.reserve(keypoints.size());
    for (const cv::KeyPoint& cvKeypoint : keypoints)
    {
        scene::Feature feature;
        feature.imageId_ = imageId;
        feature.keypoint_.x_ = cvKeypoint.pt.x;
        feature.keypoint_.y_ = cvKeypoint.pt.y;
        feature.keypoint_.size_ = cvKeypoint.size;
        feature.keypoint_.angle_ = cvKeypoint.angle;
        feature.keypoint_.response_ = cvKeypoint.response;
        feature.keypoint_.octave_ = cvKeypoint.octave;
        feature.keypoint_.classId_ = cvKeypoint.class_id;
        featureSet.features_.push_back(feature);
    }

    cv::Mat continuousDescriptors = descriptors;
    if (!continuousDescriptors.isContinuous())
    {
        continuousDescriptors = continuousDescriptors.clone();
    }

    const auto* descriptorData = continuousDescriptors.ptr<std::uint8_t>();
    const std::size_t descriptorBytes = continuousDescriptors.total() *
                                        continuousDescriptors.elemSize();
    featureSet.descriptors_.assign(
        descriptorData,
        descriptorData + descriptorBytes);

    if (!featureSet.hasValidDescriptors())
    {
        throw std::runtime_error("FeatureSet descriptor validation failed.");
    }

    return featureSet;
}

inline cv::ORB::ScoreType getCvORBScoreType(const ORBScoreType scoreType)
{
    switch (scoreType)
    {
    case ORBScoreType::HARRIS:
        return cv::ORB::HARRIS_SCORE;
    case ORBScoreType::FAST:
        return cv::ORB::FAST_SCORE;
    default:
        throw std::invalid_argument("Unsupported ORB score type.");
    }
}

} // namespace sfm::feature::detail
