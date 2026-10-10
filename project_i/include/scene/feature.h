#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sfm::scene
{

struct Keypoint
{
    float x_ = 0.0f;
    float y_ = 0.0f;
    float size_ = 0.0f;
    float angle_ = 0.0f;
    float response_ = 0.0f;
    int octave_ = 0;
    int classId_ = -1;
};

struct Feature
{
    std::uint32_t imageId_ = 0;
    Keypoint keypoint_;
};

struct FeatureSet
{
    std::uint32_t imageId_ = 0;
    std::vector<Feature> features_;

    // Descriptor row i belongs to features_[i]. ORB descriptors are 32 bytes each.
    std::size_t descriptorSize_ = 0;
    std::vector<std::uint8_t> descriptors_;

    bool empty() const noexcept;
    std::size_t size() const noexcept;
    bool hasValidDescriptors() const noexcept;
};

} // namespace sfm::scene
