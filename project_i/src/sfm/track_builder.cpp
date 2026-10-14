#include "sfm/track_builder.h"
#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace sfm
{
std::size_t TrackBuilder::extend(const scene::MatchGraph& graph,
                                 const std::vector<scene::FeatureSet>& featureSets,
                                 scene::Reconstruction& reconstruction) const
{
    using ObservationKey = std::pair<ImageId, std::size_t>;
    std::map<ImageId, const scene::FeatureSet*> featuresById;
    for (const auto& features : featureSets)
    {
        if (!features.hasValidDescriptors() || !featuresById.emplace(features.imageId_, &features).second)
        {
            throw std::invalid_argument("Invalid or duplicate features during track construction.");
        }
    }
    std::map<ObservationKey, TrackId> membership;
    for (const auto& track : reconstruction.tracks_)
    {
        for (const auto& observation : track.observations_)
        {
            if (!membership.emplace(ObservationKey(observation.imageId_, observation.featureIndex_), track.id_).second)
            {
                throw std::invalid_argument("One feature belongs to multiple tracks.");
            }
        }
    }
    std::size_t rejected = 0;
    for (const auto& edge : graph.pairs())
    {
        const auto& pair = edge.second;
        if (pair.geometryStatus_ != scene::GeometryStatus::VERIFIED)
        {
            continue;
        }
        const auto& first = *featuresById.at(pair.pair_.imageId1_);
        const auto& second = *featuresById.at(pair.pair_.imageId2_);
        auto indices = pair.inlierMatchIndices_;
        std::stable_sort(indices.begin(), indices.end(), [&](const auto left, const auto right)
        {
            return pair.matches_.at(left).distance_ < pair.matches_.at(right).distance_;
        });
        for (const auto index : indices)
        {
            const auto& match = pair.matches_.at(index);
            const ObservationKey firstKey(first.imageId_, match.featureIndex1_);
            const ObservationKey secondKey(second.imageId_, match.featureIndex2_);
            const auto& firstKeypoint = first.features_.at(match.featureIndex1_).keypoint_;
            const auto& secondKeypoint = second.features_.at(match.featureIndex2_).keypoint_;
            const scene::Observation firstObservation{first.imageId_, match.featureIndex1_, firstKeypoint.x_, firstKeypoint.y_};
            const scene::Observation secondObservation{second.imageId_, match.featureIndex2_, secondKeypoint.x_, secondKeypoint.y_};
            const auto firstFound = membership.find(firstKey);
            const auto secondFound = membership.find(secondKey);
            if (firstFound == membership.end() && secondFound == membership.end())
            {
                const TrackId id = reconstruction.tracks_.size();
                reconstruction.tracks_.push_back({id, {firstObservation, secondObservation}, INVALID_POINT_ID});
                membership[firstKey] = id;
                membership[secondKey] = id;
                continue;
            }
            if (firstFound == membership.end() || secondFound == membership.end())
            {
                const auto id = firstFound == membership.end() ? secondFound->second : firstFound->second;
                auto& track = reconstruction.tracks_.at(id);
                const auto& observation = firstFound == membership.end() ? firstObservation : secondObservation;
                const bool conflict = std::any_of(track.observations_.begin(), track.observations_.end(),
                    [&](const auto& existing)
                    {
                        return existing.imageId_ == observation.imageId_;
                    });
                if (conflict)
                {
                    ++rejected;
                    continue;
                }
                track.observations_.push_back(observation);
                membership[{observation.imageId_, observation.featureIndex_}] = id;
                continue;
            }
            if (firstFound->second == secondFound->second)
            {
                continue;
            }
            auto& destination = reconstruction.tracks_.at(firstFound->second);
            auto& source = reconstruction.tracks_.at(secondFound->second);
            bool conflict = destination.pointId_ != INVALID_POINT_ID && source.pointId_ != INVALID_POINT_ID;
            for (const auto& left : destination.observations_)
            {
                for (const auto& right : source.observations_)
                {
                    conflict = conflict || left.imageId_ == right.imageId_;
                }
            }
            if (conflict)
            {
                ++rejected;
                continue;
            }
            for (const auto& observation : source.observations_)
            {
                destination.observations_.push_back(observation);
                membership[{observation.imageId_, observation.featureIndex_}] = destination.id_;
            }
            if (source.pointId_ != INVALID_POINT_ID)
            {
                destination.pointId_ = source.pointId_;
                reconstruction.points_.at(source.pointId_).trackId_ = destination.id_;
            }
            source.observations_.clear();
            source.pointId_ = INVALID_POINT_ID;
        }
    }
    std::vector<scene::Track> compact;
    for (auto& track : reconstruction.tracks_)
    {
        if (track.observations_.empty())
        {
            continue;
        }
        track.id_ = compact.size();
        if (track.pointId_ != INVALID_POINT_ID)
        {
            reconstruction.points_.at(track.pointId_).trackId_ = track.id_;
        }
        compact.push_back(std::move(track));
    }
    reconstruction.tracks_ = std::move(compact);
    return rejected;
}
}
