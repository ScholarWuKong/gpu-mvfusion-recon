#include "ba/bundle_adjuster.h"
#include <cmath>
#include <stdexcept>

namespace sfm::ba
{
void BundleAdjusterConfig::validate() const
{
    if (maxIterations_ <= 0 || !std::isfinite(huberDelta_) || huberDelta_ <= 0.0 ||
        !std::isfinite(initialDamping_) || initialDamping_ <= 0.0 ||
        !std::isfinite(relativeTolerance_) || relativeTolerance_ <= 0.0)
    {
        throw std::invalid_argument("Invalid bundle adjustment configuration.");
    }
}
}
