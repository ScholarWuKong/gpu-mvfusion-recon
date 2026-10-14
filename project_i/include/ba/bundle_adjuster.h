#pragma once

#include "scene/reconstruction.h"
#include <memory>

namespace sfm::ba
{
enum class BundleAdjustmentBackend
{
    CPU,
    GPU
};

struct BundleAdjusterConfig
{
    BundleAdjustmentBackend backend_ = BundleAdjustmentBackend::CPU;
    int maxIterations_ = 15;
    double huberDelta_ = 2.0;
    double initialDamping_ = 1e-3;
    double relativeTolerance_ = 1e-6;
    void validate() const;
};

struct BundleAdjustmentResult
{
    bool attempted_ = false;
    bool converged_ = false;
    int iterations_ = 0;
    std::size_t observationCount_ = 0;
    double initialCost_ = 0.0;
    double finalCost_ = 0.0;
};

class IBundleAdjuster
{
public:
    virtual ~IBundleAdjuster() = default;
    virtual BundleAdjustmentResult optimize(scene::Reconstruction& reconstruction) = 0;
};

std::unique_ptr<IBundleAdjuster> createBundleAdjuster(const BundleAdjusterConfig& config);
}
