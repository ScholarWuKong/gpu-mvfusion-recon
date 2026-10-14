#pragma once
#include <cuda_runtime.h>
#include <stdexcept>
#include <string>

namespace sfm::cuda
{
inline void checkCuda(const cudaError_t error, const char* operation)
{
    if (error != cudaSuccess)
    {
        throw std::runtime_error(std::string(operation) + ": " + cudaGetErrorString(error));
    }
}
}
