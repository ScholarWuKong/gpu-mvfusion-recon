#include "app/application.h"
#include "app/integration_checks.h"
#include "ba/bundle_adjuster.h"
#include "feature/feature_extractor.h"
#include "io/image_io.h"
#include "io/reconstruction_io.h"
#include "sfm/incremental_mapper.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace sfm::app
{
namespace
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
};

void printUsage()
{
    std::cout << "Usage: project_i [options] <image1> <image2> [image3 ...]\n"
                 "  --feature-backend cpu|gpu  ORB backend (default cpu)\n"
                 "  --ba cpu|gpu|none          BA backend (default cpu)\n"
                 "  --focal pixels             Known focal length, fx=fy\n"
                 "  --nfeatures count          Maximum ORB features (default 5000)\n"
                 "  --ratio value              Lowe ratio in (0,1), default 0.8\n"
                 "  --output directory         Output directory\n"
                 "  --self-test                Run integrated contract/geometry/GPU/BA tests\n"
                 "Images must be undistorted. Without --focal, use 1.2*max(width,height).\n"
                 "Exit codes: 0 complete reconstruction; 1 error; 2 partial/no reconstruction.\n";
}

double parseNumber(const std::string& value)
{
    std::size_t parsed = 0;
    const double number = std::stod(value, &parsed);
    if (parsed != value.size() || !std::isfinite(number))
    {
        throw std::invalid_argument("Invalid numeric option: " + value);
    }
    return number;
}

ApplicationOptions parseOptions(const int argc, char* argv[])
{
    ApplicationOptions options;
    options.featureConfig_.nFeatures_ = 5000;
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument.rfind("--", 0) != 0)
        {
            options.imagePaths_.push_back(argument);
            continue;
        }
        if (index + 1 >= argc)
        {
            throw std::invalid_argument("Missing value for " + argument);
        }
        const std::string value = argv[++index];
        if (argument == "--feature-backend")
        {
            if (value != "cpu" && value != "gpu")
            {
                throw std::invalid_argument("Feature backend must be cpu or gpu.");
            }
            options.featureConfig_.backend_ = value == "cpu" ? feature::FeatureBackend::CPU : feature::FeatureBackend::GPU;
        }
        else if (argument == "--ba")
        {
            if (value != "cpu" && value != "gpu" && value != "none")
            {
                throw std::invalid_argument("BA backend must be cpu, gpu, or none.");
            }
            options.mapperConfig_.runBundleAdjustment_ = value != "none";
            options.baConfig_.backend_ = value == "gpu" ? ba::BundleAdjustmentBackend::GPU : ba::BundleAdjustmentBackend::CPU;
        }
        else if (argument == "--focal")
        {
            options.focalLength_ = parseNumber(value);
            if (options.focalLength_ <= 0.0)
            {
                throw std::invalid_argument("Focal length must be positive.");
            }
        }
        else if (argument == "--nfeatures")
        {
            const double count = parseNumber(value);
            if (count < 1.0 || count > std::numeric_limits<int>::max() || std::floor(count) != count)
            {
                throw std::invalid_argument("Feature count must be a positive integer.");
            }
            options.featureConfig_.nFeatures_ = static_cast<int>(count);
        }
        else if (argument == "--ratio")
        {
            options.matcherConfig_.ratioThreshold_ = static_cast<float>(parseNumber(value));
        }
        else if (argument == "--output")
        {
            options.outputDirectory_ = value;
        }
        else
        {
            throw std::invalid_argument("Unknown option: " + argument);
        }
    }
    if (options.imagePaths_.size() < 2 || options.imagePaths_.size() > std::numeric_limits<ImageId>::max())
    {
        throw std::invalid_argument("Supply at least two input images.");
    }
    options.featureConfig_.validate();
    options.matcherConfig_.validate();
    options.baConfig_.validate();
    options.mapperConfig_.validate();
    return options;
}
}

int runApplication(const int argc, char* argv[])
{
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--help"))
    {
        printUsage();
        return argc == 1 ? 1 : 0;
    }
    if (argc == 2 && std::string(argv[1]) == "--self-test")
    {
#ifdef SFM_ENABLE_INTEGRATION_CHECKS
        return runIntegrationChecks();
#else
        throw std::runtime_error("Integration checks were disabled at configure time.");
#endif
    }
    const auto options = parseOptions(argc, argv);
    auto extractor = feature::createFeatureExtractor(options.featureConfig_);
    auto matcher = matching::createFeatureMatcher(options.matcherConfig_);
    auto adjuster = options.mapperConfig_.runBundleAdjustment_ ? ba::createBundleAdjuster(options.baConfig_) : nullptr;
    IncrementalMapper mapper(*extractor, *matcher, adjuster.get(), options.mapperConfig_);
    if (options.focalLength_ == 0.0)
    {
        std::cout << "Intrinsics estimated: focal=1.2*max(image dimensions), principal point=image center.\n";
    }
    for (std::size_t index = 0; index < options.imagePaths_.size(); ++index)
    {
        const auto image = io::loadImage(options.imagePaths_[index], static_cast<ImageId>(index));
        const double focal = options.focalLength_ > 0.0 ? options.focalLength_ : 1.2 * std::max(image.width_, image.height_);
        const scene::Camera camera{focal, focal, 0.5 * (image.width_ - 1), 0.5 * (image.height_ - 1)};
        mapper.addImage(image, camera);
        std::cout << "Image " << image.id_ << ": " << mapper.featureSets().back().size()
                  << " features; registered " << mapper.reconstruction().registeredImageCount()
                  << '/' << mapper.reconstruction().images_.size() << "; points "
                  << mapper.reconstruction().points_.size() << '\n';
    }
    for (const auto& edge : mapper.matchGraph().pairs())
    {
        const auto& pair = edge.second;
        std::cout << "Pair " << pair.pair_.imageId1_ << '-' << pair.pair_.imageId2_ << ": "
                  << pair.matches_.size() << " descriptor matches; " << pair.inlierMatchIndices_.size()
                  << " geometric inliers; "
                  << (pair.geometryStatus_ == scene::GeometryStatus::VERIFIED ? "verified" : "rejected") << '\n';
    }
    const auto& baResult = mapper.lastBundleAdjustment();
    if (baResult.attempted_)
    {
        std::cout << "BA: " << baResult.observationCount_ << " observations, cost " << baResult.initialCost_
                  << " -> " << baResult.finalCost_ << ", iterations " << baResult.iterations_ << '\n';
    }
    std::cout << "Tracks: " << mapper.reconstruction().tracks_.size() << "; rejected conflicting links: "
              << mapper.rejectedTrackConnections() << '\n';
    io::writeReconstruction(mapper.reconstruction(), mapper.matchGraph(), options.outputDirectory_);
    std::ofstream manifest(options.outputDirectory_ / "inputs.txt");
    manifest.exceptions(std::ios::failbit | std::ios::badbit);
    for (std::size_t index = 0; index < options.imagePaths_.size(); ++index)
    {
        manifest << index << '\t' << std::filesystem::absolute(options.imagePaths_[index]).string() << '\n';
    }
    manifest.close();
    const bool complete = mapper.reconstruction().registeredImageCount() == options.imagePaths_.size() &&
                          mapper.reconstruction().points_.size() >= options.mapperConfig_.minInitialPoints_;
    std::cout << (complete ? "Reconstruction completed" : "Partial/no reconstruction: insufficient verified geometry or PnP support")
              << "; output: " << std::filesystem::absolute(options.outputDirectory_).string() << '\n';
    return complete ? 0 : 2;
}
}
