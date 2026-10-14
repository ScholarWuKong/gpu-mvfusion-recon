#pragma once

#include "ba/bundle_adjuster.h"
#include "config.h"
#include "feature/feature_extractor.h"
#include "matching/feature_matcher.h"
#include "scene/reconstruction.h"

namespace sfm
{
class IncrementalMapper
{
public:
    IncrementalMapper(feature::IFeatureExtractor& extractor,
                       matching::IFeatureMatcher& matcher,
                       ba::IBundleAdjuster* bundleAdjuster, const MapperConfig& config);
    // Empty candidates means all existing images. Previous pairs are retained.
    void addImage(const scene::Image& image, const scene::Camera& camera,
                   const std::vector<ImageId>& candidateIds = {});
    const scene::Reconstruction& reconstruction() const noexcept;
    const scene::MatchGraph& matchGraph() const noexcept;
    const std::vector<scene::FeatureSet>& featureSets() const noexcept;
    const ba::BundleAdjustmentResult& lastBundleAdjustment() const noexcept;
    std::size_t rejectedTrackConnections() const noexcept;

private:
    void advanceReconstruction();
    feature::IFeatureExtractor& extractor_;
    matching::IFeatureMatcher& matcher_;
    ba::IBundleAdjuster* bundleAdjuster_;
    MapperConfig config_;
    std::vector<scene::FeatureSet> featureSets_;
    scene::MatchGraph graph_;
    scene::Reconstruction reconstruction_;
    ba::BundleAdjustmentResult lastBundleAdjustment_;
    std::size_t rejectedTrackConnections_ = 0;
};
}
