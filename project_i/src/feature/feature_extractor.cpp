#include "feature/feature_extractor.h"

#include <stdexcept>

namespace sfm::feature
{

IFeatureExtractor::~IFeatureExtractor() = default;

void FeatureExtractorConfig::validate() const
{
    if (nFeatures_ <= 0)
    {
        throw std::invalid_argument("nFeatures must be greater than zero.");
    }
    if (scaleFactor_ <= 1.0f)
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
    if (wtaK_ != 2 && wtaK_ != 3 && wtaK_ != 4)
    {
        throw std::invalid_argument("wtaK must be 2, 3, or 4.");
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
