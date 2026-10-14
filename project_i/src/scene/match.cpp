#include "scene/match.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <tuple>

namespace sfm::scene
{
ImagePair::ImagePair(const ImageId imageId1, const ImageId imageId2)
    : imageId1_(std::min(imageId1, imageId2)), imageId2_(std::max(imageId1, imageId2))
{
    if (imageId1 == imageId2)
    {
        throw std::invalid_argument("An image pair must contain two distinct IDs.");
    }
}

bool ImagePair::operator<(const ImagePair& other) const noexcept
{
    return std::tie(imageId1_, imageId2_) < std::tie(other.imageId1_, other.imageId2_);
}

PairwiseMatch PairwiseMatch::fromOrderedMatches(const ImageId imageId1,
                                               const ImageId imageId2,
                                               std::vector<FeatureMatch> matches)
{
    PairwiseMatch result;
    result.pair_ = ImagePair(imageId1, imageId2);
    result.attempted_ = true;
    if (imageId1 > imageId2)
    {
        for (auto& match : matches)
        {
            std::swap(match.featureIndex1_, match.featureIndex2_);
        }
    }
    result.matches_ = std::move(matches);
    return result;
}

void MatchGraph::set(PairwiseMatch match)
{
    if (match.pair_.imageId1_ >= match.pair_.imageId2_)
    {
        throw std::invalid_argument("MatchGraph requires normalized image pairs.");
    }
    for (const auto& featureMatch : match.matches_)
    {
        if (!std::isfinite(featureMatch.distance_) || featureMatch.distance_ < 0.0f)
        {
            throw std::invalid_argument("Invalid descriptor distance.");
        }
    }
    for (const auto index : match.inlierMatchIndices_)
    {
        if (index >= match.matches_.size())
        {
            throw std::out_of_range("Invalid geometric inlier index.");
        }
    }
    const auto pair = match.pair_;
    pairs_.insert_or_assign(pair, std::move(match));
}

bool MatchGraph::contains(const ImagePair& pair) const
{
    return pairs_.find(pair) != pairs_.end();
}

const std::map<ImagePair, PairwiseMatch>& MatchGraph::pairs() const noexcept
{
    return pairs_;
}

std::map<ImagePair, PairwiseMatch>& MatchGraph::pairs() noexcept
{
    return pairs_;
}
}
