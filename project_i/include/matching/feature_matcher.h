#pragma once

#include "scene/feature.h"
#include "scene/match.h"
#include <memory>
#include <vector>

namespace sfm::matching
{
struct FeatureMatcherConfig
{
    int k_ = 2;
    float ratioThreshold_ = 0.8f;
    void validate() const;
};

// Callers retain feature ownership. This interface is not thread-safe.
class IFeatureMatcher
{
public:
    virtual ~IFeatureMatcher() = default;
    virtual scene::MatchGraph matchPairs(const std::vector<scene::FeatureSet>& featureSets,
                                        const std::vector<scene::ImagePair>& pairs) = 0;
    scene::MatchGraph matchAllPairs(const std::vector<scene::FeatureSet>& featureSets);
    virtual void clearCache() = 0;
};

std::unique_ptr<IFeatureMatcher> createFeatureMatcher(const FeatureMatcherConfig& config);
std::unique_ptr<IFeatureMatcher> CreateFeatureMatcher(const FeatureMatcherConfig& config);
}
