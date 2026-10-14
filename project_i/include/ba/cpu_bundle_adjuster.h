#pragma once
#include "ba/bundle_adjuster.h"

namespace sfm::ba
{
class CpuBundleAdjuster final : public IBundleAdjuster
{
public:
    explicit CpuBundleAdjuster(const BundleAdjusterConfig& config);
    BundleAdjustmentResult optimize(scene::Reconstruction& reconstruction) override;

private:
    BundleAdjusterConfig config_;
};
using CPUBundleAdjuster = CpuBundleAdjuster;
}
