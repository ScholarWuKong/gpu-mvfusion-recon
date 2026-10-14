#include "ba/bundle_adjuster.h"
#include "ba/cpu_bundle_adjuster.h"
#include "ba/gpu_bundle_adjuster.h"
#include <stdexcept>

namespace sfm::ba
{
std::unique_ptr<IBundleAdjuster> createBundleAdjuster(const BundleAdjusterConfig& config)
{
    config.validate();
    switch (config.backend_)
    {
    case BundleAdjustmentBackend::CPU:
        return std::make_unique<CpuBundleAdjuster>(config);
    case BundleAdjustmentBackend::GPU:
        return std::make_unique<GpuBundleAdjuster>(config);
    default:
        throw std::invalid_argument("Unsupported BA backend.");
    }
}
}
