#include "app/integration_checks.h"
#include "app/application_options.h"
#include "ba/bundle_adjuster.h"
#include "feature/feature_extractor.h"
#include "geometry/triangulation.h"
#include "geometry/two_view_geometry.h"
#include "matching/feature_matcher.h"
#include "sfm/incremental_mapper.h"
#include "sfm/observation_manager.h"
#include "sfm/track_builder.h"
#include "../src/ba/residual_internal.h"
#include "../src/feature/feature_selection.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace sfm::app
{
namespace
{
void require(const bool condition, const std::string& message)
{
    if (!condition)
    {
        throw std::runtime_error("Integration check failed: " + message);
    }
}

template <typename Function>
void requireThrows(Function function, const std::string& message)
{
    bool thrown = false;
    try
    {
        function();
    }
    catch (const std::exception&)
    {
        thrown = true;
    }
    require(thrown, message);
}

std::uint32_t randomValue(std::uint32_t& state)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

struct SyntheticScene
{
    scene::Camera camera_{800.0, 800.0, 399.5, 299.5};
    std::vector<Vector3> points_;
    std::vector<scene::CameraPose> poses_;
    std::vector<scene::FeatureSet> features_;
};

SyntheticScene makeSyntheticScene()
{
    SyntheticScene synthetic;
    scene::CameraPose first;
    scene::CameraPose second;
    scene::CameraPose third;
    second.translation_ = {-1.0, 0.0, 0.0};
    third.translation_ = {-0.4, -0.2, 0.1};
    const double angle = 0.05;
    third.rotation_ = {std::cos(angle), 0.0, std::sin(angle), 0.0, 1.0, 0.0,
                      -std::sin(angle), 0.0, std::cos(angle)};
    synthetic.poses_ = {first, second, third};
    std::uint32_t state = 123456789;
    constexpr std::size_t POINT_COUNT = 80;
    for (std::size_t index = 0; index < POINT_COUNT; ++index)
    {
        const double x = static_cast<double>(randomValue(state) % 10000) / 10000.0 * 3.6 - 1.8;
        const double y = static_cast<double>(randomValue(state) % 10000) / 10000.0 * 2.2 - 1.1;
        const double z = static_cast<double>(randomValue(state) % 10000) / 10000.0 * 5.0 + 5.0;
        synthetic.points_.push_back({x, y, z});
    }
    for (ImageId imageId = 0; imageId < 3; ++imageId)
    {
        scene::FeatureSet features;
        features.imageId_ = imageId;
        features.descriptorSize_ = 32;
        state = 987654321;
        for (const auto& point : synthetic.points_)
        {
            geometry::PixelPoint pixel;
            require(geometry::projectPoint(point, synthetic.camera_, synthetic.poses_[imageId], pixel), "synthetic projection");
            scene::Feature feature;
            feature.imageId_ = imageId;
            feature.keypoint_.x_ = static_cast<float>(pixel.x_);
            feature.keypoint_.y_ = static_cast<float>(pixel.y_);
            features.features_.push_back(feature);
            for (std::size_t byte = 0; byte < features.descriptorSize_; ++byte)
            {
                features.descriptors_.push_back(static_cast<std::uint8_t>(randomValue(state)));
            }
        }
        synthetic.features_.push_back(std::move(features));
    }
    return synthetic;
}

scene::Image makeImage(const ImageId imageId)
{
    scene::Image image;
    image.id_ = imageId;
    image.width_ = 800;
    image.height_ = 600;
    image.rowStrideBytes_ = 800;
    image.pixels_.resize(800 * 600);
    return image;
}

void checkApplicationOptions()
{
    const auto options = parseApplicationOptions({"--intrinsics", "3334.23,3341.06,2057.09,1548.96",
        "--feature-backend", "gpu", "--nfeatures", "20000", "--gpu-feature-grid", "8x6",
        "--gpu-orb-blur", "on", "--fast-threshold", "10", "--robust-triangulation", "on", "a.jpg", "b.jpg"});
    const auto camera = cameraForImage(options, 4096, 3072);
    require(std::abs(camera.fx_ - 3334.23) < 1e-9 && std::abs(camera.fy_ - 3341.06) < 1e-9 &&
            std::abs(camera.cx_ - 2057.09) < 1e-9 && std::abs(camera.cy_ - 1548.96) < 1e-9,
            "full calibration preserves unequal focal lengths and noncentral principal point");
    require(options.featureConfig_.gpuGridColumns_ == 8 && options.featureConfig_.gpuGridRows_ == 6 &&
            options.featureConfig_.gpuBlurForDescriptor_ && options.featureConfig_.fastThreshold_ == 10 &&
            options.mapperConfig_.robustTriangulation_, "GPU coverage options parsed");
    const auto legacy = cameraForImage(parseApplicationOptions({"--focal", "800", "a", "b"}), 800, 600);
    require(legacy.fx_ == 800 && legacy.fy_ == 800 && legacy.cx_ == 399.5, "legacy focal option preserved");
    for (const auto& calibration : {"1,2,3", "1,2,3,4,5", "1,2,3,4,", "0,2,3,4", "nan,2,3,4"})
    {
        requireThrows([&]
        {
            parseApplicationOptions({"--intrinsics", calibration, "a", "b"});
        }, "malformed calibration rejected before GPU initialization");
    }
    requireThrows([]
    {
        parseApplicationOptions({"--intrinsics", "1,2,3,4", "--focal", "10", "a", "b"});
    }, "ambiguous focal and full intrinsics rejected");
    requireThrows([]
    {
        parseApplicationOptions({"--gpu-feature-grid", "0x6", "a", "b"});
    }, "invalid spatial grid rejected");
    std::cout << "PASS calibrated camera selection, legacy options and invalid parameters\n";
}

void checkSpatialFeatureSelection()
{
    scene::FeatureSet candidates;
    candidates.imageId_ = 89;
    candidates.descriptorSize_ = 32;
    for (int index = 0; index < 12; ++index)
    {
        scene::Feature feature;
        feature.imageId_ = candidates.imageId_;
        feature.keypoint_.x_ = index < 10 ? 10.0f + index : index == 10 ? 150.0f : 250.0f;
        feature.keypoint_.y_ = 20.0f;
        feature.keypoint_.response_ = index < 10 ? 100.0f - index : 1.0f;
        feature.keypoint_.classId_ = index;
        candidates.features_.push_back(feature);
        for (int byte = 0; byte < 32; ++byte)
        {
            candidates.descriptors_.push_back(static_cast<std::uint8_t>(index + byte));
        }
    }
    const auto selected = feature::detail::distributeFeatures(candidates, 300, 100, 6, 3, 1);
    require(selected.size() == 6 && selected.hasValidDescriptors() && selected.imageId_ == 89,
            "spatial feature cap and data contract");
    bool middlePresent = false;
    bool rightPresent = false;
    for (std::size_t row = 0; row < selected.size(); ++row)
    {
        const auto source = selected.features_[row].keypoint_.classId_;
        middlePresent = middlePresent || source == 10;
        rightPresent = rightPresent || source == 11;
        for (std::size_t byte = 0; byte < 32; ++byte)
        {
            require(selected.descriptors_[row * 32 + byte] == static_cast<std::uint8_t>(source + byte),
                    "descriptor rows follow spatially selected keypoints exactly");
        }
    }
    require(middlePresent && rightPresent, "weakly textured occupied cells retain representatives");
    scene::FeatureSet empty;
    empty.imageId_ = 91;
    empty.descriptorSize_ = 32;
    const auto selectedEmpty = feature::detail::distributeFeatures(empty, 300, 100, 6, 3, 1);
    require(selectedEmpty.empty() && selectedEmpty.imageId_ == 91, "empty spatial selection retains image ID");
    std::cout << "PASS balanced spatial feature coverage and exact descriptor-row correspondence\n";
}

void checkRobustTriangulation(const SyntheticScene& synthetic)
{
    std::vector<geometry::TriangulationView> views;
    scene::Reconstruction reconstruction;
    scene::Track track;
    track.id_ = 0;
    for (ImageId imageId = 0; imageId < 4; ++imageId)
    {
        const auto pose = imageId < 3 ? synthetic.poses_[imageId] : geometry::CameraPose{};
        geometry::PixelPoint pixel;
        geometry::projectPoint(synthetic.points_[0], synthetic.camera_, pose, pixel);
        if (imageId == 3)
        {
            pixel.x_ += 45.0;
            pixel.y_ -= 35.0;
        }
        views.push_back({synthetic.camera_, pose, pixel});
        reconstruction.images_[imageId] = {imageId, 800, 600, 1, synthetic.camera_, pose, true};
        track.observations_.push_back({imageId, 0, pixel.x_, pixel.y_});
    }
    reconstruction.tracks_.push_back(track);
    Vector3 position;
    require(!geometry::triangulatePoint(views, position, 2.0, 1.0), "outlying view rejects all-view DLT fixture");
    std::vector<std::size_t> inliers;
    require(geometry::triangulatePointRobust(views, position, inliers, 2.0, 1.0) &&
            inliers == std::vector<std::size_t>({0, 1, 2}), "robust triangulation recovers the three correct views");
    for (int axis = 0; axis < 3; ++axis)
    {
        require(std::abs(position[axis] - synthetic.points_[0][axis]) < 1e-8, "robust triangulation world-point accuracy");
    }
    MapperConfig config;
    config.robustTriangulation_ = true;
    const auto added = ObservationManager{}.triangulateTracks(reconstruction, config);
    require(added == 1 && reconstruction.tracks_[0].pointId_ == 0 &&
            !reconstruction.tracks_[0].observations_[3].enabled_ &&
            reconstruction.tracks_[0].observations_[0].enabled_, "recovered Track retains source observation indices");
    reconstruction.validate();
    std::cout << "PASS robust Track recovery despite one bad observation\n";
}

void checkContractsAndExtraction()
{
    scene::FeatureSet empty;
    empty.imageId_ = 17;
    require(empty.hasValidDescriptors() && empty.empty() && empty.imageId_ == 17, "empty features retain ID");
    const auto normalized = scene::PairwiseMatch::fromOrderedMatches(9, 2, {{3, 7, 4.0f}});
    require(normalized.pair_.imageId1_ == 2 && normalized.matches_[0].featureIndex1_ == 7 &&
            normalized.matches_[0].featureIndex2_ == 3, "pair normalization also swaps feature indices");
    requireThrows([]
    {
        scene::ImagePair invalid(2, 2);
    }, "self pairs rejected");
    for (const auto backend : {feature::FeatureBackend::CPU, feature::FeatureBackend::GPU})
    {
        feature::FeatureExtractorConfig config;
        config.backend_ = backend;
        config.nFeatures_ = 200;
        if (backend == feature::FeatureBackend::GPU)
        {
            config.gpuGridColumns_ = 4;
            config.gpuGridRows_ = 4;
            config.gpuBlurForDescriptor_ = true;
        }
        auto extractor = feature::CreateFeatureExtractor(config);
        const auto blank = extractor->extract(makeImage(17));
        require(blank.empty() && blank.imageId_ == 17 && blank.hasValidDescriptors(), "zero keypoint ORB contract");
        requireThrows([&]
        {
            extractor->extract(scene::Image{});
        }, "empty image rejected");
    }
    std::cout << "PASS data contracts and CPU/GPU ORB empty-input handling\n";
}

void checkGpuMatching(const SyntheticScene& synthetic)
{
    auto matcher = matching::CreateFeatureMatcher({});
    const auto all = matcher->matchAllPairs(synthetic.features_);
    require(all.pairs().size() == 3, "multi-image all-pairs matching");
    for (const auto& edge : all.pairs())
    {
        require(edge.second.attempted_ && edge.second.matches_.size() == synthetic.points_.size(), "exact CUDA matching count");
        for (const auto& match : edge.second.matches_)
        {
            require(match.featureIndex1_ == match.featureIndex2_ && match.distance_ == 0.0f, "correct CUDA descriptor indices");
        }
    }
    const auto subset = matcher->matchPairs(synthetic.features_, {{2, 0}, {0, 2}});
    require(subset.pairs().size() == 1 && subset.contains({0, 2}), "requested pairs normalized and deduplicated");
    auto changed = synthetic.features_;
    std::swap_ranges(changed[1].descriptors_.begin(), changed[1].descriptors_.begin() + 32,
                     changed[1].descriptors_.begin() + 32);
    const auto refreshed = matcher->matchPairs(changed, {{0, 1}});
    const auto& refreshedMatches = refreshed.pairs().begin()->second.matches_;
    require(refreshedMatches[0].featureIndex2_ == 1 && refreshedMatches[1].featureIndex2_ == 0,
            "same-ID descriptor mutation invalidates GPU upload");
    matcher->clearCache();
    require(matcher->matchPairs(synthetic.features_, {{0, 1}}).pairs().begin()->second.matches_[0].featureIndex2_ == 0,
            "clearCache permits correct reupload");
    auto emptyInputs = synthetic.features_;
    emptyInputs[1].features_.clear();
    emptyInputs[1].descriptors_.clear();
    const auto emptyResult = matcher->matchPairs(emptyInputs, {{0, 1}});
    require(emptyResult.pairs().begin()->second.attempted_ && emptyResult.pairs().begin()->second.matches_.empty(),
            "empty descriptors still record attempted pair");
    emptyInputs[1].features_ = {synthetic.features_[1].features_[0]};
    emptyInputs[1].descriptors_.assign(synthetic.features_[1].descriptors_.begin(), synthetic.features_[1].descriptors_.begin() + 32);
    require(matcher->matchPairs(emptyInputs, {{0, 1}}).pairs().begin()->second.matches_.empty(), "KNN needs two train descriptors");
    requireThrows([&]
    {
        auto duplicate = synthetic.features_;
        duplicate.push_back(duplicate[0]);
        matcher->matchAllPairs(duplicate);
    }, "duplicate image IDs rejected");
    requireThrows([&]
    {
        matcher->matchPairs(synthetic.features_, {{0, 100}});
    }, "unknown candidate image rejected");
    requireThrows([&]
    {
        auto invalid = synthetic.features_;
        invalid[1].descriptorSize_ = 16;
        matcher->matchPairs(invalid, {{0, 1}});
    }, "inconsistent descriptor dimensions rejected");
    requireThrows([&]
    {
        auto invalid = synthetic.features_;
        invalid[0].features_[0].imageId_ = 100;
        matcher->matchPairs(invalid, {{0, 1}});
    }, "feature image ID mismatch rejected");
    std::cout << "PASS CUDA multi-image matching, requested pairs, cache refresh and invalid inputs\n";
}

void checkGeometry(const SyntheticScene& synthetic)
{
    std::vector<geometry::PixelPoint> first;
    std::vector<geometry::PixelPoint> second;
    for (std::size_t index = 0; index < synthetic.points_.size(); ++index)
    {
        first.push_back({synthetic.features_[0].features_[index].keypoint_.x_, synthetic.features_[0].features_[index].keypoint_.y_});
        second.push_back({synthetic.features_[1].features_[index].keypoint_.x_, synthetic.features_[1].features_[index].keypoint_.y_});
    }
    // Introduce mismatches: returned inliers must continue to index the original inputs.
    for (std::size_t index = 70; index < 80; ++index)
    {
        second[index] = second[index - 50];
    }
    const auto result = geometry::verifyTwoView(first, second, synthetic.camera_, synthetic.camera_, {});
    require(result.verified_ && result.inlierIndices_.size() >= 65, "RANSAC/essential/cheirality chain");
    for (const auto index : result.inlierIndices_)
    {
        require(index < synthetic.points_.size(), "inlier indices reference original correspondences");
    }
    geometry::PixelPoint pixel0;
    geometry::PixelPoint pixel1;
    geometry::projectPoint(synthetic.points_[0], synthetic.camera_, synthetic.poses_[0], pixel0);
    geometry::projectPoint(synthetic.points_[0], synthetic.camera_, synthetic.poses_[1], pixel1);
    Vector3 point;
    require(geometry::triangulatePoint({{synthetic.camera_, synthetic.poses_[0], pixel0},
                                      {synthetic.camera_, synthetic.poses_[1], pixel1}}, point, 1.0, 1.0), "triangulation");
    for (int axis = 0; axis < 3; ++axis)
    {
        require(std::abs(point[axis] - synthetic.points_[0][axis]) < 1e-8, "triangulation numerical accuracy");
    }
    require(!geometry::triangulatePoint({{synthetic.camera_, synthetic.poses_[0], pixel0},
                                        {synthetic.camera_, synthetic.poses_[0], pixel0}}, point, 1.0, 1.0),
            "zero baseline is not accepted as reconstruction");
    std::cout << "PASS geometric inlier mapping, accurate triangulation and zero-baseline rejection\n";
}

scene::MatchGraph verifiedEdge(const ImageId first, const ImageId second,
                               std::vector<scene::FeatureMatch> matches)
{
    auto pair = scene::PairwiseMatch::fromOrderedMatches(first, second, std::move(matches));
    pair.geometryStatus_ = scene::GeometryStatus::VERIFIED;
    for (std::size_t index = 0; index < pair.matches_.size(); ++index)
    {
        pair.inlierMatchIndices_.push_back(index);
    }
    scene::MatchGraph graph;
    graph.set(std::move(pair));
    return graph;
}

void checkTracks(const SyntheticScene& synthetic)
{
    scene::Reconstruction reconstruction;
    const auto rejected = TrackBuilder{}.extend(verifiedEdge(0, 1, {{0, 0, 0.0f}, {1, 0, 1.0f}}),
                                                 synthetic.features_, reconstruction);
    require(rejected == 1 && reconstruction.tracks_.size() == 1, "conflicting same-image track connections rejected");
    TrackBuilder{}.extend(verifiedEdge(1, 2, {{0, 0, 0.0f}}), synthetic.features_, reconstruction);
    require(reconstruction.tracks_[0].observations_.size() == 3, "incremental track extension");
    TrackBuilder{}.extend(verifiedEdge(1, 2, {{0, 0, 0.0f}, {0, 0, 0.0f}}), synthetic.features_, reconstruction);
    require(reconstruction.tracks_[0].observations_.size() == 3, "duplicate matches do not duplicate observations");
    requireThrows([&]
    {
        TrackBuilder{}.extend(verifiedEdge(0, 1, {{100000, 0, 0.0f}}), synthetic.features_, reconstruction);
    }, "invalid match indices rejected");
    auto fourViews = synthetic.features_;
    auto fourth = fourViews[0];
    fourth.imageId_ = 3;
    for (auto& feature : fourth.features_)
    {
        feature.imageId_ = 3;
    }
    fourViews.push_back(std::move(fourth));
    scene::Reconstruction merged;
    for (ImageId imageId = 0; imageId < 4; ++imageId)
    {
        merged.images_[imageId] = {imageId, 800, 600, synthetic.points_.size(), synthetic.camera_, {}, true};
    }
    TrackBuilder{}.extend(verifiedEdge(0, 1, {{0, 0, 0.0f}}), fourViews, merged);
    TrackBuilder{}.extend(verifiedEdge(2, 3, {{1, 1, 0.0f}}), fourViews, merged);
    merged.tracks_[1].pointId_ = 0;
    merged.points_.push_back({0, 1, synthetic.points_[0]});
    TrackBuilder{}.extend(verifiedEdge(1, 2, {{0, 1, 0.0f}}), fourViews, merged);
    require(merged.tracks_.size() == 1 && merged.tracks_[0].observations_.size() == 4 &&
            merged.points_[0].trackId_ == 0 && merged.tracks_[0].pointId_ == 0,
            "track merge and compaction preserve a pre-existing 3D point");
    merged.validate();
    std::cout << "PASS track conflict, duplicate-edge and invalid-index handling\n";
}

scene::Reconstruction makeBaFixture(const SyntheticScene& synthetic)
{
    scene::Reconstruction reconstruction;
    reconstruction.fixedImageIds_ = {0, 1};
    for (ImageId imageId = 0; imageId < 3; ++imageId)
    {
        reconstruction.images_[imageId] = {imageId, 800, 600, synthetic.points_.size(),
                                           synthetic.camera_, synthetic.poses_[imageId], true};
    }
    for (std::size_t index = 0; index < synthetic.points_.size(); ++index)
    {
        scene::Track track;
        track.id_ = index;
        track.pointId_ = index;
        for (ImageId imageId = 0; imageId < 3; ++imageId)
        {
            const auto& keypoint = synthetic.features_[imageId].features_[index].keypoint_;
            track.observations_.push_back({imageId, index, keypoint.x_, keypoint.y_});
        }
        reconstruction.tracks_.push_back(std::move(track));
        auto position = synthetic.points_[index];
        position[0] += 0.03;
        position[1] -= 0.02;
        position[2] += 0.04;
        reconstruction.points_.push_back({index, index, position});
    }
    reconstruction.images_[2].pose_.translation_[0] += 0.04;
    reconstruction.images_[2].pose_.translation_[1] -= 0.03;
    const double perturbedAngle = 0.065;
    reconstruction.images_[2].pose_.rotation_ = {std::cos(perturbedAngle), 0.0, std::sin(perturbedAngle),
        0.0, 1.0, 0.0, -std::sin(perturbedAngle), 0.0, std::cos(perturbedAngle)};
    return reconstruction;
}

void checkJacobians()
{
    ba::detail::ResidualInput input{};
    input.rotation_[0] = input.rotation_[4] = input.rotation_[8] = 1.0;
    input.translation_[0] = -0.5;
    input.point_[0] = 1.0;
    input.point_[1] = -0.3;
    input.point_[2] = 6.0;
    input.intrinsics_[0] = input.intrinsics_[1] = 800.0;
    const auto analytic = ba::detail::computeResidual(input);
    constexpr double STEP = 1e-6;
    for (int parameter = 0; parameter < 9; ++parameter)
    {
        auto plus = input;
        auto minus = input;
        if (parameter < 3)
        {
            plus.point_[parameter] += STEP;
            minus.point_[parameter] -= STEP;
        }
        else if (parameter >= 6)
        {
            plus.translation_[parameter - 6] += STEP;
            minus.translation_[parameter - 6] -= STEP;
        }
        else
        {
            const int axis = parameter - 3;
            const int first = (axis + 1) % 3;
            const int second = (axis + 2) % 3;
            plus.rotation_[3 * first + second] = -std::sin(STEP);
            plus.rotation_[3 * second + first] = std::sin(STEP);
            minus.rotation_[3 * first + second] = std::sin(STEP);
            minus.rotation_[3 * second + first] = -std::sin(STEP);
            plus.rotation_[3 * first + first] = plus.rotation_[3 * second + second] = std::cos(STEP);
            minus.rotation_[3 * first + first] = minus.rotation_[3 * second + second] = std::cos(STEP);
            plus.translation_[first] = std::cos(STEP) * input.translation_[first] - std::sin(STEP) * input.translation_[second];
            plus.translation_[second] = std::sin(STEP) * input.translation_[first] + std::cos(STEP) * input.translation_[second];
            minus.translation_[first] = std::cos(STEP) * input.translation_[first] + std::sin(STEP) * input.translation_[second];
            minus.translation_[second] = -std::sin(STEP) * input.translation_[first] + std::cos(STEP) * input.translation_[second];
        }
        const auto positive = ba::detail::computeResidual(plus);
        const auto negative = ba::detail::computeResidual(minus);
        for (int row = 0; row < 2; ++row)
        {
            const double numerical = (positive.residual_[row] - negative.residual_[row]) / (2.0 * STEP);
            const double expected = parameter < 3 ? analytic.pointJacobian_[3 * row + parameter] :
                                                    analytic.cameraJacobian_[6 * row + parameter - 3];
            require(std::abs(numerical - expected) < 1e-5, "BA analytical Jacobian versus central differences");
        }
    }
}

void checkBundleAdjustment(const SyntheticScene& synthetic)
{
    checkJacobians();
    // With only the two fixed seed cameras, BA optimizes points only. This must
    // not perform matrix arithmetic on the empty camera Schur system.
    auto twoView = makeBaFixture(synthetic);
    twoView.images_.erase(2);
    for (auto& track : twoView.tracks_)
    {
        track.observations_.pop_back();
    }
    const auto twoViewResult = ba::createBundleAdjuster({})->optimize(twoView);
    require(twoViewResult.attempted_ && twoViewResult.observationCount_ == 160 &&
            twoViewResult.finalCost_ < twoViewResult.initialCost_ * 0.01,
            "two fixed cameras support point-only BA without an empty matrix error");
    std::cout << "PASS two-view point-only BA cost " << twoViewResult.initialCost_
              << " -> " << twoViewResult.finalCost_ << '\n';
    auto filtered = makeBaFixture(synthetic);
    filtered.tracks_[0].observations_[0].enabled_ = false;
    filtered.tracks_[0].observations_[1].enabled_ = false;
    ObservationManager{}.filterObservations(filtered, 1000000.0);
    require(filtered.points_.size() == 79 && filtered.tracks_[0].pointId_ == INVALID_POINT_ID &&
            filtered.points_[0].trackId_ == 1 && filtered.tracks_[1].pointId_ == 0,
            "unsupported point removal repairs compact point IDs");
    filtered.validate();
    requireThrows([&]
    {
        auto invalid = filtered;
        auto duplicate = invalid.tracks_[1];
        duplicate.id_ = invalid.tracks_.size();
        duplicate.pointId_ = INVALID_POINT_ID;
        invalid.tracks_.push_back(duplicate);
        invalid.validate();
    }, "reconstruction rejects a feature shared by different tracks");
    double cpuCost = 0.0;
    for (const auto backend : {ba::BundleAdjustmentBackend::CPU, ba::BundleAdjustmentBackend::GPU})
    {
        auto reconstruction = makeBaFixture(synthetic);
        const auto fixedFirst = reconstruction.images_[0].pose_;
        const auto fixedSecond = reconstruction.images_[1].pose_;
        ba::BundleAdjusterConfig config;
        config.backend_ = backend;
        auto adjuster = ba::createBundleAdjuster(config);
        const auto result = adjuster->optimize(reconstruction);
        require(result.attempted_ && result.observationCount_ == 240 && result.finalCost_ < result.initialCost_ * 0.01,
                "joint BA reduces reprojection cost");
        require(reconstruction.images_[0].pose_.rotation_ == fixedFirst.rotation_ &&
                reconstruction.images_[0].pose_.translation_ == fixedFirst.translation_ &&
                reconstruction.images_[1].pose_.rotation_ == fixedSecond.rotation_ &&
                reconstruction.images_[1].pose_.translation_ == fixedSecond.translation_, "BA gauge cameras stay fixed");
        if (backend == ba::BundleAdjustmentBackend::CPU)
        {
            cpuCost = result.finalCost_;
        }
        else
        {
            require(std::abs(result.finalCost_ - cpuCost) < 1e-5, "CPU/CUDA BA numerical agreement");
        }
        std::cout << "PASS " << (backend == ba::BundleAdjustmentBackend::CPU ? "CPU" : "CUDA")
                  << " BA cost " << result.initialCost_ << " -> " << result.finalCost_ << '\n';
    }
}

// Only replaces feature detection with deterministic geometric fixtures. Matching,
// geometry, tracks, PnP, triangulation and BA use the production implementations.
class FixtureExtractor final : public feature::IFeatureExtractor
{
public:
    explicit FixtureExtractor(const SyntheticScene& synthetic) : synthetic_(synthetic)
    {
    }

    scene::FeatureSet extract(const scene::Image& image) override
    {
        return synthetic_.features_.at(image.id_);
    }

private:
    const SyntheticScene& synthetic_;
};

class RecordingMatcher final : public matching::IFeatureMatcher
{
public:
    RecordingMatcher() : matcher_(matching::createFeatureMatcher({}))
    {
    }

    scene::MatchGraph matchPairs(const std::vector<scene::FeatureSet>& features,
                                 const std::vector<scene::ImagePair>& pairs) override
    {
        if (omitNextResult_)
        {
            omitNextResult_ = false;
            return {};
        }
        requests_.push_back(pairs);
        return matcher_->matchPairs(features, pairs);
    }

    void clearCache() override
    {
        matcher_->clearCache();
    }

    std::vector<std::vector<scene::ImagePair>> requests_;
    bool omitNextResult_ = false;

private:
    std::unique_ptr<matching::IFeatureMatcher> matcher_;
};

void checkIncrementalMapping(const SyntheticScene& synthetic)
{
    FixtureExtractor extractor(synthetic);
    RecordingMatcher matcher;
    auto adjuster = ba::createBundleAdjuster({});
    IncrementalMapper mapper(extractor, matcher, adjuster.get(), {});
    mapper.addImage(makeImage(0), synthetic.camera_);
    mapper.addImage(makeImage(1), synthetic.camera_);
    require(mapper.reconstruction().registeredImageCount() == 2 && mapper.reconstruction().points_.size() >= 70,
            "two-view initialization through production pipeline");
    mapper.addImage(makeImage(2), synthetic.camera_, {1});
    require(mapper.reconstruction().registeredImageCount() == 3 && mapper.reconstruction().points_.size() >= 70,
            "third image registered by PnP");
    require(mapper.matchGraph().pairs().size() == 2 && !mapper.matchGraph().contains({0, 2}), "candidate-only incremental matching");
    require(matcher.requests_.size() == 2 && matcher.requests_[0].size() == 1 && matcher.requests_[1].size() == 1 &&
            matcher.requests_[1][0].imageId1_ == 1 && matcher.requests_[1][0].imageId2_ == 2,
            "old pair is not recomputed when a new image arrives");
    mapper.reconstruction().validate();
    require(mapper.lastBundleAdjustment().attempted_, "incremental BA invoked");
    requireThrows([&]
    {
        mapper.addImage(makeImage(2), synthetic.camera_);
    }, "mapper rejects duplicate image IDs");
    RecordingMatcher failedMatcher;
    MapperConfig noBa;
    noBa.runBundleAdjustment_ = false;
    IncrementalMapper retryMapper(extractor, failedMatcher, nullptr, noBa);
    retryMapper.addImage(makeImage(0), synthetic.camera_);
    failedMatcher.omitNextResult_ = true;
    requireThrows([&]
    {
        retryMapper.addImage(makeImage(1), synthetic.camera_);
    }, "mapper rejects incomplete matcher output");
    require(retryMapper.featureSets().size() == 1 && retryMapper.reconstruction().images_.size() == 1 &&
            retryMapper.matchGraph().pairs().empty(), "failed match request rolls back appended features");
    retryMapper.addImage(makeImage(1), synthetic.camera_);
    require(retryMapper.reconstruction().registeredImageCount() == 2, "image can be retried after matching failure");
    std::cout << "PASS three-view incremental pipeline: " << mapper.reconstruction().registeredImageCount()
              << " cameras, " << mapper.reconstruction().points_.size() << " points; old pairs retained\n";
}
}

int runIntegrationChecks()
{
    const auto synthetic = makeSyntheticScene();
    checkApplicationOptions();
    checkSpatialFeatureSelection();
    checkContractsAndExtraction();
    checkGpuMatching(synthetic);
    checkGeometry(synthetic);
    checkRobustTriangulation(synthetic);
    checkTracks(synthetic);
    checkBundleAdjustment(synthetic);
    checkIncrementalMapping(synthetic);
    std::cout << "All integration checks passed.\n";
    return 0;
}
}
