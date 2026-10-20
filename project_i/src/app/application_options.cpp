#include "app/application_options.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace sfm::app
{
namespace
{
double parseNumber(const std::string& value)
{
    std::size_t parsed = 0;
    const double number = std::stod(value, &parsed);
    if (parsed != value.size() || !std::isfinite(number))
    {
        throw std::invalid_argument("Invalid numeric option: " + value);
    }
    return number;
}

int parseInteger(const std::string& value, const int minimum)
{
    const double number = parseNumber(value);
    if (number < minimum || number > std::numeric_limits<int>::max() || std::floor(number) != number)
    {
        throw std::invalid_argument("Invalid integer option: " + value);
    }
    return static_cast<int>(number);
}

bool parseToggle(const std::string& value)
{
    if (value != "on" && value != "off")
    {
        throw std::invalid_argument("Toggle options accept on or off.");
    }
    return value == "on";
}

scene::Camera parseIntrinsics(const std::string& value)
{
    std::istringstream input(value);
    std::vector<double> parameters;
    std::string token;
    while (std::getline(input, token, ','))
    {
        parameters.push_back(parseNumber(token));
    }
    if (parameters.size() != 4 || value.empty() || value.back() == ',')
    {
        throw std::invalid_argument("Intrinsics require exactly fx,fy,cx,cy.");
    }
    const scene::Camera camera{parameters[0], parameters[1], parameters[2], parameters[3]};
    if (!camera.isValid())
    {
        throw std::invalid_argument("Intrinsics require finite parameters and positive fx/fy.");
    }
    return camera;
}
}

ApplicationOptions parseApplicationOptions(const std::vector<std::string>& arguments)
{
    ApplicationOptions options;
    options.featureConfig_.nFeatures_ = 5000;
    for (std::size_t index = 0; index < arguments.size(); ++index)
    {
        const auto& argument = arguments[index];
        if (argument.rfind("--", 0) != 0)
        {
            options.imagePaths_.push_back(argument);
            continue;
        }
        if (index + 1 >= arguments.size())
        {
            throw std::invalid_argument("Missing value for " + argument);
        }
        const auto& value = arguments[++index];
        if (argument == "--feature-backend")
        {
            if (value != "cpu" && value != "gpu")
            {
                throw std::invalid_argument("Feature backend must be cpu or gpu.");
            }
            options.featureConfig_.backend_ = value == "cpu" ? feature::FeatureBackend::CPU : feature::FeatureBackend::GPU;
        }
        else if (argument == "--ba")
        {
            if (value != "cpu" && value != "gpu" && value != "none")
            {
                throw std::invalid_argument("BA backend must be cpu, gpu, or none.");
            }
            options.mapperConfig_.runBundleAdjustment_ = value != "none";
            options.baConfig_.backend_ = value == "gpu" ? ba::BundleAdjustmentBackend::GPU : ba::BundleAdjustmentBackend::CPU;
        }
        else if (argument == "--focal")
        {
            options.focalLength_ = parseNumber(value);
            if (options.focalLength_ <= 0.0)
            {
                throw std::invalid_argument("Focal length must be positive.");
            }
        }
        else if (argument == "--intrinsics")
        {
            options.calibratedCamera_ = parseIntrinsics(value);
        }
        else if (argument == "--nfeatures")
        {
            options.featureConfig_.nFeatures_ = parseInteger(value, 1);
        }
        else if (argument == "--fast-threshold")
        {
            options.featureConfig_.fastThreshold_ = parseInteger(value, 0);
        }
        else if (argument == "--gpu-orb-blur")
        {
            options.featureConfig_.gpuBlurForDescriptor_ = parseToggle(value);
        }
        else if (argument == "--gpu-feature-grid")
        {
            const auto separator = value.find('x');
            if (separator == std::string::npos)
            {
                throw std::invalid_argument("GPU feature grid requires columnsxrows, e.g. 8x6.");
            }
            options.featureConfig_.gpuGridColumns_ = parseInteger(value.substr(0, separator), 1);
            options.featureConfig_.gpuGridRows_ = parseInteger(value.substr(separator + 1), 1);
        }
        else if (argument == "--robust-triangulation")
        {
            options.mapperConfig_.robustTriangulation_ = parseToggle(value);
        }
        else if (argument == "--ratio")
        {
            options.matcherConfig_.ratioThreshold_ = static_cast<float>(parseNumber(value));
        }
        else if (argument == "--output")
        {
            options.outputDirectory_ = value;
        }
        else
        {
            throw std::invalid_argument("Unknown option: " + argument);
        }
    }
    if (options.imagePaths_.size() < 2 || options.imagePaths_.size() > std::numeric_limits<ImageId>::max())
    {
        throw std::invalid_argument("Supply at least two input images.");
    }
    if (options.calibratedCamera_ && options.focalLength_ > 0.0)
    {
        throw std::invalid_argument("Use either --intrinsics or --focal.");
    }
    options.featureConfig_.validate();
    options.matcherConfig_.validate();
    options.baConfig_.validate();
    options.mapperConfig_.validate();
    return options;
}

scene::Camera cameraForImage(const ApplicationOptions& options, const int width, const int height)
{
    if (width <= 0 || height <= 0)
    {
        throw std::invalid_argument("Camera selection requires positive image dimensions.");
    }
    if (options.calibratedCamera_)
    {
        return *options.calibratedCamera_;
    }
    const double focal = options.focalLength_ > 0.0 ? options.focalLength_ : 1.2 * std::max(width, height);
    return {focal, focal, 0.5 * (width - 1), 0.5 * (height - 1)};
}
}
