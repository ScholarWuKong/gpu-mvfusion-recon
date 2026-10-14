#include "ba/cpu_bundle_adjuster.h"
#include "residual_internal.h"

namespace sfm::ba
{
namespace
{
void evaluateCpu(const std::vector<detail::ResidualInput>& inputs,
                  std::vector<detail::ResidualBlock>& blocks)
{
    blocks.resize(inputs.size());
    for (std::size_t index = 0; index < inputs.size(); ++index)
    {
        blocks[index] = detail::computeResidual(inputs[index]);
    }
}
}

CpuBundleAdjuster::CpuBundleAdjuster(const BundleAdjusterConfig& config) : config_(config)
{
    config_.validate();
}

BundleAdjustmentResult CpuBundleAdjuster::optimize(scene::Reconstruction& reconstruction)
{
    return detail::runOptimization(reconstruction, config_, evaluateCpu);
}
}
