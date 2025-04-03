#include "Application.h"

#include <chrono>
#include <cstring>

#define GL_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "common.h"
#include "renderer.h"
#include "Texture.h"
#include "Log.h"
#include "ResourceManager.h"
#include "PipelineManager.h"
#include "vk_utils.h"
#include "Vertex.h"

namespace Cravillac
{
	Application::Application(const char* title) : m_resourceManager(nullptr), m_window(nullptr), m_surface(VK_NULL_HANDLE)
	{
		renderer = std::make_shared<Renderer>();
		textures = new std::vector<Texture>();
		io = nullptr;
	}
	void Application::Init()
	{
		Log::Init();

		// commented out cuz this is meant to be cross-platform, and the hinstance and hwnd part only works for windows
		/*
		VkWin32SurfaceCreateInfoKHR createInfo {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = GetModuleHandle(nullptr),
		.hwnd = glfwGetWin32Window(m_window),
		};

		if (vkCreateWin32SurfaceKHR(m_instance, &createInfo, nullptr, &m_surface) != VK_SUCCESS)
		{
			Log::Error("[VULKAN] Surface Creation Failure");
		}
		else Log::Info("[VULKAN] Surface Creation Success");*/

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Test", nullptr, nullptr);

		renderer->InitVulkan();

		if (glfwCreateWindowSurface(renderer->m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
		{
			Log::Error("[VULKAN] Surface Creation Failure");
		}
		else
			Log::Info("[VULKAN] Surface Creation Success");
		// post surface stuff
		renderer->PickPhysicalDevice(m_surface);
		renderer->CreateLogicalDevice(m_surface);
		renderer->CreateSwapChain(m_surface, m_window);

		renderer->CreateCommandPool(m_surface);

		m_resourceManager = new ResourceManager(renderer);

		// init imgui

		SetResources();

	}
	void Application::Run()
	{
		DrawFrame();
	}
	void Application::DrawFrame()
	{
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();

			vkWaitForFences(renderer->m_device, 1, &m_inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);
			vkResetFences(renderer->m_device, 1, &m_inFlightFence[currentFrame]);

			UpdateUniformBuffer(currentFrame);

			uint32_t imageIndex{};
			vkAcquireNextImageKHR(renderer->m_device, renderer->m_swapChain, UINT64_MAX, m_imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

			vkResetCommandBuffer(m_cmdBuffers[currentFrame], 0);

			RecordCmdBuffer(m_cmdBuffers[currentFrame], imageIndex);

			VkSubmitInfo submitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
			VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore[currentFrame] };
			VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };        // after the fragment stage cuz the actual shading occurs after. fragemnt stage only computes the color, doesnt actually render to the frame
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_cmdBuffers[currentFrame];

			VkSemaphore signalSemaphore[] = { m_renderFinishedSemaphore[currentFrame] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphore;

			if (vkQueueSubmit(renderer->m_graphicsQueue, 1, &submitInfo, m_inFlightFence[currentFrame]) != VK_SUCCESS)
			{
				Log::Error("[DRAW] Submit Draw Command buffer Failed");
			}
			// else Log::Info("[DRAW] Submit Draw Command buffer Success");

			VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &m_renderFinishedSemaphore[currentFrame];
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &renderer->m_swapChain;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;

			vkQueuePresentKHR(renderer->m_presentQueue, &presentInfo);

			currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}

		vkDeviceWaitIdle(renderer->m_device);
	}
	void Application::SetResources()
	{
		// vertex buffer
		VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
		VkBuffer stagingBuffer{};
		VkDeviceMemory stagingBufferMemory{};

		stagingBuffer = m_resourceManager->CreateBufferBuilder()
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			.build(stagingBufferMemory);

		void* data{};
		vkMapMemory(renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(renderer->m_device, stagingBufferMemory);

		m_vertexBuffer = m_resourceManager->CreateBufferBuilder()
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(m_vertexMemory);

		CopyBuffer(renderer->m_device, renderer->m_commandPool, renderer->m_graphicsQueue, stagingBuffer, m_vertexBuffer, bufferSize);

		// index buffer
		bufferSize = indices.size() * sizeof(indices[0]);

		stagingBuffer = m_resourceManager->CreateBufferBuilder()
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			.build(stagingBufferMemory);

		data = nullptr;
		vkMapMemory(renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
		vkUnmapMemory(renderer->m_device, stagingBufferMemory);

		m_indexBuffer = m_resourceManager->CreateBufferBuilder()
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(m_indexMemory);

		CopyBuffer(renderer->m_device, renderer->m_commandPool, renderer->m_graphicsQueue, stagingBuffer, m_indexBuffer, bufferSize);
		vkDestroyBuffer(renderer->m_device, stagingBuffer, nullptr);
		vkFreeMemory(renderer->m_device, stagingBufferMemory, nullptr);

		// ubo for camera

		bufferSize = sizeof(UniformBufferObject);

		m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBufferMem.resize(MAX_FRAMES_IN_FLIGHT);
		m_uboMemMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_uniformBuffers[i] = m_resourceManager->CreateBufferBuilder()
				.setSize(bufferSize)
				.setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
				.setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				.build(m_uniformBufferMem[i]);
			vkMapMemory(renderer->m_device, m_uniformBufferMem[i], 0, bufferSize, 0, &m_uboMemMapped[i]);
		}

		// pipeline
		// TODO

		// descriptor pool/sets
		std::vector<VkDescriptorPoolSize> poolSizes;

		poolSizes.resize(2);

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 32);

		m_resourceManager->ConfigureDescriptorPoolSizes(poolSizes, MAX_FRAMES_IN_FLIGHT*2);

		VkDescriptorPool descPool = m_resourceManager->getDescriptorPool();
		std::vector<VkDescriptorSetLayout> descLayout;

		descLayout.resize(2);
		descriptorSets.resize(2);

		Texture tex1, tex2;
		tex1.LoadTexture(renderer, "../../../../assets/textures/cat.jpg");
		textures->push_back(tex1);
		tex2.LoadTexture(renderer, "../../../../assets/textures/texture.jpg");
		textures->push_back(tex2);

		descLayout[0] = m_resourceManager->getDescriptorSetLayout("ubo");
		descLayout[1] = m_resourceManager->getDescriptorSetLayout("textures");


		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			descriptorSets[i][0] = m_resourceManager->CreateDescriptorSet(descLayout[0]);
			descriptorSets[i][1] = m_resourceManager->CreateDescriptorSet(descLayout[1]);
			// create two descriptor sets and update them
			// first ubo buffer
			descriptorSets[i][0] = m_resourceManager->UpdateDescriptorSet(descriptorSets[i][0], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_uniformBuffers[i], sizeof(UniformBufferObject), nullptr);
			// the texture descriptor
			VkBuffer buffer = VK_NULL_HANDLE;
			descriptorSets[i][1] = m_resourceManager->UpdateDescriptorSet(descriptorSets[i][1], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, buffer, static_cast<uint64_t>(0), textures);
		}

		// manage pipelines

		PipelineManager* pipelineManager = m_resourceManager->getPipelineManager();

		VkVertexInputBindingDescription binding = Vertex::getBindingDescription();

		std::array<VkVertexInputAttributeDescription, 3> attributes = Vertex::getAttributeDescription();

		PipelineManager::Builder(pipelineManager)
			.setVertexShader("../../../../shaders/Triangle.vert.spv")
			.setFragmentShader("../../../../shaders/Triangle.frag.spv")
			.addDescriptorSetLayout("ubo")
			.addDescriptorSetLayout("textures")
			.setVertexInput(binding, attributes)
			.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.setDynamicStates({ VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR })
			.setDepthTest(false)
			.setBlendMode(false)
			.build("triangle");
		// cmd buffer and sync objects
		renderer->CreateCommandBuffer(m_cmdBuffers);
		renderer->CreateSynObjects(m_imageAvailableSemaphore, m_renderFinishedSemaphore, m_inFlightFence);
	}
	void Application::RecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		PipelineManager* pipelineManager = m_resourceManager->getPipelineManager();
		VkPipelineLayout pipelineLayout = pipelineManager->getPipelineLayout("ubo;textures;");

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr };

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			Log::Error("[DRAW] Recording Command buffer Failure");
		}
		// else Log::Info("[DRAW] Recording Command buffer Success");

		// dynamic rendering stuff

		// transition color image from undefined to optimal for rendering
		TransitionImage(commandBuffer, renderer->m_swapChainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

		VkRenderingAttachmentInfo colorAttachmentInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = renderer->m_swapChainImageViews[imageIndex],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE };
		colorAttachmentInfo.clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

		VkRenderingInfo renderingInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		};
		renderingInfo.renderArea.offset = { 0, 0 };
		renderingInfo.renderArea.extent = renderer->m_swapChainExtent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;

		auto graphicsPipeline = pipelineManager->getPipeline("triangle");

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(renderer->m_swapChainExtent.width);
		viewport.height = static_cast<float>(renderer->m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = renderer->m_swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);


		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 2, descriptorSets[currentFrame].data(), 0, nullptr);

		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRendering(commandBuffer);

		TransitionImage(commandBuffer, renderer->m_swapChainImages[imageIndex], VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			Log::Error("[DRAW] Recording Command buffer Failure");
		}
		// else Log::Info("[DRAW] Recording Command buffer Success");
	}


	void Application::UpdateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.f), time * glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
		ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.proj = glm::perspective(glm::radians(45.0f), renderer->m_swapChainExtent.width / static_cast<float>(renderer->m_swapChainExtent.height), 0.1f, 10.0f);

		ubo.proj[1][1] *= -1;

		memcpy(m_uboMemMapped[currentImage], &ubo, sizeof(ubo));
	}
}