#include "geometry/essential.h"
#include "opencv_internal.h"
#include <opencv2/calib3d.hpp>
#include <algorithm>

namespace sfm::geometry
{
MatrixEstimate estimateEssential(const std::vector<PixelPoint>& first,
                                 const std::vector<PixelPoint>& second,
                                 const CameraIntrinsics& firstCamera,
                                 const CameraIntrinsics& secondCamera,
                                 const double threshold, const double confidence)
{
    detail::validatePairs(first, second, threshold, confidence);
    const auto normalizedFirst = detail::pixels(first, &firstCamera);
    const auto normalizedSecond = detail::pixels(second, &secondCamera);
    MatrixEstimate result;
    if (first.size() < 5)
    {
        return result;
    }
    const double focal = std::min({firstCamera.fx_, firstCamera.fy_, secondCamera.fx_, secondCamera.fy_});
    cv::Mat mask;
    const auto essential = cv::findEssentialMat(normalizedFirst, normalizedSecond, 1.0,
                                                cv::Point2d(0.0, 0.0), cv::RANSAC, confidence,
                                                threshold / focal, mask);
    if (essential.rows == 3 && essential.cols == 3)
    {
        result.valid_ = true;
        result.matrix_ = detail::values(essential);
        result.inlierIndices_ = detail::inliers(mask);
    }
    return result;
}
}
