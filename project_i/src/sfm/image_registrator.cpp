#include "sfm/image_registrator.h"
#include "geometry/pose_estimation.h"
#include <set>

namespace sfm
{
bool ImageRegistrator::registerImage(const ImageId imageId, scene::Reconstruction& reconstruction,
                                      const MapperConfig& config) const
{
    auto& image = reconstruction.images_.at(imageId);
    if (image.registered_)
    {
        return true;
    }
    std::vector<Vector3> points;
    std::vector<geometry::PixelPoint> pixels;
    std::vector<scene::Observation*> observations;
    for (auto& track : reconstruction.tracks_)
    {
        if (track.pointId_ == INVALID_POINT_ID)
        {
            continue;
        }
        for (auto& observation : track.observations_)
        {
            if (observation.imageId_ == imageId && observation.enabled_)
            {
                points.push_back(reconstruction.points_.at(track.pointId_).position_);
                pixels.push_back({observation.x_, observation.y_});
                observations.push_back(&observation);
            }
        }
    }
    if (points.size() < config.minPnpInliers_)
    {
        return false;
    }
    const auto estimate = geometry::estimateAbsolutePose(points, pixels, image.camera_, config.maxReprojectionError_);
    if (!estimate.valid_ || estimate.inlierIndices_.size() < config.minPnpInliers_)
    {
        return false;
    }
    image.pose_ = estimate.pose_;
    image.registered_ = true;
    const std::set<std::size_t> inliers(estimate.inlierIndices_.begin(), estimate.inlierIndices_.end());
    for (std::size_t index = 0; index < observations.size(); ++index)
    {
        observations[index]->enabled_ = inliers.count(index) != 0;
    }
    return true;
}
}
