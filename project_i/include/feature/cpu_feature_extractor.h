#pragma once

#include "feature/feature_extractor.h"

#include <opencv2/features2d.hpp>

namespace sfm::feature
{

class CpuFeatureExtractor final : public IFeatureExtractor
{
public:
    explicit CpuFeatureExtractor(const FeatureExtractorConfig& config);

    scene::FeatureSet extract(const scene::Image& image) override;

private:
    cv::Ptr<cv::ORB> orb_;
};

} // namespace sfm::feature
