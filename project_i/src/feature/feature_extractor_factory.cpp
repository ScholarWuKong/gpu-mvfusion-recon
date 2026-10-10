#include "feature/feature_extractor.h"

#include "feature/cpu_feature_extractor.h"
#include "feature/gpu_feature_extractor.h"

#include <stdexcept>

namespace sfm::feature
{

std::unique_ptr<IFeatureExtractor> createFeatureExtractor(const FeatureExtractorConfig& config)
{
    config.validate();

    switch (config.backend_)
    {
    case FeatureBackend::CPU:
        return std::make_unique<CpuFeatureExtractor>(config);
    case FeatureBackend::GPU:
        return std::make_unique<GpuFeatureExtractor>(config);
    default:
        throw std::invalid_argument("Unsupported feature extractor backend.");
    }
}

} // namespace sfm::feature
