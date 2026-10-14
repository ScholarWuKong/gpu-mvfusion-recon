#pragma once
#include "ba/bundle_adjuster.h"
#include <vector>

namespace sfm::ba::detail
{
struct ResidualInput
{
    double rotation_[9];
    double translation_[3];
    double point_[3];
    double intrinsics_[4];
    double pixel_[2];
};

struct ResidualBlock
{
    double residual_[2] = {};
    double pointJacobian_[6] = {};
    double cameraJacobian_[12] = {};
    bool valid_ = false;
};

using ResidualEvaluator = void (*)(const std::vector<ResidualInput>&, std::vector<ResidualBlock>&);
BundleAdjustmentResult runOptimization(scene::Reconstruction& reconstruction,
                                       const BundleAdjusterConfig& config, ResidualEvaluator evaluator);
}
