#include "feature/gpu_feature_extractor.h"

#include "feature_extractor_internal.h"
#include "feature_selection.h"

#include <opencv2/core/cuda.hpp>

#include <stdexcept>
#include <vector>

namespace sfm::feature
{

GpuFeatureExtractor::GpuFeatureExtractor(const FeatureExtractorConfig& config) : config_(config)
{
    config.validate();

    const int deviceCount = cv::cuda::getCudaEnabledDeviceCount();
    if (deviceCount <= 0)
    {
        throw std::runtime_error(
            "OpenCV CUDA is unavailable or no CUDA-enabled GPU was detected.");
    }

    const int scoreType = static_cast<int>(detail::getCvORBScoreType(config.scoreType_));
    orb_ = cv::cuda::ORB::create(
        config.gpuGridColumns_ > 0 ? 2 * config.nFeatures_ : config.nFeatures_,
        config.scaleFactor_,
        config.nLevels_,
        config.edgeThreshold_,
        config.firstLevel_,
        config.wtaK_,
        scoreType,
        config.patchSize_,
        config.fastThreshold_,
        config.gpuBlurForDescriptor_);

    if (orb_.empty())
    {
        throw std::runtime_error("Failed to create the CUDA ORB extractor.");
    }
}

scene::FeatureSet GpuFeatureExtractor::extract(const scene::Image& image)
{
    cv::Mat grayImage = detail::makeGrayMat(image);

    cv::cuda::GpuMat gpuImage;
    cv::cuda::GpuMat gpuKeypoints;
    cv::cuda::GpuMat gpuDescriptors;

    gpuImage.upload(grayImage, stream_);
    orb_->detectAndComputeAsync(
        gpuImage,
        cv::noArray(),
        gpuKeypoints,
        gpuDescriptors,
        false,
        stream_);
    stream_.waitForCompletion();

    std::vector<cv::KeyPoint> keypoints;
    orb_->convert(gpuKeypoints, keypoints);

    cv::Mat descriptors;
    if (!gpuDescriptors.empty())
    {
        gpuDescriptors.download(descriptors, stream_);
        stream_.waitForCompletion();
    }

    auto features = detail::makeFeatureSet(
        image.id_,
        keypoints,
        descriptors,
        orb_->descriptorSize());
    if (config_.gpuGridColumns_ > 0)
    {
        return detail::distributeFeatures(std::move(features), image.width_, image.height_,
            static_cast<std::size_t>(config_.nFeatures_), config_.gpuGridColumns_, config_.gpuGridRows_);
    }
    return features;
}

} // namespace sfm::feature
