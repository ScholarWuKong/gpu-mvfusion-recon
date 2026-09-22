cmake_minimum_required(VERSION 3.25)

project(gpu_mvfusion_recon LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(gpu_mvfusion_recon
    src/main.cu
)