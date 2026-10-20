#include "feature_selection.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace sfm::feature::detail
{
scene::FeatureSet distributeFeatures(scene::FeatureSet candidates, const int width, const int height,
                                     const std::size_t maximumFeatures, const int columns, const int rows)
{
    if (!candidates.hasValidDescriptors() || width <= 0 || height <= 0 || maximumFeatures == 0 ||
        columns <= 0 || rows <= 0 || columns > 64 || rows > 64)
    {
        throw std::invalid_argument("Invalid spatial feature selection inputs.");
    }
    std::vector<std::vector<std::size_t>> cells(static_cast<std::size_t>(columns * rows));
    for (std::size_t index = 0; index < candidates.size(); ++index)
    {
        const auto& keypoint = candidates.features_[index].keypoint_;
        if (keypoint.x_ < 0.0f || keypoint.y_ < 0.0f || keypoint.x_ >= width || keypoint.y_ >= height ||
            !std::isfinite(keypoint.response_))
        {
            throw std::invalid_argument("Feature coordinates or response are invalid for spatial selection.");
        }
        const int column = std::min(columns - 1, static_cast<int>(keypoint.x_ * columns / width));
        const int row = std::min(rows - 1, static_cast<int>(keypoint.y_ * rows / height));
        cells[static_cast<std::size_t>(row * columns + column)].push_back(index);
    }
    if (candidates.size() <= maximumFeatures)
    {
        return candidates;
    }
    for (auto& cell : cells)
    {
        std::stable_sort(cell.begin(), cell.end(), [&](const auto first, const auto second)
        {
            return candidates.features_[first].keypoint_.response_ > candidates.features_[second].keypoint_.response_;
        });
    }
    std::vector<std::size_t> offsets(cells.size(), 0);
    std::vector<std::size_t> selected;
    selected.reserve(maximumFeatures);
    while (selected.size() < maximumFeatures)
    {
        for (std::size_t cell = 0; cell < cells.size() && selected.size() < maximumFeatures; ++cell)
        {
            if (offsets[cell] < cells[cell].size())
            {
                selected.push_back(cells[cell][offsets[cell]++]);
            }
        }
    }
    // Preserve source order and copy the corresponding descriptor row verbatim.
    std::sort(selected.begin(), selected.end());
    scene::FeatureSet result;
    result.imageId_ = candidates.imageId_;
    result.descriptorSize_ = candidates.descriptorSize_;
    result.features_.reserve(selected.size());
    result.descriptors_.reserve(selected.size() * result.descriptorSize_);
    for (const auto index : selected)
    {
        result.features_.push_back(candidates.features_[index]);
        const auto begin = candidates.descriptors_.begin() + index * result.descriptorSize_;
        result.descriptors_.insert(result.descriptors_.end(), begin, begin + result.descriptorSize_);
    }
    return result;
}
}
