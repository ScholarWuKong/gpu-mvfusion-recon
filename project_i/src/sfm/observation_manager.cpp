#include "sfm/observation_manager.h"
#include "geometry/triangulation.h"

namespace sfm
{
std::size_t ObservationManager::triangulateTracks(scene::Reconstruction& reconstruction,
                                                  const MapperConfig& config) const
{
    std::size_t added = 0;
    for (auto& track : reconstruction.tracks_)
    {
        if (track.pointId_ != INVALID_POINT_ID)
        {
            continue;
        }
        std::vector<geometry::TriangulationView> views;
        for (const auto& observation : track.observations_)
        {
            const auto& image = reconstruction.images_.at(observation.imageId_);
            if (image.registered_ && observation.enabled_)
            {
                views.push_back({image.camera_, image.pose_, {observation.x_, observation.y_}});
            }
        }
        Vector3 position;
        if (geometry::triangulatePoint(views, position, config.maxReprojectionError_, config.minTriangulationAngle_))
        {
            const PointId id = reconstruction.points_.size();
            reconstruction.points_.push_back({id, track.id_, position});
            track.pointId_ = id;
            ++added;
        }
    }
    return added;
}

void ObservationManager::filterObservations(scene::Reconstruction& reconstruction,
                                             const double maxReprojectionError) const
{
    for (const auto& point : reconstruction.points_)
    {
        auto& track = reconstruction.tracks_.at(point.trackId_);
        for (auto& observation : track.observations_)
        {
            const auto& image = reconstruction.images_.at(observation.imageId_);
            if (image.registered_ && geometry::computeReprojectionError(point.position_,
                {image.camera_, image.pose_, {observation.x_, observation.y_}}) > maxReprojectionError)
            {
                observation.enabled_ = false;
            }
        }
    }
    // Remove unsupported points and repair both directions of the compact IDs.
    std::vector<scene::Point3D> retainedPoints;
    for (auto point : reconstruction.points_)
    {
        auto& track = reconstruction.tracks_.at(point.trackId_);
        std::size_t support = 0;
        for (const auto& observation : track.observations_)
        {
            if (observation.enabled_ && reconstruction.images_.at(observation.imageId_).registered_)
            {
                ++support;
            }
        }
        if (support < 2)
        {
            track.pointId_ = INVALID_POINT_ID;
            continue;
        }
        point.id_ = retainedPoints.size();
        track.pointId_ = point.id_;
        retainedPoints.push_back(point);
    }
    reconstruction.points_ = std::move(retainedPoints);
}
}
