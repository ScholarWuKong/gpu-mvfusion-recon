#include "matching/feature_matcher.h"
#include "matching/gpu_feature_matcher.h"

namespace sfm::matching
{
std::unique_ptr<IFeatureMatcher> createFeatureMatcher(const FeatureMatcherConfig& config)
{
    return std::make_unique<GpuFeatureMatcher>(config);
}

std::unique_ptr<IFeatureMatcher> CreateFeatureMatcher(const FeatureMatcherConfig& config)
{
    return createFeatureMatcher(config);
}
}
