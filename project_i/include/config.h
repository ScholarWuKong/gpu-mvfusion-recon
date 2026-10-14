#pragma once

#include "geometry/two_view_geometry.h"

namespace sfm
{
struct MapperConfig
{
    geometry::TwoViewConfig geometry_;
    double maxReprojectionError_ = 4.0;
    double minTriangulationAngle_ = 1.0;
    std::size_t minInitialPoints_ = 15;
    std::size_t minPnpInliers_ = 12;
    bool runBundleAdjustment_ = true;
    void validate() const;
};
}
