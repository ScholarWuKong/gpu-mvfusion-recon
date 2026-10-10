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
 
# ===== GPT 片段开始：插入在这里 =====
add_library(sfm_headers INTERFACE)
target_include_directories(sfm_headers INTERFACE
    "${CMAKE_SOURCE_DIR}/include"
)

add_library(sfm_scene
    src/scene/image.cpp
    src/scene/feature.cpp
)
target_link_libraries(sfm_scene PUBLIC sfm_headers)

add_library(sfm_io
    src/io/image_io.cpp
)
target_include_directories(sfm_io PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_io PUBLIC sfm_scene ${OpenCV_LIBS})

add_library(sfm_feature_api
    src/feature/feature_extractor.cpp
)
target_link_libraries(sfm_feature_api PUBLIC sfm_scene sfm_headers)

add_library(sfm_feature_cpu
    src/feature/cpu_feature_extractor.cpp
)
target_include_directories(sfm_feature_cpu PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_feature_cpu PUBLIC sfm_feature_api ${OpenCV_LIBS})

add_library(sfm_feature_gpu
    src/feature/gpu_feature_extractor.cpp
)
target_include_directories(sfm_feature_gpu PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_feature_gpu
    PUBLIC sfm_feature_api ${OpenCV_LIBS} CUDA::cudart
)

add_library(sfm_feature_factory
    src/feature/feature_extractor_factory.cpp
)
target_include_directories(sfm_feature_factory PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_feature_factory
    PUBLIC sfm_feature_api sfm_feature_cpu sfm_feature_gpu
)

target_link_libraries(project_i PRIVATE sfm_io sfm_feature_factory)
# ===== GPT 片段结束 =====

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