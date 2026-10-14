#pragma once

#include "util/types.h"

namespace sfm::geometry
{
struct CameraIntrinsics
{
    double fx_ = 0.0;
    double fy_ = 0.0;
    double cx_ = 0.0;
    double cy_ = 0.0;

    bool isValid() const noexcept;
};

// World-to-camera convention: Xc = R * Xw + t. Matrices are row-major.
struct CameraPose
{
    Matrix3 rotation_ = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    Vector3 translation_ = {0.0, 0.0, 0.0};
};

struct PixelPoint
{
    double x_ = 0.0;
    double y_ = 0.0;
};
}
