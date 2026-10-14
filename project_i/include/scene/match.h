#pragma once

#include "geometry/types.h"
#include <map>
#include <vector>

namespace sfm::scene
{
struct ImagePair
{
    ImageId imageId1_ = 0;
    ImageId imageId2_ = 0;

    ImagePair() = default;
    ImagePair(ImageId imageId1, ImageId imageId2);
    bool operator<(const ImagePair& other) const noexcept;
};

struct FeatureMatch
{
    std::size_t featureIndex1_ = 0;
    std::size_t featureIndex2_ = 0;
    float distance_ = 0.0f;
};

enum class GeometryStatus
{
    UNVERIFIED,
    REJECTED,
    VERIFIED
};

struct PairwiseMatch
{
    ImagePair pair_;
    bool attempted_ = false;
    std::vector<FeatureMatch> matches_;
    GeometryStatus geometryStatus_ = GeometryStatus::UNVERIFIED;
    // Indices into matches_, never a replacement for the descriptor matches.
    std::vector<std::size_t> inlierMatchIndices_;
    Matrix3 fundamental_ = {};
    Matrix3 essential_ = {};
    geometry::CameraPose relativePose_;

    static PairwiseMatch fromOrderedMatches(ImageId imageId1, ImageId imageId2,
                                            std::vector<FeatureMatch> matches);
};

class MatchGraph
{
public:
    void set(PairwiseMatch match);
    bool contains(const ImagePair& pair) const;
    const std::map<ImagePair, PairwiseMatch>& pairs() const noexcept;
    std::map<ImagePair, PairwiseMatch>& pairs() noexcept;

private:
    std::map<ImagePair, PairwiseMatch> pairs_;
};
}
