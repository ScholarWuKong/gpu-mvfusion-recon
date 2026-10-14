#pragma once
#include "scene/match.h"
#include "scene/reconstruction.h"
#include <filesystem>

namespace sfm::io
{
void writeReconstruction(const scene::Reconstruction& reconstruction,
                          const scene::MatchGraph& graph,
                          const std::filesystem::path& outputDirectory);
}
