#include "matching/gpu_feature_matcher.h"
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudafeatures2d.hpp>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace sfm::matching
{
struct GpuFeatureMatcher::Impl
{
    struct CachedDescriptors
    {
        std::size_t descriptorSize_ = 0;
        // Exact content comparison avoids stale uploads even when an ID is reused.
        std::vector<std::uint8_t> bytes_;
        cv::cuda::GpuMat gpu_;
    };

    FeatureMatcherConfig config_;
    cv::Ptr<cv::cuda::DescriptorMatcher> matcher_;
    cv::cuda::Stream stream_;
    std::map<ImageId, CachedDescriptors> cache_;

    cv::cuda::GpuMat& descriptors(const scene::FeatureSet& features)
    {
        auto& cached = cache_[features.imageId_];
        if (cached.descriptorSize_ != features.descriptorSize_ ||
            cached.bytes_ != features.descriptors_)
        {
            cv::cuda::GpuMat uploaded;
            if (!features.empty())
            {
                cv::Mat host(static_cast<int>(features.size()),
                             static_cast<int>(features.descriptorSize_), CV_8UC1,
                             const_cast<std::uint8_t*>(features.descriptors_.data()));
                uploaded.upload(host, stream_);
                stream_.waitForCompletion();
            }
            cached.gpu_ = std::move(uploaded);
            cached.bytes_ = features.descriptors_;
            cached.descriptorSize_ = features.descriptorSize_;
        }
        return cached.gpu_;
    }
};

GpuFeatureMatcher::GpuFeatureMatcher(const FeatureMatcherConfig& config)
    : impl_(std::make_unique<Impl>())
{
    config.validate();
    if (cv::cuda::getCudaEnabledDeviceCount() <= 0)
    {
        throw std::runtime_error("GPU matching requires OpenCV CUDA and a CUDA device.");
    }
    impl_->config_ = config;
    impl_->matcher_ = cv::cuda::DescriptorMatcher::createBFMatcher(cv::NORM_HAMMING);
    if (impl_->matcher_.empty())
    {
        throw std::runtime_error("Could not create CUDA Hamming matcher.");
    }
}

GpuFeatureMatcher::~GpuFeatureMatcher() = default;

scene::MatchGraph GpuFeatureMatcher::matchPairs(
    const std::vector<scene::FeatureSet>& featureSets,
    const std::vector<scene::ImagePair>& pairs)
{
    std::map<ImageId, const scene::FeatureSet*> byId;
    for (const auto& features : featureSets)
    {
        if (!features.hasValidDescriptors() ||
            features.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
            (!features.empty() && features.descriptorSize_ != 32))
        {
            throw std::invalid_argument("Expected valid 32-byte ORB descriptors and feature IDs.");
        }
        if (!byId.emplace(features.imageId_, &features).second)
        {
            throw std::invalid_argument("Duplicate image ID in feature sets.");
        }
    }
    // Validate the whole request before any upload or matching.
    std::set<scene::ImagePair> uniquePairs;
    for (const auto& requested : pairs)
    {
        const scene::ImagePair pair(requested.imageId1_, requested.imageId2_);
        if (byId.count(pair.imageId1_) == 0 || byId.count(pair.imageId2_) == 0)
        {
            throw std::invalid_argument("Requested pair contains an unknown image ID.");
        }
        const auto& first = *byId.at(pair.imageId1_);
        const auto& second = *byId.at(pair.imageId2_);
        if (first.descriptorSize_ != second.descriptorSize_ && !first.empty() && !second.empty())
        {
            throw std::invalid_argument("Descriptor dimensions do not agree.");
        }
        uniquePairs.insert(pair);
    }

    scene::MatchGraph graph;
    for (const auto& pair : uniquePairs)
    {
        const auto& first = *byId.at(pair.imageId1_);
        const auto& second = *byId.at(pair.imageId2_);
        auto result = scene::PairwiseMatch::fromOrderedMatches(pair.imageId1_, pair.imageId2_, {});
        // Even empty/too-small inputs produce an attempted edge.
        if (!first.empty() && second.size() >= 2)
        {
            auto& query = impl_->descriptors(first);
            auto& train = impl_->descriptors(second);
            cv::cuda::GpuMat gpuMatches;
            impl_->matcher_->knnMatchAsync(query, train, gpuMatches, impl_->config_.k_,
                                          cv::noArray(), impl_->stream_);
            impl_->stream_.waitForCompletion();
            std::vector<std::vector<cv::DMatch>> neighbors;
            impl_->matcher_->knnMatchConvert(gpuMatches, neighbors);
            for (const auto& candidates : neighbors)
            {
                if (candidates.size() < 2)
                {
                    continue;
                }
                const auto& best = candidates[0];
                if (best.queryIdx < 0 || best.trainIdx < 0 ||
                    static_cast<std::size_t>(best.queryIdx) >= first.size() ||
                    static_cast<std::size_t>(best.trainIdx) >= second.size())
                {
                    throw std::runtime_error("CUDA matcher returned an invalid feature index.");
                }
                if (best.distance < impl_->config_.ratioThreshold_ * candidates[1].distance)
                {
                    result.matches_.push_back({static_cast<std::size_t>(best.queryIdx),
                                               static_cast<std::size_t>(best.trainIdx), best.distance});
                }
            }
        }
        graph.set(std::move(result));
    }
    return graph;
}

void GpuFeatureMatcher::clearCache()
{
    impl_->stream_.waitForCompletion();
    impl_->cache_.clear();
}
}
