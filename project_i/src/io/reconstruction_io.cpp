#include "io/reconstruction_io.h"
#include <fstream>
#include <iomanip>
#include <set>

namespace sfm::io
{
namespace
{
std::ofstream openOutput(const std::filesystem::path& path)
{
    std::ofstream output(path);
    output.exceptions(std::ios::badbit | std::ios::failbit);
    output << std::setprecision(17);
    return output;
}
}

void writeReconstruction(const scene::Reconstruction& reconstruction,
                          const scene::MatchGraph& graph,
                          const std::filesystem::path& outputDirectory)
{
    reconstruction.validate();
    std::filesystem::create_directories(outputDirectory);
    auto points = openOutput(outputDirectory / "points.ply");
    points << "ply\nformat ascii 1.0\nelement vertex " << reconstruction.points_.size()
           << "\nproperty double x\nproperty double y\nproperty double z\nend_header\n";
    for (const auto& point : reconstruction.points_)
    {
        points << point.position_[0] << ' ' << point.position_[1] << ' ' << point.position_[2] << '\n';
    }
    points.close();
    auto cameras = openOutput(outputDirectory / "cameras.json");
    cameras << "{\n  \"pose_convention\": \"X_camera = R * X_world + t\",\n"
            << "  \"scale\": \"arbitrary, seed baseline = 1\",\n  \"images\": [\n";
    bool first = true;
    for (const auto& entry : reconstruction.images_)
    {
        const auto& image = entry.second;
        if (!first)
        {
            cameras << ",\n";
        }
        first = false;
        cameras << "    {\"image_id\": " << image.id_ << ", \"registered\": "
                << (image.registered_ ? "true" : "false") << ", \"width\": " << image.width_
                << ", \"height\": " << image.height_ << ", \"intrinsics\": ["
                << image.camera_.fx_ << ',' << image.camera_.fy_ << ',' << image.camera_.cx_ << ',' << image.camera_.cy_
                << "], \"rotation\": [";
        for (std::size_t index = 0; index < image.pose_.rotation_.size(); ++index)
        {
            cameras << (index == 0 ? "" : ",") << image.pose_.rotation_[index];
        }
        cameras << "], \"translation\": [" << image.pose_.translation_[0] << ','
                << image.pose_.translation_[1] << ',' << image.pose_.translation_[2] << "]}";
    }
    cameras << "\n  ]\n}\n";
    cameras.close();
    auto observations = openOutput(outputDirectory / "observations.csv");
    observations << "track_id,point_id,image_id,feature_index,x,y,enabled\n";
    for (const auto& track : reconstruction.tracks_)
    {
        for (const auto& observation : track.observations_)
        {
            observations << track.id_ << ',';
            if (track.pointId_ != INVALID_POINT_ID)
            {
                observations << track.pointId_;
            }
            observations << ',' << observation.imageId_ << ',' << observation.featureIndex_ << ','
                         << observation.x_ << ',' << observation.y_ << ',' << observation.enabled_ << '\n';
        }
    }
    observations.close();
    auto pairStats = openOutput(outputDirectory / "pairs.csv");
    auto matches = openOutput(outputDirectory / "matches.csv");
    pairStats << "image_id1,image_id2,attempted,raw_matches,inliers,geometry_status\n";
    matches << "image_id1,image_id2,raw_match_index,feature_index1,feature_index2,distance,geometric_inlier\n";
    for (const auto& edge : graph.pairs())
    {
        const auto& pair = edge.second;
        const auto status = pair.geometryStatus_ == scene::GeometryStatus::VERIFIED ? "verified" :
            pair.geometryStatus_ == scene::GeometryStatus::REJECTED ? "rejected" : "unverified";
        pairStats << pair.pair_.imageId1_ << ',' << pair.pair_.imageId2_ << ',' << pair.attempted_ << ','
                  << pair.matches_.size() << ',' << pair.inlierMatchIndices_.size() << ',' << status << '\n';
        const std::set<std::size_t> inliers(pair.inlierMatchIndices_.begin(), pair.inlierMatchIndices_.end());
        for (std::size_t index = 0; index < pair.matches_.size(); ++index)
        {
            const auto& match = pair.matches_[index];
            matches << pair.pair_.imageId1_ << ',' << pair.pair_.imageId2_ << ',' << index << ','
                    << match.featureIndex1_ << ',' << match.featureIndex2_ << ',' << match.distance_ << ','
                    << (inliers.count(index) != 0) << '\n';
        }
    }
    pairStats.close();
    matches.close();
}
}
