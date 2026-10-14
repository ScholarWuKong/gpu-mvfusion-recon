#pragma once
#include "geometry/fundamental.h"

namespace sfm::geometry
{
MatrixEstimate estimateEssential(const std::vector<PixelPoint>& first,
                                 const std::vector<PixelPoint>& second,
                                 const CameraIntrinsics& firstCamera,
                                 const CameraIntrinsics& secondCamera,
                                 double threshold, double confidence);
}
