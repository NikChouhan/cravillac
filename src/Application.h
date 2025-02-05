#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include "glm/glm.hpp"

#include <vector>
#include <optional>
#include <array>

namespace VKTest
{
    constexpr uint32_t WIDTH = 800;
    constexpr uint32_t HEIGHT = 600;

    constexpr int MAX_FRAMES_IN_FLIGHT = 2;

    enum class Buffer
    {
        VERTEX,
        INDEX
    };

    struct Vertex
    {
        glm::vec2 pos;
        glm::vec3 color;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription bindingDesc{
                .binding = 0,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
            };

            return bindingDesc;
        }

        static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription()
        {
            std::array<VkVertexInputAttributeDescription, 2> attrDescs{};

            attrDescs[0].binding = 0;
            attrDescs[0].location = 0;
            attrDescs[0].format = VK_FORMAT_R32G32_SFLOAT;
            attrDescs[0].offset = offsetof(Vertex, pos);

            attrDescs[1].binding = 0;
            attrDescs[1].location = 1;
            attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attrDescs[1].offset = offsetof(Vertex, color);

            return attrDescs;
        }
    };

    const std::vector<Vertex> vertices = {
    {{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
    };

    const std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

    const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };


#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif


    struct QueueFamilyIndices
    {
        std::optional<uint32_t> _graphicsFamily;
        std::optional<uint32_t> _presentFamily;

        bool _IsComplete()
        {
            return _graphicsFamily.has_value() && _presentFamily.has_value();
        }
    };

    struct SwapChainSupportDetails
    {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    class VulkanTest
    {
    public:
        void Run();

    private:

        // non API dependent
        void InitWindow();
        void InitVulkan();
        void Render();
        void Cleanup() const;

        // Vulkan base setup
        void CreateInstance();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateSwapChain();
        void CreateImageView();
        //void RecreateSwapChain(); // TODO

        // drawing stuff
        void CreateGraphicsPipeline();
        void CreateCommandPool();
        void CreateVertexBuffer();
        void CreateCommandBuffer();
        void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        void DrawFrame();

        // fence stuff
        void CreateSynObjects();

        // vulkan base support funcs
        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
        bool CheckValidationLayerSupport();
        std::vector<const char*> GetRequiredExtensions();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void SetupDebugMessenger();
        bool IsDeviceSuitable(VkPhysicalDevice device);
        bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice physicalDevice) const;
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        // transition image layout for rendering/presenting, etc etc
        void TransitionImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

        // shaders stuff
        static std::vector<char> ReadFile(const std::string& filename);
        VkShaderModule CreateShaderModule(const std::vector<char>& code) const;

    public:
        GLFWwindow* m_window;
    private:
        uint32_t currentFrame = 0;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkSurfaceKHR m_surface = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkQueue m_presentQueue = VK_NULL_HANDLE;
        VkSwapchainKHR m_swapChain = VK_NULL_HANDLE;
        VkFormat m_swapChainImageFormat;
        VkExtent2D m_swapChainExtent;
        std::vector<VkImage> m_swapChainImages{};
        std::vector<VkImageView> m_swapChainImageViews{};
        VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;

        // bufffer vars
        VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
        VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;


        // issue commands
        VkCommandPool m_commandPool = VK_NULL_HANDLE;
        std::vector<VkCommandBuffer> m_commandBuffer;

        // sync primitives
        std::vector<VkSemaphore> m_imageAvailableSemaphore;
        std::vector<VkSemaphore> m_renderFinishedSemaphore;
        std::vector<VkFence> m_inFlightFence;  

        VkDebugUtilsMessengerEXT debugMessenger;
    };
};