#include "Application.h"

#include <chrono>
#include <cstring>

#include "Camera.h"
#include "common.h"
#include "renderer.h"
#include "Log.h"
#include "ResourceManager.h"
#include "PipelineManager.h"
#include "vk_utils.h"
#include "Vertex.h"

namespace
{
	static void HandleCameraMovement(const std::shared_ptr<Cravillac::Camera>& camera, GLFWwindow* window, float deltaTime, float sensitivity);
	void HandleMouseMovement(std::shared_ptr<Cravillac::Camera>& camera, float deltaX, float deltaY, float sensitivity);
	void mouse_callback(GLFWwindow* window, double xpos, double ypos);
}
namespace Cravillac
{
	Application::Application(const char* title) : m_resourceManager(nullptr), m_window(nullptr), m_surface(VK_NULL_HANDLE)
	{
		renderer = std::make_shared<Renderer>();
		textures = std::vector<Texture>();
		m_camera = std::make_unique<Camera>();
	}
	void Application::Init()
	{
		Log::Init();

		// camera setup
		m_camera->InitAsPerspective(45.0f, WIDTH, HEIGHT);
		//m_camera->InitAsOrthographic(m_renderer->m_width, m_renderer->m_height);
		m_camera->SetPosition({ 0.0f, 0.0f, 6.f });

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

		glfwMakeContextCurrent(m_window);
		glfwSetWindowUserPointer(m_window, this);

		glfwSetCursorPosCallback(m_window, mouse_callback);

		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


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
		renderer->CreateDepthResources();

		renderer->CreateCommandPool(m_surface);

		m_resourceManager = new ResourceManager(renderer);

		// init imgui

		SetResources();
	}

	void Application::SetResources()
	{
		Model mod1;
		//mod1.LoadModel(renderer,"../../../../assets/models/suzanne/Suzanne.gltf");
		//mod1.LoadModel(renderer,"../../../../assets/models/flighthelmet/FlightHelmet.gltf");
		mod1.LoadModel(renderer, "../../../../assets/models/sponza/Sponza.gltf");
		//mod1.LoadModel(renderer,"../../../../assets/models/Cube/cube.gltf");
		models.push_back(mod1);

		MAX_TEXTURES = models[0].m_primitives.size();

		std::vector<uint32_t> matIndexArray;
		for (const auto& prim : models[0].m_primitives)
		{
			matIndexArray.push_back(prim.materialIndex);
		}

		// ubo for camera

		m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBufferMem.resize(MAX_FRAMES_IN_FLIGHT);
		m_uboMemMapped.resize(MAX_FRAMES_IN_FLIGHT);

		m_matIndexSSBO.resize(MAX_FRAMES_IN_FLIGHT);
		m_matIndexSSBOMemory.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDeviceSize bufferSize = sizeof(UniformBufferObject);
			m_uniformBuffers[i] = m_resourceManager->CreateBufferBuilder()
			                                       .setSize(bufferSize)
			                                       .setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
			                                       .setMemoryProperties(
				                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				                                       VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			                                       .build(m_uniformBufferMem[i]);
			vkMapMemory(renderer->m_device, m_uniformBufferMem[i], 0, bufferSize, 0, &m_uboMemMapped[i]);

			// material index storage buffer

			bufferSize = matIndexArray.size() * sizeof(uint32_t);
			m_matIndexSSBO[i] = m_resourceManager->CreateBufferBuilder()
			                                     .setSize(bufferSize)
			                                     .setUsage(VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT)
			                                     .setMemoryProperties(
				                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			                                     .build(m_matIndexSSBOMemory[i]);
			void* data{};

			vkMapMemory(renderer->m_device, m_matIndexSSBOMemory[i], 0, bufferSize, 0, &data);
			memcpy(data, matIndexArray.data(), bufferSize);
			vkUnmapMemory(renderer->m_device, m_matIndexSSBOMemory[i]);
		}

		// descriptor pool/sets
		std::vector<VkDescriptorPoolSize> poolSizes;

		poolSizes.resize(3);

		MAX_TEXTURES = mod1.modelTextures.size() * 2;


		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES);
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		m_resourceManager->ConfigureDescriptorPoolSizes(poolSizes, MAX_FRAMES_IN_FLIGHT * 3);

		VkDescriptorPool descPool = m_resourceManager->getDescriptorPool();
		std::vector<VkDescriptorSetLayout> descLayout;

		descLayout.resize(3);

		//Texture tex2;
		////tex1.LoadTexture(renderer, "../../../../assets/textures/cat.jpg");
		////textures.push_back(tex1);
		////tex2.LoadTexture(renderer, "../../../../assets/textures/texture.jpg");
		//tex2.LoadTexture(renderer, "../../../../assets/textures/pink.jpg");
		////tex2.LoadTexture(renderer, "../../../../assets/textures/Suzanne_BaseColor.png");
		//textures.push_back(tex2);

		descLayout[0] = m_resourceManager->getDescriptorSetLayout("ubo");
		descLayout[1] = m_resourceManager->getDescriptorSetLayout("textures");
		descLayout[2] = m_resourceManager->getDescriptorSetLayout("ssbo");

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			descriptorSets[i].resize(3);
			descriptorSets[i][0] = m_resourceManager->CreateDescriptorSet(descLayout[0]);
			descriptorSets[i][1] = m_resourceManager->CreateDescriptorSet(descLayout[1]);
			descriptorSets[i][2] = m_resourceManager->CreateDescriptorSet(descLayout[2]);
			// create two descriptor sets and update them
			// first ubo buffer
			descriptorSets[i][0] = m_resourceManager->UpdateDescriptorSet(
				descriptorSets[i][0], 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_uniformBuffers[i],
				sizeof(UniformBufferObject), std::nullopt);
			// texture descriptor
			VkBuffer buffer = VK_NULL_HANDLE;
			descriptorSets[i][1] = m_resourceManager->UpdateDescriptorSet(
				descriptorSets[i][1], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, buffer, static_cast<uint64_t>(0),
				mod1.modelTextures);
			// ssbo for material index descriptor
			descriptorSets[i][2] = m_resourceManager->UpdateDescriptorSet(
				descriptorSets[i][2], 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, m_matIndexSSBO[i],
				sizeof(uint32_t) * matIndexArray.size(),std::nullopt );
		}

		// push the ssbo once to be accessed indefinitely

		// manage pipelines

		PipelineManager* pipelineManager = m_resourceManager->getPipelineManager();

		VkVertexInputBindingDescription binding = Vertex::getBindingDescription();

		std::array<VkVertexInputAttributeDescription, 4> attributes = Vertex::getAttributeDescription();

		PipelineManager::Builder(pipelineManager)
			.setVertexShader("../../../../shaders/Triangle.vert.spv")
			.setFragmentShader("../../../../shaders/Triangle.frag.spv")
			.addDescriptorSetLayout("ubo")
			.addDescriptorSetLayout("textures")
			.addDescriptorSetLayout("ssbo")
			.setVertexInput(binding, attributes)
			.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.setDynamicStates({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
			.setDepthTest(true)
			.setBlendMode(false)
			.build("triangle");
		// cmd buffer and sync objects
		renderer->CreateCommandBuffer(m_cmdBuffers);
		renderer->CreateSynObjects(m_imageAvailableSemaphore, m_renderFinishedSemaphore, m_inFlightFence);
	}

	void Application::Run()
	{
		DrawFrame();
	}

	void Application::DrawFrame()
	{
		static float lastFrameTime = 0.f;
		while (!glfwWindowShouldClose(m_window))
		{
			glfwPollEvents();

			float currentFrameTime = static_cast<float>(glfwGetTime());
			float deltaTime = currentFrameTime - lastFrameTime;
			lastFrameTime = currentFrameTime;

			HandleCameraMovement(m_camera, m_window, deltaTime, 2.f);

			vkWaitForFences(renderer->m_device, 1, &m_inFlightFence[currentFrame], VK_TRUE, UINT64_MAX);
			vkResetFences(renderer->m_device, 1, &m_inFlightFence[currentFrame]);

			uint32_t imageIndex{};
			vkAcquireNextImageKHR(renderer->m_device, renderer->m_swapChain, UINT64_MAX, m_imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

			vkResetCommandBuffer(m_cmdBuffers[currentFrame], 0);

			RecordCmdBuffer(m_cmdBuffers[currentFrame], imageIndex, currentFrame);

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

	void Application::RecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) const
	{
		PipelineManager* pipelineManager = m_resourceManager->getPipelineManager();
		VkPipelineLayout pipelineLayout = pipelineManager->getPipelineLayout("ubo;textures;ssbo;");

		VkCommandBufferBeginInfo beginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = 0,
			.pInheritanceInfo = nullptr };

		// begin cmd buffer -> do the transition stuff, fill rendering struct -> begin rendering ->bind pipeline
		// -> bind vert/idx buffers -> bind desc sets -> draw -> end rendering -> do the remaining stuff like transition -> end command buffer

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
		{
			Log::Error("[DRAW] Recording Command buffer Failure");
		}
		// transition color image from undefined to optimal for rendering
		TransitionImage(commandBuffer, renderer->m_swapChainImages[imageIndex], VK_IMAGE_LAYOUT_UNDEFINED,
		                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		// transition depth image from undefined to optimal for rendering
		TransitionImage(commandBuffer, renderer->m_depthImage, VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

		for (const auto& prim : models[0].m_primitives)
		{
			UpdateUniformBuffer(currentFrame, prim);

			VkMemoryBarrier memoryBarrier = {};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			memoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1, &memoryBarrier, 0, nullptr, 0, nullptr);
		}

		VkRenderingAttachmentInfo colorAttachmentInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = renderer->m_swapChainImageViews[imageIndex],
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE };
		colorAttachmentInfo.clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

		VkRenderingAttachmentInfo depthAttachmentInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.pNext = nullptr,
			.imageView = renderer->m_depthImageView,
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		};

		depthAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };

		VkRenderingInfo renderingInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
		};
		renderingInfo.renderArea.offset = { .x= 0, .y= 0};
		renderingInfo.renderArea.extent = renderer->m_swapChainExtent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

		auto graphicsPipeline = pipelineManager->getPipeline("triangle");

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { models[0].m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, models[0].m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

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


		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 3, descriptorSets[currentFrame].data(), 0, nullptr);

		for (const auto& prim : models[0].m_primitives)
		{
			int materialIndex = prim.materialIndex;
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(int), &materialIndex);
			vkCmdDrawIndexed(commandBuffer, prim.indexCount, 1, prim.startIndex, prim.startVertex, 0);
		}

		vkCmdEndRendering(commandBuffer);
		// transition color image to present mode. No need for depth image, it is used directly for depth purposes, and we dont need to store or use it elsewhere (at least currently)
		TransitionImage(commandBuffer, renderer->m_swapChainImages[imageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			Log::Error("[DRAW] Recording Command buffer Failure");
		}
		// else Log::Info("[DRAW] Recording Command buffer Success");
	}


	void Application::UpdateUniformBuffer(uint32_t currentImage, const Primitive& prim) const
	{
		DirectX::XMMATRIX viewMatrix = m_camera->GetViewMatrix();
		DirectX::XMMATRIX projectionMatrix = m_camera->GetProjectionMatrix();

		DirectX::XMMATRIX worldMatrix = prim.transform.Matrix;

		DirectX::XMMATRIX worldViewProjMatrix = worldMatrix * viewMatrix * projectionMatrix;

		UniformBufferObject ubo{};

		ubo.mvp = worldViewProjMatrix;
		ubo.mWorld = worldMatrix;

		memcpy(m_uboMemMapped[currentImage], &ubo, sizeof(ubo));
	}

}

namespace 
{
	static void HandleCameraMovement(const std::shared_ptr<Cravillac::Camera>& camera, GLFWwindow* window, float deltaTime, float sensitivity)
	{
		constexpr float moveSpeed = 1.0f;

		SM::Vector3 forward = camera->GetLookAtTarget();
		forward.Normalize();

		SM::Vector3 up = camera->GetUp();
		up.Normalize();

		//might fix this later but I guess this left-right movement is more intuitive. Still will change it if needs be
		SM::Vector3 right = forward.Cross(up);
		right.Normalize();

		// Calculate movement in local space
		SM::Vector3 movement(0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		{
			//Log::Info("W key pressed");
			movement += forward * moveSpeed * deltaTime;
		}
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= forward * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= right * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += right * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) movement -= up * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) movement += up * moveSpeed * deltaTime;
		//Spar::Log::InfoDebug("movement: ", movement);
		camera->Translate(movement);
	}

	void HandleMouseMovement(std::shared_ptr<Cravillac::Camera>& camera, float deltaX, float deltaY, float sensitivity) {
		deltaX *= sensitivity;
		deltaY *= -sensitivity;

		float yaw = 0.0f;
		yaw += deltaX;
		float pitch = 0.0f;
		pitch += deltaY;

		bool constrainPitch = true;

		if (constrainPitch) {
			if (pitch > 89.0f) // Fixed to 89.0f for consistency  // NOLINT(readability-use-std-min-max)
				pitch = 89.0f;
			if (pitch < -89.0f)  // NOLINT(readability-use-std-min-max)
				pitch = -89.0f;
		}

		SM::Vector3 up = camera->GetUp();
		up.Normalize();

		camera->Rotate(up, DirectX::XMConvertToRadians(yaw));

		SM::Vector3 forward = camera->GetLookAtTarget();
		forward.Normalize();

		SM::Vector3 right = forward.Cross(up);
		right.Normalize();

		camera->Rotate(right, DirectX::XMConvertToRadians(pitch));
	}

	void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
		static bool firstMouse = true;
		static float lastX = 0.0f, lastY = 0.0f;
		static float sensitivity = 1.f; // Adjust as needed

		// Retrieve Application instance
		Cravillac::Application* app = reinterpret_cast<Cravillac::Application*>(glfwGetWindowUserPointer(window));

		if (firstMouse) {
			lastX = static_cast<float>(xpos);
			lastY = static_cast<float>(ypos);
			firstMouse = false;
		}

		float deltaX = static_cast<float>(xpos) - lastX;
		float deltaY = static_cast<float>(ypos) - lastY;

		lastX = static_cast<float>(xpos);
		lastY = static_cast<float>(ypos);

		// Pass Application's camera to HandleMouseMovement
		HandleMouseMovement(app->m_camera, deltaX, deltaY, sensitivity);
	}
}