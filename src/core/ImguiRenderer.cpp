#include <ImguiRenderer.h>
#include "common.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

void CV::ImguiRenderer::InitImgui(std::shared_ptr<CV::Renderer> renderer, GLFWwindow* _window)
{
	// imgui init stuff
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();

	std::vector<vk::DescriptorPoolSize> imguiPoolSizes = {
	{vk::DescriptorType::eSampler, 1000},
	{vk::DescriptorType::eCombinedImageSampler, 1000},
	{vk::DescriptorType::eSampledImage, 1000},
	{vk::DescriptorType::eStorageImage, 1000},
	{vk::DescriptorType::eUniformTexelBuffer, 1000},
	{vk::DescriptorType::eStorageTexelBuffer, 1000},
	{vk::DescriptorType::eUniformBuffer, 1000},
	{vk::DescriptorType::eStorageBuffer, 1000},
	{vk::DescriptorType::eUniformBufferDynamic, 1000},
	{vk::DescriptorType::eStorageBufferDynamic, 1000},
	{vk::DescriptorType::eInputAttachment, 1000}
	};

	vk::DescriptorPoolCreateInfo poolCI{};
	poolCI.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
	poolCI.maxSets = 1000 * MAX_FRAMES_IN_FLIGHT;
	poolCI.poolSizeCount = static_cast<uint32_t>(imguiPoolSizes.size());
	poolCI.pPoolSizes = imguiPoolSizes.data();

	VK_ASSERT_L(renderer->_device.createDescriptorPool(&poolCI, nullptr, &_imguiDescriptorPool),
		[&]()
		{
			renderer->_device.destroyDescriptorPool(_imguiDescriptorPool);
		});

	vk::PipelineRenderingCreateInfo pipelineRenderingInfo;
	pipelineRenderingInfo.colorAttachmentCount = 1;
	pipelineRenderingInfo.pColorAttachmentFormats = &renderer->_swapChainImageFormat;		

	ImGui_ImplGlfw_InitForVulkan(_window, true);
	ImGui_ImplVulkan_InitInfo initInfo{};
	initInfo.Instance = renderer->_instance;
	initInfo.PhysicalDevice = renderer->_physicalDevice;
	initInfo.Device = renderer->_device;
	initInfo.QueueFamily = renderer->_queueFamily;
	initInfo.Queue = renderer->_graphicsQueue;
	initInfo.PipelineCache = nullptr;
	initInfo.DescriptorPool = _imguiDescriptorPool;
	initInfo.Allocator = nullptr;
	initInfo.MinImageCount = 2;
	initInfo.ImageCount = renderer->_swapChainImages.size();
	initInfo.CheckVkResultFn = nullptr;

	initInfo.UseDynamicRendering = true;
	initInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;

	initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;


	ImGui_ImplVulkan_Init(&initInfo);

}

void CV::ImguiRenderer::BeginFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void CV::ImguiRenderer::ImguiFrameRender(GLFWwindow*_window, ImDrawData *_drawData)
{
	
}
