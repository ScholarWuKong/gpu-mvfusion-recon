#pragma once

#include "feature/feature_extractor.h"

#include <opencv2/core/cuda.hpp>
#include <opencv2/cudafeatures2d.hpp>

namespace sfm::feature
{

class GpuFeatureExtractor final : public IFeatureExtractor
{
public:
    explicit GpuFeatureExtractor(const FeatureExtractorConfig& config);

    GpuFeatureExtractor(const GpuFeatureExtractor&) = delete;
    GpuFeatureExtractor& operator=(const GpuFeatureExtractor&) = delete;

    scene::FeatureSet extract(const scene::Image& image) override;

private:
    cv::Ptr<cv::cuda::ORB> orb_;
    cv::cuda::Stream stream_;
};

} // namespace sfm::feature
