#pragma once

#include "util/types.h"

namespace sfm::scene
{
struct Point3D
{
    PointId id_ = 0;
    TrackId trackId_ = 0;
    Vector3 position_ = {0.0, 0.0, 0.0};
};
}
