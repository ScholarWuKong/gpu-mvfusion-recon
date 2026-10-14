#pragma once

#include "scene/reconstruction.h"
#include "config.h"

namespace sfm
{
class ObservationManager
{
public:
    std::size_t triangulateTracks(scene::Reconstruction& reconstruction,
                                  const MapperConfig& config) const;
    void filterObservations(scene::Reconstruction& reconstruction,
                             double maxReprojectionError) const;
};
}
