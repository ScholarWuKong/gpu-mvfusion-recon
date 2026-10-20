#include "feature/feature_extractor.h"

#include <stdexcept>
#include <cmath>
#include <limits>

namespace sfm::feature
{

IFeatureExtractor::~IFeatureExtractor() = default;

void FeatureExtractorConfig::validate() const
{
    if (gpuGridColumns_ < 0 || gpuGridRows_ < 0 || gpuGridColumns_ > 64 || gpuGridRows_ > 64 ||
        ((gpuGridColumns_ == 0) != (gpuGridRows_ == 0)))
    {
        throw std::invalid_argument("GPU feature grid must be disabled or have 1..64 columns and rows.");
    }
    if (gpuGridColumns_ > 0 && nFeatures_ > std::numeric_limits<int>::max() / 2)
    {
        throw std::invalid_argument("Feature count is too large for GPU grid oversampling.");
    }
    if (nFeatures_ <= 0)
    {
        throw std::invalid_argument("nFeatures must be greater than zero.");
    }
    if (!std::isfinite(scaleFactor_) || scaleFactor_ <= 1.0f)
    {
        throw std::invalid_argument("scaleFactor must be greater than 1.0.");
    }
    if (nLevels_ <= 0)
    {
        throw std::invalid_argument("nLevels must be greater than zero.");
    }
    if (edgeThreshold_ < 0)
    {
        throw std::invalid_argument("edgeThreshold cannot be negative.");
    }
    if (firstLevel_ < 0 || firstLevel_ >= nLevels_)
    {
        throw std::invalid_argument("firstLevel must be in [0, nLevels).");
    }
    if (wtaK_ != 2)
    {
        throw std::invalid_argument("This pipeline requires ORB wtaK=2 for Hamming matching.");
    }
    if (patchSize_ <= 0)
    {
        throw std::invalid_argument("patchSize must be greater than zero.");
    }
    if (fastThreshold_ < 0)
    {
        throw std::invalid_argument("fastThreshold cannot be negative.");
    }
}

} // namespace sfm::feature
