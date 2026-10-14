cmake_minimum_required(VERSION 3.24)
project(gpu_mvfusion_recon LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")
set(OpenCV_DIR "${THIRD_PARTY_DIR}/opencv" CACHE PATH "Prebuilt OpenCV 4.12 package")
find_package(OpenCV 4.12 REQUIRED COMPONENTS core imgproc imgcodecs features2d calib3d cudafeatures2d)
find_package(CUDAToolkit REQUIRED)

add_library(sfm_util INTERFACE)
target_include_directories(sfm_util INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(sfm_util INTERFACE cxx_std_17)
if(MSVC)
    target_compile_options(sfm_util INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:/utf-8>" "$<$<COMPILE_LANGUAGE:CXX>:/W4>")
endif()

add_library(sfm_geometry
    src/geometry/types.cpp
    src/geometry/fundamental.cpp
    src/geometry/essential.cpp
    src/geometry/pose_estimation.cpp
    src/geometry/triangulation.cpp
    src/geometry/two_view_geometry.cpp
)
target_include_directories(sfm_geometry SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_geometry PUBLIC sfm_util PRIVATE opencv_core opencv_calib3d)

add_library(sfm_scene src/scene/image.cpp src/scene/feature.cpp src/scene/match.cpp src/scene/reconstruction.cpp)
target_link_libraries(sfm_scene PUBLIC sfm_util sfm_geometry)

add_library(sfm_feature_api src/feature/feature_extractor.cpp)
target_link_libraries(sfm_feature_api PUBLIC sfm_scene)
add_library(sfm_feature_cpu src/feature/cpu_feature_extractor.cpp)
target_include_directories(sfm_feature_cpu SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_feature_cpu PUBLIC sfm_feature_api PRIVATE opencv_features2d opencv_imgproc)
add_library(sfm_feature_gpu src/feature/gpu_feature_extractor.cpp)
target_include_directories(sfm_feature_gpu SYSTEM PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_feature_gpu PUBLIC sfm_feature_api PRIVATE opencv_cudafeatures2d opencv_imgproc CUDA::cudart)
add_library(sfm_feature_factory src/feature/feature_extractor_factory.cpp)
target_link_libraries(sfm_feature_factory PUBLIC sfm_feature_api PRIVATE sfm_feature_cpu sfm_feature_gpu)

add_library(sfm_matching_api src/matching/feature_matcher.cpp)
target_link_libraries(sfm_matching_api PUBLIC sfm_scene)
add_library(sfm_matching_gpu src/matching/gpu_feature_matcher.cpp)
target_include_directories(sfm_matching_gpu SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_matching_gpu PUBLIC sfm_matching_api PRIVATE opencv_cudafeatures2d CUDA::cudart)
add_library(sfm_matching_factory src/matching/feature_matcher_factory.cpp)
target_link_libraries(sfm_matching_factory PUBLIC sfm_matching_api PRIVATE sfm_matching_gpu)

add_library(sfm_ba_api src/ba/bundle_adjuster.cpp)
target_link_libraries(sfm_ba_api PUBLIC sfm_scene)
add_library(sfm_ba_solver src/ba/optimizer.cpp)
target_include_directories(sfm_ba_solver SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_ba_solver PUBLIC sfm_ba_api PRIVATE opencv_core opencv_calib3d)
add_library(sfm_ba_cpu src/ba/cpu_bundle_adjuster.cpp)
target_link_libraries(sfm_ba_cpu PUBLIC sfm_ba_api PRIVATE sfm_ba_solver)
add_library(sfm_ba_gpu src/ba/gpu_bundle_adjuster.cu)
target_link_libraries(sfm_ba_gpu PUBLIC sfm_ba_api PRIVATE sfm_ba_solver CUDA::cudart)
add_library(sfm_ba_factory src/ba/bundle_adjuster_factory.cpp)
target_link_libraries(sfm_ba_factory PUBLIC sfm_ba_api PRIVATE sfm_ba_cpu sfm_ba_gpu)

add_library(sfm_mapper
    src/app/config.cpp
    src/sfm/track_builder.cpp
    src/sfm/observation_manager.cpp
    src/sfm/initializer.cpp
    src/sfm/image_registrator.cpp
    src/sfm/incremental_mapper.cpp
)
target_link_libraries(sfm_mapper PUBLIC sfm_scene sfm_feature_api sfm_matching_api sfm_ba_api)

add_library(sfm_io src/io/image_io.cpp src/io/reconstruction_io.cpp)
target_include_directories(sfm_io SYSTEM PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(sfm_io PUBLIC sfm_scene PRIVATE opencv_imgcodecs)

add_library(sfm_app src/app/application.cpp)
target_link_libraries(sfm_app PUBLIC sfm_util PRIVATE sfm_mapper sfm_io sfm_feature_factory sfm_matching_factory sfm_ba_factory)
add_executable(project_i src/main.cpp)
target_link_libraries(project_i PRIVATE sfm_app)

include(CTest)
if(BUILD_TESTING)
    target_sources(sfm_app PRIVATE tests/integration_checks.cpp)
    target_compile_definitions(sfm_app PRIVATE SFM_ENABLE_INTEGRATION_CHECKS)
    add_test(NAME sfm_integration COMMAND project_i --self-test)
endif()

# Only discover runtime DLLs, never source files. Retain the existing prebuilt layout.
set(OpenCV_BIN_DIR "${OpenCV_DIR}/x64/vc17/bin")
file(GLOB OPENCV_DLLS "${OpenCV_BIN_DIR}/*.dll")
if(NOT OPENCV_DLLS)
    message(FATAL_ERROR "No OpenCV runtime DLLs found in ${OpenCV_BIN_DIR}")
endif()
add_custom_command(TARGET project_i POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OPENCV_DLLS} "$<TARGET_FILE_DIR:project_i>"
    COMMAND_EXPAND_LISTS
    VERBATIM
)
