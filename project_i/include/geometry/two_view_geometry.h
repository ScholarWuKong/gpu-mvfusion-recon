#pragma once
#include "geometry/pose_estimation.h"

namespace sfm::geometry
{
struct TwoViewConfig
{
    double ransacThreshold_ = 2.0;
    double confidence_ = 0.999;
    std::size_t minInliers_ = 15;
    void validate() const;
};

struct TwoViewResult
{
    bool verified_ = false;
    Matrix3 fundamental_ = {};
    Matrix3 essential_ = {};
    CameraPose relativePose_;
    std::vector<std::size_t> inlierIndices_;
};

TwoViewResult verifyTwoView(const std::vector<PixelPoint>& first,
                            const std::vector<PixelPoint>& second,
                            const CameraIntrinsics& firstCamera,
                            const CameraIntrinsics& secondCamera,
                            const TwoViewConfig& config);
}
