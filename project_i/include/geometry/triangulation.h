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
}
