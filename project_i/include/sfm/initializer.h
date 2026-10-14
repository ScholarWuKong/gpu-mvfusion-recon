#pragma once

#include "config.h"
#include "scene/match.h"
#include "scene/reconstruction.h"

namespace sfm
{
class Initializer
{
public:
    bool initialize(const scene::MatchGraph& graph, scene::Reconstruction& reconstruction,
                     const MapperConfig& config) const;
};
}
