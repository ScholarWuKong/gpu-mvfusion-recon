#pragma once
#include "ba/bundle_adjuster.h"

namespace sfm::ba
{
class GpuBundleAdjuster final : public IBundleAdjuster
{
public:
    explicit GpuBundleAdjuster(const BundleAdjusterConfig& config);
    BundleAdjustmentResult optimize(scene::Reconstruction& reconstruction) override;

private:
    BundleAdjusterConfig config_;
};
using GPUBundleAdjuster = GpuBundleAdjuster;
}
