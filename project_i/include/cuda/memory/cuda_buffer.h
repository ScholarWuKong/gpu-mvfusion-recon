#pragma once
#include "cuda/memory/cuda_error.h"
#include <cstddef>
#include <limits>
#include <utility>

namespace sfm::cuda
{
// Used by custom BA kernels only. OpenCV matching owns its GpuMat allocations.
template <typename T>
class CudaBuffer
{
public:
    explicit CudaBuffer(const std::size_t count) : count_(count)
    {
        if (count > std::numeric_limits<std::size_t>::max() / sizeof(T))
        {
            throw std::overflow_error("CUDA allocation size overflow.");
        }
        if (count != 0)
        {
            checkCuda(cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(T)), "cudaMalloc");
        }
    }

    ~CudaBuffer()
    {
        if (data_ != nullptr)
        {
            cudaFree(data_);
        }
    }

    CudaBuffer(const CudaBuffer&) = delete;
    CudaBuffer& operator=(const CudaBuffer&) = delete;

    T* data() noexcept
    {
        return data_;
    }

    void upload(const T* source)
    {
        if (count_ != 0)
        {
            checkCuda(cudaMemcpy(data_, source, count_ * sizeof(T), cudaMemcpyHostToDevice), "CUDA upload");
        }
    }

    void download(T* destination) const
    {
        if (count_ != 0)
        {
            checkCuda(cudaMemcpy(destination, data_, count_ * sizeof(T), cudaMemcpyDeviceToHost), "CUDA download");
        }
    }

private:
    T* data_ = nullptr;
    std::size_t count_ = 0;
};
}
