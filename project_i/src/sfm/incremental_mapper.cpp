#include "sfm/incremental_mapper.h"
#include "geometry/two_view_geometry.h"
#include "sfm/image_registrator.h"
#include "sfm/initializer.h"
#include "sfm/observation_manager.h"
#include "sfm/track_builder.h"
#include <set>
#include <stdexcept>

namespace sfm
{
IncrementalMapper::IncrementalMapper(feature::IFeatureExtractor& extractor,
                                     matching::IFeatureMatcher& matcher,
                                     ba::IBundleAdjuster* bundleAdjuster, const MapperConfig& config)
    : extractor_(extractor), matcher_(matcher), bundleAdjuster_(bundleAdjuster), config_(config)
{
    config_.validate();
    if (config_.runBundleAdjustment_ && bundleAdjuster_ == nullptr)
    {
        throw std::invalid_argument("Bundle adjustment is enabled but no optimizer was injected.");
    }
}

void IncrementalMapper::addImage(const scene::Image& image, const scene::Camera& camera,
                                 const std::vector<ImageId>& candidateIds)
{
    if (!image.isValid() || !camera.isValid() || reconstruction_.images_.count(image.id_) != 0)
    {
        throw std::invalid_argument("Invalid image/camera or duplicate image ID.");
    }
    std::set<ImageId> candidates(candidateIds.begin(), candidateIds.end());
    if (candidateIds.empty())
    {
        for (const auto& entry : reconstruction_.images_)
        {
            candidates.insert(entry.first);
        }
    }
    std::vector<scene::ImagePair> pairs;
    for (const auto candidate : candidates)
    {
        if (reconstruction_.images_.count(candidate) == 0)
        {
            throw std::invalid_argument("Candidate image is not in this mapper.");
        }
        pairs.emplace_back(candidate, image.id_);
    }
    auto features = extractor_.extract(image);
    if (features.imageId_ != image.id_ || !features.hasValidDescriptors())
    {
        throw std::runtime_error("Extractor returned inconsistent feature data.");
    }
    featureSets_.push_back(std::move(features));
    scene::MatchGraph newMatches;
    try
    {
        if (!pairs.empty())
        {
            newMatches = matcher_.matchPairs(featureSets_, pairs);
        }
        const std::set<scene::ImagePair> requested(pairs.begin(), pairs.end());
        if (newMatches.pairs().size() != requested.size())
        {
            throw std::runtime_error("Matcher must return one attempted edge per requested pair.");
        }
        for (auto& edge : newMatches.pairs())
        {
            auto& pair = edge.second;
            if (requested.count(pair.pair_) == 0 || !pair.attempted_)
            {
                throw std::runtime_error("Matcher returned an unexpected or unattempted pair.");
            }
            const scene::FeatureSet* first = nullptr;
            const scene::FeatureSet* second = nullptr;
            for (const auto& set : featureSets_)
            {
                if (set.imageId_ == pair.pair_.imageId1_)
                {
                    first = &set;
                }
                if (set.imageId_ == pair.pair_.imageId2_)
                {
                    second = &set;
                }
            }
            if (first == nullptr || second == nullptr)
            {
                throw std::runtime_error("Matcher returned an unexpected image pair.");
            }
            std::vector<geometry::PixelPoint> firstPixels;
            std::vector<geometry::PixelPoint> secondPixels;
            for (const auto& match : pair.matches_)
            {
                const auto& firstPoint = first->features_.at(match.featureIndex1_).keypoint_;
                const auto& secondPoint = second->features_.at(match.featureIndex2_).keypoint_;
                firstPixels.push_back({firstPoint.x_, firstPoint.y_});
                secondPixels.push_back({secondPoint.x_, secondPoint.y_});
            }
            const auto verified = geometry::verifyTwoView(firstPixels, secondPixels,
                first->imageId_ == image.id_ ? camera : reconstruction_.images_.at(first->imageId_).camera_,
                second->imageId_ == image.id_ ? camera : reconstruction_.images_.at(second->imageId_).camera_, config_.geometry_);
            pair.geometryStatus_ = verified.verified_ ? scene::GeometryStatus::VERIFIED : scene::GeometryStatus::REJECTED;
            pair.inlierMatchIndices_ = verified.inlierIndices_;
            pair.fundamental_ = verified.fundamental_;
            pair.essential_ = verified.essential_;
            pair.relativePose_ = verified.relativePose_;
        }
    }
    catch (...)
    {
        featureSets_.pop_back();
        throw;
    }
    reconstruction_.images_.emplace(image.id_, scene::ImageState{
        image.id_, image.width_, image.height_, featureSets_.back().size(), camera, {}, false});
    for (const auto& edge : newMatches.pairs())
    {
        graph_.set(edge.second);
    }
    rejectedTrackConnections_ += TrackBuilder{}.extend(newMatches, featureSets_, reconstruction_);
    advanceReconstruction();
    reconstruction_.validate();
}

void IncrementalMapper::advanceReconstruction()
{
    bool changed = false;
    if (reconstruction_.registeredImageCount() == 0)
    {
        if (!Initializer{}.initialize(graph_, reconstruction_, config_))
        {
            return;
        }
        changed = true;
    }
    bool progress = true;
    while (progress)
    {
        progress = false;
        for (auto& entry : reconstruction_.images_)
        {
            if (!entry.second.registered_ && ImageRegistrator{}.registerImage(entry.first, reconstruction_, config_))
            {
                progress = true;
                changed = true;
                ObservationManager{}.filterObservations(reconstruction_, config_.maxReprojectionError_);
                ObservationManager{}.triangulateTracks(reconstruction_, config_);
            }
        }
    }
    const auto added = ObservationManager{}.triangulateTracks(reconstruction_, config_);
    if (config_.runBundleAdjustment_ && (changed || added != 0))
    {
        ObservationManager{}.filterObservations(reconstruction_, config_.maxReprojectionError_);
        lastBundleAdjustment_ = bundleAdjuster_->optimize(reconstruction_);
        ObservationManager{}.filterObservations(reconstruction_, config_.maxReprojectionError_);
    }
}

const scene::Reconstruction& IncrementalMapper::reconstruction() const noexcept
{
    return reconstruction_;
}

const scene::MatchGraph& IncrementalMapper::matchGraph() const noexcept
{
    return graph_;
}

const std::vector<scene::FeatureSet>& IncrementalMapper::featureSets() const noexcept
{
    return featureSets_;
}

const ba::BundleAdjustmentResult& IncrementalMapper::lastBundleAdjustment() const noexcept
{
    return lastBundleAdjustment_;
}

std::size_t IncrementalMapper::rejectedTrackConnections() const noexcept
{
    return rejectedTrackConnections_;
}
}
