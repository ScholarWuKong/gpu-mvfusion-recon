#pragma once
#include "geometry/types.h"
#include <vector>

namespace sfm::geometry
{
struct PoseEstimate
{
    bool valid_ = false;
    CameraPose pose_;
    std::vector<std::size_t> inlierIndices_;
};

PoseEstimate recoverRelativePose(const Matrix3& essential,
                                 const std::vector<PixelPoint>& first,
                                 const std::vector<PixelPoint>& second,
                                 const CameraIntrinsics& firstCamera,
                                 const CameraIntrinsics& secondCamera,
                                 const std::vector<std::size_t>& essentialInliers);
PoseEstimate estimateAbsolutePose(const std::vector<Vector3>& points,
                                  const std::vector<PixelPoint>& pixels,
                                  const CameraIntrinsics& camera, double threshold);
}
