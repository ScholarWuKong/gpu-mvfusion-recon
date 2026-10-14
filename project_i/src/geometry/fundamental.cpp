#include "geometry/fundamental.h"
#include "opencv_internal.h"
#include <opencv2/calib3d.hpp>

namespace sfm::geometry
{
MatrixEstimate estimateFundamental(const std::vector<PixelPoint>& first,
                                   const std::vector<PixelPoint>& second,
                                   const double threshold, const double confidence)
{
    detail::validatePairs(first, second, threshold, confidence);
    MatrixEstimate result;
    if (first.size() < 8)
    {
        return result;
    }
    cv::Mat mask;
    const auto fundamental = cv::findFundamentalMat(detail::pixels(first), detail::pixels(second),
                                                   cv::FM_RANSAC, threshold, confidence, mask);
    if (fundamental.rows == 3 && fundamental.cols == 3)
    {
        result.valid_ = true;
        result.matrix_ = detail::values(fundamental);
        result.inlierIndices_ = detail::inliers(mask);
    }
    return result;
}
}
