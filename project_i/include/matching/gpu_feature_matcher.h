#pragma once

#include "matching/feature_matcher.h"

namespace sfm::matching
{
class GpuFeatureMatcher final : public IFeatureMatcher
{
public:
    explicit GpuFeatureMatcher(const FeatureMatcherConfig& config);
    ~GpuFeatureMatcher() override;
    GpuFeatureMatcher(const GpuFeatureMatcher&) = delete;
    GpuFeatureMatcher& operator=(const GpuFeatureMatcher&) = delete;

    scene::MatchGraph matchPairs(const std::vector<scene::FeatureSet>& featureSets,
                                const std::vector<scene::ImagePair>& pairs) override;
    void clearCache() override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
