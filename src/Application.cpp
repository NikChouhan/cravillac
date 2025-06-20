#include <pch.h>

#include "Application.h"

#include <chrono>
#include <cstring>
#include <stdio.h>

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
	void HandleMouseMovement(const std::shared_ptr<Cravillac::Camera>& camera, float deltaX, float deltaY, float sensitivity);
	void mouse_callback(GLFWwindow* window, double xpos, double ypos);
}
namespace Cravillac
{
	Application::Application() : title(nullptr), m_surface(VK_NULL_HANDLE), m_window(nullptr),
	                             m_resourceManager(nullptr),
	                             m_vertexBufferAddress(0),
	                             m_meshletBufferAddress(0) {
		renderer = std::make_shared<Renderer>();
		textures = std::vector<Texture>();
		m_camera = std::make_unique<Camera>();
	}

	void Application::Init()
	{
		title = "Cravillac";
		// camera setup
		m_camera->InitAsPerspective(45.0f, WIDTH, HEIGHT);
		//m_camera->InitAsOrthographic(m_renderer->m_width, m_renderer->m_height);
		m_camera->SetPosition({ 0.0f, 0.0f, 6.f });
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(WIDTH, HEIGHT, this->title, nullptr, nullptr);

		renderer->InitVulkan();

		glfwMakeContextCurrent(m_window);
		glfwSetWindowUserPointer(m_window, this);

		glfwSetCursorPosCallback(m_window, mouse_callback);
		glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		VkSurfaceKHR ret{};

		auto result = (glfwCreateWindowSurface(renderer->m_instance, m_window, nullptr, &ret));
		if (result != VK_SUCCESS)
			Log::Error("[VULKAN] Failed to initialise GLFW window surface");
		else Log::Info("[VULKAN] Initialised GLFW window surface");
		m_surface = vk::SurfaceKHR{ ret };
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
		//mod1.LoadModel(renderer, "../../../../assets/models/bistrogodot/bistrogodot.gltf");
		//mod1.LoadModel(renderer,"../../../../assets/models/Cube/cube.gltf");
		models.push_back(mod1);

#if MESH_SHADING
		// bda + pvp for meshlet buffer address
		vk::BufferDeviceAddressInfo meshletBufferAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
		meshletBufferAddressInfo.buffer = models[0].m_meshletBuffer;
		m_meshletBufferAddress = vkGetBufferDeviceAddress(renderer->m_device, &meshletBufferAddressInfo);
#else
		// bda + pvp get vertex buffer address
		vk::BufferDeviceAddressInfo vertexBufferAddressInfo{};
		vertexBufferAddressInfo.buffer = models[0].m_vertexBuffer;
		m_vertexBufferAddress = renderer->m_device.getBufferAddress(&vertexBufferAddressInfo);
#endif
		// descriptor pool/sets
		std::vector<vk::DescriptorPoolSize> poolSizes;

		poolSizes.resize(1);

		MAX_TEXTURES = static_cast<uint32_t>(mod1.modelTextures.size()) * MAX_FRAMES_IN_FLIGHT * 2;	// kept it as large as possible cuz lazy

		poolSizes[0].type = vk::DescriptorType::eCombinedImageSampler;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES);

		m_resourceManager->ConfigureDescriptorPoolSizes(poolSizes, MAX_FRAMES_IN_FLIGHT * poolSizes.size() *4);

		std::vector<vk::DescriptorSetLayout> descLayout;

		descLayout.resize(1);

		descLayout[0] = m_resourceManager->getDescriptorSetLayout("textures");

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			descriptorSets[i].resize(1);
			descriptorSets[i][0] = m_resourceManager->CreateDescriptorSet(descLayout[0]);
			// texture descriptor
			vk::Buffer buffer{};
			descriptorSets[i][0] = m_resourceManager->UpdateDescriptorSet(
				descriptorSets[i][0], 0, vk::DescriptorType::eCombinedImageSampler, buffer, static_cast<uint64_t>(0),
				mod1.modelTextures);
		}
		// manage pipelines
		PipelineManager* pipelineManager = m_resourceManager->getPipelineManager();
#if MESH_SHADING
		PipelineManager::Builder(pipelineManager)
			.setMeshShader("shaders/meshlet.mesh.spv")
			.setFragmentShader("shaders/mesh.frag.spv")
			.addDescriptorSetLayout("textures")
			.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.setDynamicStates({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
			.setDepthTest(true)
			.setBlendMode(false)
			.build("meshlet_raster");
#else
		// raster graphics pipeline
		PipelineManager::Builder(pipelineManager)
			.setVertexShader("shaders/mesh.vert.spv")
			.setFragmentShader("shaders/mesh.frag.spv")
			.addDescriptorSetLayout("textures")
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setDynamicStates({vk::DynamicState::eViewport, vk::DynamicState::eScissor})
			.setDepthTest(true)
			.setBlendMode(false)
			.build("mesh_raster");
#endif
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
		static double frameTimestamp = glfwGetTime();
		static float lastFrameTime = 0.f;
		while (!glfwWindowShouldClose(m_window))
		{
			double frameDelta = glfwGetTime() - frameTimestamp;
			frameTimestamp = glfwGetTime();

			glfwPollEvents();

			float currentFrameTime = static_cast<float>(glfwGetTime());
			float deltaTime = currentFrameTime - lastFrameTime;
			lastFrameTime = currentFrameTime;

			float sensitivity = 0.2f;
			HandleCameraMovement(m_camera, m_window, deltaTime, sensitivity);

		 	VK_ASSERT(renderer->m_device.waitForFences(1u, &m_inFlightFence[m_currentFrame], VK_TRUE, UINT64_MAX));
			VK_ASSERT(renderer->m_device.resetFences(1u, &m_inFlightFence[m_currentFrame]));

			uint32_t imageIndex{};
			VK_ASSERT(renderer->m_device.acquireNextImageKHR(renderer->m_swapChain, UINT64_MAX, m_imageAvailableSemaphore[m_currentFrame], VK_NULL_HANDLE, &imageIndex));

			m_cmdBuffers[m_currentFrame].reset(vk::CommandBufferResetFlagBits::eReleaseResources);

			RecordCmdBuffer(m_cmdBuffers[m_currentFrame], imageIndex, m_currentFrame);

			vk::SubmitInfo submitInfo{};
			vk::Semaphore waitSemaphores[] = { m_imageAvailableSemaphore[m_currentFrame] };
			vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
			// after the fragment stage cuz the actual shading occurs after. fragment stage only computes the color, doesn't actually render to the frame
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_cmdBuffers[m_currentFrame];

			vk::Semaphore signalSemaphore[] = { m_renderFinishedSemaphore[m_currentFrame] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphore;

			VK_ASSERT(renderer->m_graphicsQueue.submit(1, &submitInfo, m_inFlightFence[m_currentFrame]));

			vk::PresentInfoKHR presentInfo{};
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &m_renderFinishedSemaphore[m_currentFrame];
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &renderer->m_swapChain;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;

			VK_ASSERT(renderer->m_presentQueue.presentKHR(&presentInfo));

			m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
			char newTitle[256];

			snprintf(newTitle,sizeof(newTitle), "Cravillac --- CPU time: %.2fms", frameDelta*1000);

			glfwSetWindowTitle(m_window, newTitle);
		}
		vkDeviceWaitIdle(renderer->m_device);
	}

	void Application::RecordCmdBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) const
	{
		PipelineManager* pipelineManager = m_resourceManager->getPipelineManager();
		vk::PipelineLayout pipelineLayout = pipelineManager->getPipelineLayout("textures;");

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = {};
		beginInfo.pInheritanceInfo = nullptr;

		// begin cmd buffer -> do the transition stuff, fill rendering struct -> begin rendering ->bind pipeline
		// -> bind vert/idx buffers -> bind desc sets -> draw -> end rendering -> do the remaining stuff like transition -> end command buffer

		VK_ASSERT((commandBuffer.begin(&beginInfo)));
		// transition color image from undefined to optimal for rendering
		TransitionImage(commandBuffer, renderer->m_swapChainImages[imageIndex],  vk::ImageLayout::eUndefined,
		               vk::ImageLayout::eColorAttachmentOptimal);

		// transition depth image from undefined to optimal for rendering
		TransitionImage(commandBuffer, renderer->m_depthImage, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthAttachmentOptimal);
		// likely redundant cuz ubo sent to the ends
		// for (const auto& meshInfo : models[0].m_meshitives)
		// {
		// 	UpdateUniformBuffer(currentFrame, meshInfo);
		//
		// 	vk::MemoryBarrier memoryBarrier = {};
		// 	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		// 	memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		// 	memoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
		// 	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1,
		// 	                     &memoryBarrier, 0, nullptr, 0, nullptr);
		// }

		vk::RenderingAttachmentInfo colorAttachmentInfo{};
		colorAttachmentInfo.imageView = renderer->m_swapChainImageViews[imageIndex];
		colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachmentInfo.clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
		vk::RenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.imageView = renderer->m_depthImageView;
		depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = vk::Offset2D{ 0, 0};
		renderingInfo.renderArea.extent = renderer->m_swapChainExtent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;
#if MESH_SHADING
		auto meshPipeline = pipelineManager->getPipeline("meshlet_raster");
		commandBuffer.beginRendering(&renderingInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, meshPipeline);
#else
		auto graphicsPipeline = pipelineManager->getPipeline("mesh_raster");

		commandBuffer.beginRendering(&renderingInfo);
	 	commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

		vk::Buffer vertexBuffers[] = { models[0].m_vertexBuffer };
		vk::DeviceSize offsets[] = { 0 };
		commandBuffer.bindVertexBuffers(0u, 1u, vertexBuffers, offsets);
		commandBuffer.bindIndexBuffer(models[0].m_indexBuffer, 0u, vk::IndexType::eUint32);
#endif

		vk::Viewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(renderer->m_swapChainExtent.width);
		viewport.height = static_cast<float>(renderer->m_swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		commandBuffer.setViewport(0u, 1u, &viewport);

		vk::Rect2D scissor{};
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = renderer->m_swapChainExtent;
		commandBuffer.setScissor(0u, 1u, &scissor);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0u, 1u, descriptorSets[currentFrame].data(), 0u, nullptr);


		PushConstants pushConstants{};

		pushConstants.vertexBufferAddress = m_vertexBufferAddress;
#if MESH_SHADING
		pushConstants.meshletBufferAddress = m_meshletBufferAddress;
#endif

		for (const auto& meshInfo : models[0].m_meshes) {
			auto [mvp, normalMatrix] = UpdateUniformBuffer(meshInfo);
			uint32_t materialIndex = meshInfo.materialIndex;

			pushConstants.mvp = mvp;
			pushConstants.normalMatrix = normalMatrix;
			pushConstants.materialIndex = materialIndex;
#if MESH_SHADING
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_MESH_BIT_NV | VK_SHADER_STAGE_FRAGMENT_BIT,
				0, sizeof(PushConstants), &pushConstants);
			vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
#else
			commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
			                   0, sizeof(PushConstants), &pushConstants);

			commandBuffer.drawIndexed(meshInfo.indexCount, 1u, meshInfo.startIndex, static_cast<int32_t>(meshInfo.startVertex), 0u);
#endif
		}

		vkCmdEndRendering(commandBuffer);
		// transition color image to present mode. No need for depth image, it is used directly for depth purposes,
		// and we don't need to store or use it elsewhere (at least currently)
		TransitionImage(commandBuffer, renderer->m_swapChainImages[imageIndex],
		               vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

		commandBuffer.end();
	}

	UniformBufferObject Application::UpdateUniformBuffer(const MeshInfo& meshInfo) const
	{
		DirectX::XMMATRIX viewMatrix = m_camera->GetViewMatrix();
		DirectX::XMMATRIX projectionMatrix = m_camera->GetProjectionMatrix();

		DirectX::XMMATRIX worldMatrix = meshInfo.transform.Matrix;

		DirectX::XMFLOAT3X3 normalMatrix = meshInfo.normalMatrix;

		DirectX::XMMATRIX worldViewProjMatrix = worldMatrix * viewMatrix * projectionMatrix;

		UniformBufferObject ubo{};

		ubo.mvp = worldViewProjMatrix;
		ubo.normalMatrix = normalMatrix;

		return ubo;
	}

	vk::Device Application::GetDevice()
	{
		return renderer->m_device;
	}
}

namespace 
{
	static void HandleCameraMovement(const std::shared_ptr<Cravillac::Camera>& camera, GLFWwindow* window, float deltaTime, float sensitivity)
	{
		float moveSpeed = sensitivity * 5;

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

	void HandleMouseMovement(const std::shared_ptr<Cravillac::Camera>& camera, float deltaX, float deltaY, float sensitivity) {
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