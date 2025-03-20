#include "Application.h"
#include "renderer.h"
#include "Texture.h"
#include "Log.h"
#include "ResourceManager.h"
#include "vk_utils.h"
#include "common.h"

namespace Cravillac
{
	Application::Application(const char* title) : m_resourceManager(nullptr), m_window(nullptr), m_surface(VK_NULL_HANDLE)
	{
		renderer = std::make_shared<Renderer>();
	}
	void Application::Init()
	{
		Log::Init();

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
		renderer->PickPhysicalDevice();
		renderer->CreateLogicalDevice();
		renderer->CreateSwapChain();

		m_resourceManager = new ResourceManager(renderer->m_device, renderer->m_physicalDevice);

		SetResources();

	}
	void Application::Run()
	{
		DrawFrame();
	}
	void Application::DrawFrame()
	{
		
	}
	void Application::SetResources()
	{
		// vertex buffer
		VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();
		VkBuffer stagingBuffer{};
		VkDeviceMemory stagingBufferMemory{};

		stagingBuffer = m_resourceManager->CreateBuffer(bufferSize, stagingBufferMemory, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data{};
		vkMapMemory(renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), size_t(bufferSize));
		vkUnmapMemory(renderer->m_device, stagingBufferMemory);
		m_vertexBuffer = m_resourceManager->CreateBuffer(bufferSize, m_vertexMemory, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		CopyBuffer(renderer->m_device, renderer->m_commandPool, renderer->m_graphicsQueue, stagingBuffer, m_vertexBuffer, bufferSize);
		vkDestroyBuffer(renderer->m_device, stagingBuffer, nullptr);
		vkFreeMemory(renderer->m_device, stagingBufferMemory, nullptr);

		// index buffer
		bufferSize = sizeof(indices);

		stagingBuffer = m_resourceManager->CreateBuffer(bufferSize, stagingBufferMemory, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data{};
		vkMapMemory(renderer->m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), size_t(bufferSize));
		vkUnmapMemory(renderer->m_device, stagingBufferMemory);
		m_indexBuffer = m_resourceManager->CreateBuffer(bufferSize, m_indexMemory, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
			m_uniformBuffers[i] = m_resourceManager->CreateBuffer(bufferSize, m_uniformBufferMem[i], VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
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
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * 1000);

		m_resourceManager->ConfigureDescriptorPoolSizes(poolSizes, MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPool descPool = m_resourceManager->getDescriptorPool();
		std::vector<VkDescriptorSetLayout> descLayout;
		std::vector<VkDescriptorSet> descriptorSets;

		descLayout.resize(2);
		descriptorSets.resize(2);

		Texture tex;
		tex.LoadTexture(renderer, "../../../../assets/textures/cat.jpg");

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorSetLayoutBinding uboLayoutBinding{
			   .binding = 0,
			   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			   .descriptorCount = 1,
			   .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
			   .pImmutableSamplers = nullptr };

			// this was for non bindless
			/*VkDescriptorSetLayoutBinding samplerLayoutBinding{
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.pImmutableSamplers = nullptr};*/

				// bindless texture layout

			VkDescriptorSetLayoutBinding samplerLayoutBinding{
				.binding = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = 1000,
				.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
				.pImmutableSamplers = nullptr,
			};

			std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

			std::array<VkDescriptorBindingFlags, 2> bindingFlagsArr = {
				0, // No special flags for the UBO
				VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT };

			VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlags{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.bindingCount = static_cast<uint32_t>(bindingFlagsArr.size()),
				.pBindingFlags = bindingFlagsArr.data() };

			VkDescriptorSetLayoutCreateInfo decLayoutCI{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext = &bindingFlags,
				.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
				.bindingCount = static_cast<uint32_t>(bindings.size()),
				.pBindings = bindings.data() };

			if (vkCreateDescriptorSetLayout(renderer->m_device, &descLayoutCI, nullptr, &descLayout[i]) != VK_SUCCESS)
			{
				Log::Error("[VULKAN] Desc layout creation failed");
			}
			descriptorSets[i] = m_resourceManager->CreateDescriptorSet(descLayout[i]);

			// create two descriptor sets and update them
			// first ubo buffer
			VkDeviceSize bufferSize = sizeof(UniformBufferObject);
			descriptorSets[i] = m_resourceManager->UpdateDescriptorSet(descriptorSets[i], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_uniformBuffers[i], bufferSize, nullptr);
			// the texture descriptor
			VkBuffer buffer = VK_NULL_HANDLE;
			descriptorSets[i] = m_resourceManager->UpdateDescriptorSet(descriptorSets[i], 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, buffer , NULL, textures);
		}
		
	}
}