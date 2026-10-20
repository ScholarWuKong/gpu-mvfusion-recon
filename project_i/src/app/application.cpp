#include "app/application.h"
#include "app/application_options.h"
#include "app/integration_checks.h"
#include "ba/bundle_adjuster.h"
#include "feature/feature_extractor.h"
#include "io/image_io.h"
#include "io/reconstruction_io.h"
#include "sfm/incremental_mapper.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace sfm::app
{
namespace
{
void printUsage()
{
    std::cout << "Usage: project_i [options] <image1> <image2> [image3 ...]\n"
                 "  --feature-backend cpu|gpu  ORB backend (default cpu)\n"
                 "  --ba cpu|gpu|none          BA backend (default cpu)\n"
                 "  --focal pixels             Known focal length, fx=fy\n"
                 "  --intrinsics fx,fy,cx,cy    Fixed calibrated intrinsics, zero distortion\n"
                 "  --nfeatures count          Maximum ORB features (default 5000)\n"
                 "  --ratio value              Lowe ratio in (0,1), default 0.8\n"
                 "  --fast-threshold value     FAST threshold (default 20)\n"
                 "  --gpu-orb-blur on|off      Smooth GPU descriptor input (default off)\n"
                 "  --gpu-feature-grid CxR     Balanced GPU features, e.g. 8x6\n"
                 "  --robust-triangulation on|off  Recover tracks with outlying views\n"
                 "  --output directory         Output directory\n"
                 "  --self-test                Run integrated contract/geometry/GPU/BA tests\n"
                 "Images must be undistorted. Without calibration/focal, use 1.2*max(width,height).\n"
                 "Exit codes: 0 complete reconstruction; 1 error; 2 partial/no reconstruction.\n";
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
    const auto options = parseApplicationOptions(std::vector<std::string>(argv + 1, argv + argc));
    auto extractor = feature::createFeatureExtractor(options.featureConfig_);
    auto matcher = matching::createFeatureMatcher(options.matcherConfig_);
    auto adjuster = options.mapperConfig_.runBundleAdjustment_ ? ba::createBundleAdjuster(options.baConfig_) : nullptr;
    IncrementalMapper mapper(*extractor, *matcher, adjuster.get(), options.mapperConfig_);
    if (options.calibratedCamera_)
    {
        const auto& camera = *options.calibratedCamera_;
        std::cout << "Calibrated intrinsics: " << camera.fx_ << ',' << camera.fy_ << ','
                  << camera.cx_ << ',' << camera.cy_ << "; fixed, zero distortion.\n";
    }
    else if (options.focalLength_ == 0.0)
    {
        std::cout << "Intrinsics estimated: focal=1.2*max(image dimensions), principal point=image center.\n";
    }
    for (std::size_t index = 0; index < options.imagePaths_.size(); ++index)
    {
        const auto image = io::loadImage(options.imagePaths_[index], static_cast<ImageId>(index));
        const auto camera = cameraForImage(options, image.width_, image.height_);
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
