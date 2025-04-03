#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "Editor.h"

Cravillac::DearImGuiSetup::DearImGuiSetup(CreateInfo const &createInfo)
{
    ImGui::CreateContext();
    ImGuiIO* io;
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
    io->ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking

    ImGui::StyleColorsDark();
}