#pragma once

#include "scene/camera.h"
#include "scene/point3d.h"
#include "scene/track.h"
#include <map>
#include <vector>

namespace sfm::scene
{
struct ImageState
{
    ImageId id_ = 0;
    int width_ = 0;
    int height_ = 0;
    std::size_t featureCount_ = 0;
    Camera camera_;
    CameraPose pose_;
    bool registered_ = false;
};

struct Reconstruction
{
    std::map<ImageId, ImageState> images_;
    std::vector<Track> tracks_;
    std::vector<Point3D> points_;
    // Both seed cameras stay fixed during BA to remove pose/scale gauge freedoms.
    std::vector<ImageId> fixedImageIds_;

    std::size_t registeredImageCount() const noexcept;
    void validate() const;
};
}
