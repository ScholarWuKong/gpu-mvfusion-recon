#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace sfm
{
using ImageId = std::uint32_t;
using TrackId = std::size_t;
using PointId = std::size_t;
using Vector3 = std::array<double, 3>;
using Matrix3 = std::array<double, 9>;
inline constexpr PointId INVALID_POINT_ID = std::numeric_limits<PointId>::max();
}
