#include "sfm/initializer.h"
#include "sfm/observation_manager.h"
#include <algorithm>

namespace sfm
{
bool Initializer::initialize(const scene::MatchGraph& graph, scene::Reconstruction& reconstruction,
                             const MapperConfig& config) const
{
    std::vector<const scene::PairwiseMatch*> candidates;
    for (const auto& edge : graph.pairs())
    {
        if (edge.second.geometryStatus_ == scene::GeometryStatus::VERIFIED)
        {
            candidates.push_back(&edge.second);
        }
    }
    std::stable_sort(candidates.begin(), candidates.end(), [](const auto* first, const auto* second)
    {
        return first->inlierMatchIndices_.size() > second->inlierMatchIndices_.size();
    });
    for (const auto* candidate : candidates)
    {
        auto trial = reconstruction;
        auto& first = trial.images_.at(candidate->pair_.imageId1_);
        auto& second = trial.images_.at(candidate->pair_.imageId2_);
        first.registered_ = true;
        first.pose_ = scene::CameraPose{};
        second.registered_ = true;
        second.pose_ = candidate->relativePose_;
        trial.fixedImageIds_ = {first.id_, second.id_};
        ObservationManager{}.triangulateTracks(trial, config);
        if (trial.points_.size() >= config.minInitialPoints_)
        {
            reconstruction = std::move(trial);
            return true;
        }
    }
    return false;
}
}
