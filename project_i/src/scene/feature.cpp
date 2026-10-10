#include "scene/feature.h"

#include <limits>

namespace sfm::scene
{

bool FeatureSet::empty() const noexcept
{
    return features_.empty();
}

std::size_t FeatureSet::size() const noexcept
{
    return features_.size();
}

bool FeatureSet::hasValidDescriptors() const noexcept
{
    if (features_.empty())
    {
        return descriptors_.empty();
    }

    if (descriptorSize_ == 0 ||
        features_.size() > std::numeric_limits<std::size_t>::max() / descriptorSize_)
    {
        return false;
    }

    return descriptors_.size() == features_.size() * descriptorSize_;
}

} // namespace sfm::scene
