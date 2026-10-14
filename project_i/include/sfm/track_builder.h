#pragma once

#include "scene/feature.h"
#include "scene/match.h"
#include "scene/reconstruction.h"

namespace sfm
{
class TrackBuilder
{
public:
    // Extend tracks with new verified edges; retain existing point associations.
    std::size_t extend(const scene::MatchGraph& graph,
                       const std::vector<scene::FeatureSet>& featureSets,
                       scene::Reconstruction& reconstruction) const;
};
}
