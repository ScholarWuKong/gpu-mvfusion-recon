#pragma once

#include "scene/feature.h"

namespace sfm::feature::detail
{
scene::FeatureSet distributeFeatures(scene::FeatureSet candidates, int width, int height,
                                     std::size_t maximumFeatures, int columns, int rows);
}
