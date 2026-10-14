#include "matching/feature_matcher.h"
#include <cmath>
#include <set>
#include <stdexcept>

namespace sfm::matching
{
void FeatureMatcherConfig::validate() const
{
    if (k_ != 2 || !std::isfinite(ratioThreshold_) || ratioThreshold_ <= 0.0f ||
        ratioThreshold_ >= 1.0f)
    {
        throw std::invalid_argument("ORB matching requires k=2 and ratio in (0, 1).");
    }
}

scene::MatchGraph IFeatureMatcher::matchAllPairs(const std::vector<scene::FeatureSet>& featureSets)
{
    std::set<ImageId> ids;
    std::vector<scene::ImagePair> pairs;
    for (const auto& features : featureSets)
    {
        if (!ids.insert(features.imageId_).second)
        {
            throw std::invalid_argument("Duplicate image ID in feature sets.");
        }
    }
    for (std::size_t first = 0; first < featureSets.size(); ++first)
    {
        for (std::size_t second = first + 1; second < featureSets.size(); ++second)
        {
            pairs.emplace_back(featureSets[first].imageId_, featureSets[second].imageId_);
        }
    }
    return matchPairs(featureSets, pairs);
}
}
