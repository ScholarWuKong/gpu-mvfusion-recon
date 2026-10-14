#include "geometry/two_view_geometry.h"
#include "geometry/essential.h"
#include "geometry/fundamental.h"
#include <cmath>
#include <stdexcept>

namespace sfm::geometry
{
void TwoViewConfig::validate() const
{
    if (!std::isfinite(ransacThreshold_) || ransacThreshold_ <= 0.0 ||
        !std::isfinite(confidence_) || confidence_ <= 0.0 || confidence_ >= 1.0 || minInliers_ < 8)
    {
        throw std::invalid_argument("Invalid two-view geometry configuration.");
    }
}

TwoViewResult verifyTwoView(const std::vector<PixelPoint>& first,
                            const std::vector<PixelPoint>& second,
                            const CameraIntrinsics& firstCamera,
                            const CameraIntrinsics& secondCamera,
                            const TwoViewConfig& config)
{
    config.validate();
    TwoViewResult result;
    const auto fundamental = estimateFundamental(first, second, config.ransacThreshold_, config.confidence_);
    if (!fundamental.valid_ || fundamental.inlierIndices_.size() < config.minInliers_)
    {
        return result;
    }
    std::vector<PixelPoint> filteredFirst;
    std::vector<PixelPoint> filteredSecond;
    for (const auto index : fundamental.inlierIndices_)
    {
        filteredFirst.push_back(first.at(index));
        filteredSecond.push_back(second.at(index));
    }
    const auto essential = estimateEssential(filteredFirst, filteredSecond, firstCamera, secondCamera,
                                              config.ransacThreshold_, config.confidence_);
    if (!essential.valid_)
    {
        return result;
    }
    const auto pose = recoverRelativePose(essential.matrix_, filteredFirst, filteredSecond,
                                          firstCamera, secondCamera, essential.inlierIndices_);
    if (!pose.valid_ || pose.inlierIndices_.size() < config.minInliers_)
    {
        return result;
    }
    result.verified_ = true;
    result.fundamental_ = fundamental.matrix_;
    result.essential_ = essential.matrix_;
    result.relativePose_ = pose.pose_;
    for (const auto index : pose.inlierIndices_)
    {
        result.inlierIndices_.push_back(fundamental.inlierIndices_.at(index));
    }
    return result;
}
}
