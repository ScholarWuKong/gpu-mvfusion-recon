#pragma once
#include "geometry/types.h"
#include <vector>

namespace sfm::geometry
{
struct TriangulationView
{
    CameraIntrinsics camera_;
    CameraPose pose_;
    PixelPoint pixel_;
};

bool projectPoint(const Vector3& point, const CameraIntrinsics& camera,
                  const CameraPose& pose, PixelPoint& pixel);
double computeReprojectionError(const Vector3& point, const TriangulationView& view);
bool triangulatePoint(const std::vector<TriangulationView>& views, Vector3& point,
                      double maxReprojectionError, double minAngleDegrees);

// Attempt all-view DLT first, then select a consistent subset from deterministic
// two-view hypotheses. Returned indices always refer to the original views.
bool triangulatePointRobust(const std::vector<TriangulationView>& views, Vector3& point,
                            std::vector<std::size_t>& inlierViewIndices,
                            double maxReprojectionError, double minAngleDegrees);
}
