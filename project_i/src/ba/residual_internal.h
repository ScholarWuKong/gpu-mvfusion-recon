#pragma once
#include "optimizer_internal.h"
#include <cmath>

#ifdef __CUDACC__
#define SFM_HOST_DEVICE __host__ __device__
#else
#define SFM_HOST_DEVICE
#endif

namespace sfm::ba::detail
{
SFM_HOST_DEVICE inline ResidualBlock computeResidual(const ResidualInput& input)
{
    ResidualBlock block;
    double cameraPoint[3] = {};
    for (int row = 0; row < 3; ++row)
    {
        cameraPoint[row] = input.translation_[row];
        for (int column = 0; column < 3; ++column)
        {
            cameraPoint[row] += input.rotation_[row * 3 + column] * input.point_[column];
        }
    }
    const double x = cameraPoint[0];
    const double y = cameraPoint[1];
    const double z = cameraPoint[2];
    if (!(z > 1e-9) || !(z < 1e100) || !(fabs(x) < 1e100) || !(fabs(y) < 1e100))
    {
        return block;
    }
    const double fx = input.intrinsics_[0];
    const double fy = input.intrinsics_[1];
    const double nx = x / z;
    const double ny = y / z;
    block.residual_[0] = fx * nx + input.intrinsics_[2] - input.pixel_[0];
    block.residual_[1] = fy * ny + input.intrinsics_[3] - input.pixel_[1];
    for (int axis = 0; axis < 3; ++axis)
    {
        block.pointJacobian_[axis] = fx / z * (input.rotation_[axis] - nx * input.rotation_[6 + axis]);
        block.pointJacobian_[3 + axis] = fy / z * (input.rotation_[3 + axis] - ny * input.rotation_[6 + axis]);
    }
    // Left SE(3) perturbation: Xc' = Exp(omega) Xc + dt.
    block.cameraJacobian_[0] = -fx * nx * ny;
    block.cameraJacobian_[1] = fx * (1.0 + nx * nx);
    block.cameraJacobian_[2] = -fx * ny;
    block.cameraJacobian_[3] = fx / z;
    block.cameraJacobian_[4] = 0.0;
    block.cameraJacobian_[5] = -fx * nx / z;
    block.cameraJacobian_[6] = -fy * (1.0 + ny * ny);
    block.cameraJacobian_[7] = fy * nx * ny;
    block.cameraJacobian_[8] = fy * nx;
    block.cameraJacobian_[9] = 0.0;
    block.cameraJacobian_[10] = fy / z;
    block.cameraJacobian_[11] = -fy * ny / z;
    block.valid_ = true;
    return block;
}
}

#undef SFM_HOST_DEVICE
