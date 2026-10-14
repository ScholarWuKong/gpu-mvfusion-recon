#include "config.h"
#include <cmath>
#include <stdexcept>

namespace sfm
{
void MapperConfig::validate() const
{
    geometry_.validate();
    if (!std::isfinite(maxReprojectionError_) || maxReprojectionError_ <= 0.0 ||
        !std::isfinite(minTriangulationAngle_) || minTriangulationAngle_ <= 0.0 ||
        minTriangulationAngle_ >= 180.0 || minInitialPoints_ < 8 || minPnpInliers_ < 6)
    {
        throw std::invalid_argument("Invalid incremental mapper configuration.");
    }
}
}
