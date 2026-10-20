#pragma once

#include "ba/bundle_adjuster.h"
#include "config.h"
#include "feature/feature_extractor.h"
#include "matching/feature_matcher.h"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sfm::app
{
struct ApplicationOptions
{
    std::vector<std::string> imagePaths_;
    std::filesystem::path outputDirectory_ = "output/reconstruction";
    feature::FeatureExtractorConfig featureConfig_;
    ba::BundleAdjusterConfig baConfig_;
    matching::FeatureMatcherConfig matcherConfig_;
    MapperConfig mapperConfig_;
    double focalLength_ = 0.0;
    std::optional<scene::Camera> calibratedCamera_;
};

ApplicationOptions parseApplicationOptions(const std::vector<std::string>& arguments);
scene::Camera cameraForImage(const ApplicationOptions& options, int width, int height);
}
