#pragma once

#include "scene/feature.h"
#include "scene/image.h"

#include <memory>

namespace sfm::feature
{

enum class FeatureBackend
{
    CPU,
    GPU
};

enum class ORBScoreType
{
    HARRIS,
    FAST
};

struct FeatureExtractorConfig
{
    FeatureBackend backend_ = FeatureBackend::CPU;

    int nFeatures_ = 50000;
    float scaleFactor_ = 1.2f;
    int nLevels_ = 8;
    int edgeThreshold_ = 31;
    int firstLevel_ = 0;
    int wtaK_ = 2;
    ORBScoreType scoreType_ = ORBScoreType::HARRIS;
    int patchSize_ = 31;
    int fastThreshold_ = 20;

    // GPU-only controls. A nonzero grid enables 2x candidate detection followed
    // by balanced spatial selection, retaining at most nFeatures rows.
    bool gpuBlurForDescriptor_ = false;
    int gpuGridColumns_ = 0;
    int gpuGridRows_ = 0;

    void validate() const;
};

class IFeatureExtractor
{
public:
    virtual ~IFeatureExtractor();

    virtual scene::FeatureSet extract(const scene::Image& image) = 0;
};

std::unique_ptr<IFeatureExtractor> createFeatureExtractor(const FeatureExtractorConfig& config);

// Compatibility with the factory name specified in the project task.
std::unique_ptr<IFeatureExtractor> CreateFeatureExtractor(const FeatureExtractorConfig& config);

} // namespace sfm::feature
