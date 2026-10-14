#include "geometry/types.h"
#include <cmath>

namespace sfm::geometry
{
bool CameraIntrinsics::isValid() const noexcept
{
    return std::isfinite(fx_) && std::isfinite(fy_) && std::isfinite(cx_) &&
           std::isfinite(cy_) && fx_ > 0.0 && fy_ > 0.0;
}
}
