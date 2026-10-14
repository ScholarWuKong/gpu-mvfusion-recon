#pragma once
#include "geometry/types.h"
#include <vector>

namespace sfm::geometry
{
struct MatrixEstimate
{
    bool valid_ = false;
    Matrix3 matrix_ = {};
    std::vector<std::size_t> inlierIndices_;
};

MatrixEstimate estimateFundamental(const std::vector<PixelPoint>& first,
                                   const std::vector<PixelPoint>& second,
                                   double threshold, double confidence);
}
