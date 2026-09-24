// main.cpp
// 环境与opencv测试

#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/core/cuda.hpp>
#include <opencv2/cudaarithm.hpp>

int main()
{
    std::cout << "===== CUDA OpenCV Environment Test =====\n";

    std::cout << "OpenCV version: "
              << CV_VERSION << '\n';

    int count = cv::cuda::getCudaEnabledDeviceCount();

    std::cout << "CUDA device count: "
              << count << '\n';

    if (count <= 0)
    {
        std::cerr << "ERROR: OpenCV cannot detect CUDA device.\n";
        return 1;
    }

    cv::cuda::printCudaDeviceInfo(0);

    cv::Mat cpu = cv::Mat::ones(512, 512, CV_32F);

    cv::cuda::GpuMat gpu;
    gpu.upload(cpu);

    cv::cuda::GpuMat result;
    cv::cuda::add(gpu, gpu, result);

    cv::Mat output;
    result.download(output);

    std::cout << "CUDA calculation result: "
              << output.at<float>(0, 0) << '\n';

    std::cout << "===== TEST PASSED =====\n";

    return 0;
}