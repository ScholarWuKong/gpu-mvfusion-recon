cmake_minimum_required(VERSION 3.24)

project(gpu_mvfusion_recon LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

set(OpenCV_DIR ${THIRD_PARTY_DIR}/opencv)

find_package(OpenCV REQUIRED)

find_package(CUDAToolkit REQUIRED)

add_executable(project_i
    src/main.cpp
)

target_link_libraries(project_i
    PRIVATE
    ${OpenCV_LIBS}
    CUDA::cudart
)

set(OpenCV_BIN_DIR "${OpenCV_DIR}/x64/vc17/bin")
file(GLOB OPENCV_DLLS "${OpenCV_BIN_DIR}/*.dll")
add_custom_command(TARGET project_i POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${OPENCV_DLLS}
        "$<TARGET_FILE_DIR:project_i>"
)

target_include_directories(project_i
    PRIVATE
    ${OpenCV_INCLUDE_DIRS}
)