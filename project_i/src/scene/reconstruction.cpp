#include "scene/reconstruction.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace sfm::scene
{
std::size_t Reconstruction::registeredImageCount() const noexcept
{
    return static_cast<std::size_t>(std::count_if(images_.begin(), images_.end(),
        [](const auto& entry)
        {
            return entry.second.registered_;
        }));
}

void Reconstruction::validate() const
{
    for (const auto& entry : images_)
    {
        const auto& image = entry.second;
        if (entry.first != image.id_ || !image.camera_.isValid())
        {
            throw std::invalid_argument("Invalid reconstruction image or camera.");
        }
        for (const auto value : image.pose_.rotation_)
        {
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Non-finite camera rotation.");
            }
        }
        for (const auto value : image.pose_.translation_)
        {
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Non-finite camera translation.");
            }
        }
    }
    std::set<std::pair<ImageId, std::size_t>> assignedFeatures;
    for (std::size_t index = 0; index < tracks_.size(); ++index)
    {
        const auto& track = tracks_[index];
        std::set<ImageId> imageIds;
        if (track.id_ != index)
        {
            throw std::invalid_argument("Track IDs must equal their storage indices.");
        }
        for (const auto& observation : track.observations_)
        {
            const auto image = images_.find(observation.imageId_);
            if (image == images_.end() || !imageIds.insert(observation.imageId_).second ||
                !assignedFeatures.emplace(observation.imageId_, observation.featureIndex_).second ||
                observation.featureIndex_ >= image->second.featureCount_ ||
                !std::isfinite(observation.x_) || !std::isfinite(observation.y_))
            {
                throw std::invalid_argument("Conflicting or invalid track observation.");
            }
        }
        if (track.pointId_ != INVALID_POINT_ID &&
            (track.pointId_ >= points_.size() || points_[track.pointId_].trackId_ != index))
        {
            throw std::invalid_argument("Inconsistent track-to-point association.");
        }
    }
    for (std::size_t index = 0; index < points_.size(); ++index)
    {
        const auto& point = points_[index];
        if (point.id_ != index || point.trackId_ >= tracks_.size() ||
            tracks_[point.trackId_].pointId_ != index || tracks_[point.trackId_].observations_.size() < 2)
        {
            throw std::invalid_argument("Inconsistent point-to-track association.");
        }
        for (const auto value : point.position_)
        {
            if (!std::isfinite(value))
            {
                throw std::invalid_argument("Non-finite 3D point.");
            }
        }
    }
    for (const auto imageId : fixedImageIds_)
    {
        if (images_.find(imageId) == images_.end() || !images_.at(imageId).registered_)
        {
            throw std::invalid_argument("A fixed camera must be registered.");
        }
    }
}
}
