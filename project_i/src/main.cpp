#include "app/application.h"
#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
    try
    {
        return sfm::app::runApplication(argc, argv);
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Error: " << exception.what() << '\n';
        return 1;
    }
}
