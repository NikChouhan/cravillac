#include <iostream>
#include <cstdlib>
#include <stdexcept>

#include "Application.h"

int main() 
{
    Cravillac::Application app("Hello Vulkan");
    app.Init();
    app.Run();
}