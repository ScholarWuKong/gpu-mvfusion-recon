#include "ba/gpu_bundle_adjuster.h"
#include "cuda/memory/cuda_buffer.h"
#include "residual_internal.h"
#include <limits>
#include <stdexcept>

namespace sfm::ba
{
namespace
{
__global__ void computeResidualsKernel(const detail::ResidualInput* inputs,
                                       detail::ResidualBlock* blocks, const std::size_t count)
{
    const std::size_t index = static_cast<std::size_t>(blockIdx.x) * blockDim.x + threadIdx.x;
    if (index < count)
    {
        blocks[index] = detail::computeResidual(inputs[index]);
    }
}

void evaluateGpu(const std::vector<detail::ResidualInput>& inputs,
                  std::vector<detail::ResidualBlock>& blocks)
{
    blocks.resize(inputs.size());
    if (inputs.empty())
    {
        return;
    }
    constexpr unsigned int THREADS_PER_BLOCK = 256;
    const auto blockCount = (inputs.size() + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
    if (blockCount > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        throw std::overflow_error("Too many observations for a CUDA grid.");
    }
    cuda::CudaBuffer<detail::ResidualInput> gpuInputs(inputs.size());
    cuda::CudaBuffer<detail::ResidualBlock> gpuBlocks(inputs.size());
    gpuInputs.upload(inputs.data());
    computeResidualsKernel<<<static_cast<unsigned int>(blockCount), THREADS_PER_BLOCK>>>(
        gpuInputs.data(), gpuBlocks.data(), inputs.size());
    cuda::checkCuda(cudaGetLastError(), "BA residual kernel launch");
    gpuBlocks.download(blocks.data());
}
}

GpuBundleAdjuster::GpuBundleAdjuster(const BundleAdjusterConfig& config) : config_(config)
{
    config_.validate();
    int deviceCount = 0;
    cuda::checkCuda(cudaGetDeviceCount(&deviceCount), "cudaGetDeviceCount");
    if (deviceCount <= 0)
    {
        throw std::runtime_error("GPU bundle adjustment requires a CUDA device.");
    }
}

BundleAdjustmentResult GpuBundleAdjuster::optimize(scene::Reconstruction& reconstruction)
{
    return detail::runOptimization(reconstruction, config_, evaluateGpu);
}
}
