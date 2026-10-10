#include "feature/cpu_feature_extractor.h"

#include "feature_extractor_internal.h"

#include <opencv2/core.hpp>

namespace sfm::feature
{

CpuFeatureExtractor::CpuFeatureExtractor(const FeatureExtractorConfig& config)
{
    config.validate();

    orb_ = cv::ORB::create(
        config.nFeatures_,
        config.scaleFactor_,
        config.nLevels_,
        config.edgeThreshold_,
        config.firstLevel_,
        config.wtaK_,
        detail::getCvORBScoreType(config.scoreType_),
        config.patchSize_,
        config.fastThreshold_);

    if (orb_.empty())
    {
        throw cv::Exception(
            cv::Error::StsError,
            "Failed to create the CPU ORB extractor.",
            "CpuFeatureExtractor",
            __FILE__,
            __LINE__);
    }
}

scene::FeatureSet CpuFeatureExtractor::extract(const scene::Image& image)
{
    cv::Mat grayImage = detail::makeGrayMat(image);

    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    orb_->detectAndCompute(
        grayImage,
        cv::noArray(),
        keypoints,
        descriptors,
        false);

    return detail::makeFeatureSet(
        image.id_,
        keypoints,
        descriptors,
        orb_->descriptorSize());
}

} // namespace sfm::feature
