# SfM Feature Extraction Module (CPU ORB + OpenCV CUDA ORB)

This package is an initial, independently buildable feature-extraction slice for the `project_i` tree. It contains:

- backend-independent `scene::Image`, `scene::Feature`, and `scene::FeatureSet` data types;
- `feature::IFeatureExtractor` and `FeatureExtractorConfig`;
- CPU ORB (`cv::ORB`) and CUDA ORB (`cv::cuda::ORB`) implementations;
- a factory that selects the implementation from `FeatureBackend`;
- an image-loading helper and a two-image demo;
- target-based CMake wiring.

## Integrating into the existing project

The archive uses the same project-relative paths as the proposed layout. Copy the files under `include/` and `src/` into the matching folders. It includes `include/scene/image.h`, `include/scene/feature.h`, and their `.cpp` files; merge these definitions with your current scene classes if those files already contain implementations you need to preserve.

`CMakeLists.txt` is a *minimal feature-demo CMake file*: it deliberately builds only image/feature data, image IO, and feature extraction. It does not yet add the existing geometry, matching, SfM, BA, or custom CUDA-kernel sources. If you want to preserve the root CMake configuration, use `cmake/feature_module_targets.cmake`: include it after `find_package(OpenCV ...)`, `find_package(CUDAToolkit ...)`, and `add_executable(project_i src/main.cpp)`. It adds the feature targets and links them to the existing `project_i` executable without replacing the rest of your build setup.

## Build

Run from the project root (the directory containing `CMakeLists.txt` and `third_party/`):

```powershell
cmake -S . -B build -A x64
cmake --build build --config RelWithDebInfo
```

The supplied OpenCV package must expose the `core`, `imgproc`, `imgcodecs`, `features2d`, and `cudafeatures2d` components. If CMake cannot find `cudafeatures2d`, the prebuilt OpenCV package may not contain CUDA feature support, or `OpenCV_DIR` may point to the wrong directory.

Run with two image paths:

```powershell
.\build\RelWithDebInfo\project_i.exe .\data\image1.jpg .\data\image2.jpg
```

The demo always runs CPU ORB. If CUDA ORB cannot be initialized or run, it prints a diagnostic and skips the GPU portion.

## Data contract

`FeatureSet::descriptors_` is a contiguous row-major byte array: descriptor row `i` belongs to `features_[i]`, and each row has `descriptorSize_` bytes. For ORB, this is normally 32 bytes. Empty feature sets are considered valid only when the descriptor array is also empty.

The GPU implementation calls the prebuilt OpenCV CUDA API and is therefore a `.cpp` file, not a custom CUDA kernel. A `.cu` file is needed when this project compiles its own `__global__`/`__device__` code.
