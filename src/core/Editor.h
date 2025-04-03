#include "common.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

namespace Cravillac
{
    struct DearImguiCreateInfo
    {
        GLFWwindow* window;
        VkInstance instance;
        VkPhysicalDevice physicalDevice;
        VkQueue queue;
        VkFormat fomat;
    };

    class DearImGuiSetup
    {
    public:
        using CreateInfo = DearImguiCreateInfo;
        
        explicit DearImGuiSetup(CreateInfo const& createInfo);

        void NewFrame();
        void EndFrame();
        void Render(VkCommandBuffer cmdBuffer);

    private:
        enum class State : uint8_t
        {
            Ended,
            Begun
        };

        State m_state{};
    };
}