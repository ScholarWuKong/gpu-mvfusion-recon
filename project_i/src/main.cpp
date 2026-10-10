#include "feature/feature_extractor.h"
#include "io/image_io.h"

#include <exception>
#include <iomanip>
#include <iostream>
#include <string>

namespace
{

void printFeatureCount(
    const std::string& backendName,
    const sfm::scene::Image& image,
    const sfm::scene::FeatureSet& featureSet)
{
    std::cout << "  " << backendName
              << " | image " << image.id_
              << " | " << image.width_ << 'x' << image.height_
              << " | keypoints: " << std::setw(5) << featureSet.size()
              << " | descriptor size: " << featureSet.descriptorSize_
              << " bytes"
              << " | valid: "
              << (featureSet.hasValidDescriptors() ? "yes" : "no")
              << '\n';
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: project_i <image1_path> <image2_path>\n";
        return 1;
    }

    try
    {
        const sfm::scene::Image firstImage = sfm::io::loadImage(argv[1], 0);
        const sfm::scene::Image secondImage = sfm::io::loadImage(argv[2], 1);

        sfm::feature::FeatureExtractorConfig cpuConfig;
        cpuConfig.backend_ = sfm::feature::FeatureBackend::CPU;

        const auto cpuExtractor = sfm::feature::createFeatureExtractor(cpuConfig);
        const auto firstCpuFeatures = cpuExtractor->extract(firstImage);
        const auto secondCpuFeatures = cpuExtractor->extract(secondImage);

        std::cout << "CPU ORB\n";
        printFeatureCount("CPU", firstImage, firstCpuFeatures);
        printFeatureCount("CPU", secondImage, secondCpuFeatures);

        try
        {
            sfm::feature::FeatureExtractorConfig gpuConfig = cpuConfig;
            gpuConfig.backend_ = sfm::feature::FeatureBackend::GPU;

            const auto gpuExtractor = sfm::feature::createFeatureExtractor(gpuConfig);
            const auto firstGpuFeatures = gpuExtractor->extract(firstImage);
            const auto secondGpuFeatures = gpuExtractor->extract(secondImage);

            std::cout << "\nGPU ORB\n";
            printFeatureCount("GPU", firstImage, firstGpuFeatures);
            printFeatureCount("GPU", secondImage, secondGpuFeatures);
        }
        catch (const std::exception& exception)
        {
            std::cerr << "\nGPU ORB skipped: " << exception.what() << '\n';
            std::cerr << "CPU ORB completed successfully; check that your OpenCV build "
                         "contains cudafeatures2d and that a CUDA device is available.\n";
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }

    return 0;
}
