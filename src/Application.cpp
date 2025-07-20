#include <pch.h>

#include "Application.h"

#include <chrono>
#include <complex>
#include <cstring>
#include <cstdio>

#include "Camera.h"
#include "common.h"
#include "renderer.h"
#include "Log.h"
#include "ResourceManager.h"
#include "PipelineManager.h"
#include "vk_utils.h"
#include "Vertex.h"

struct MouseState {
	SM::Vector2 pos = SM::Vector2(0.0f);
	bool pressedLeft = false;
} mouseState;

constexpr float ratio = 16. / 9.;

constexpr SM::Vector3 kInitialCameraPos = SM::Vector3{ (0.0f, 1.0f, -1.5f) };
constexpr SM::Vector3 kInitialCameraTarget{ (0.0f, 0.5f, 0.0f) };

CameraPositioner_FirstPerson positioner(kInitialCameraPos, kInitialCameraTarget, SM::Vector3(0.0f, 1.0f, 0.0f));
Camera m_camera(positioner);
namespace Cravillac
{
	Application::Application() : title(nullptr), m_surface(VK_NULL_HANDLE), m_window(nullptr),
	                             m_resourceManager(nullptr),
	                             m_vertexBufferAddress(0),
	                             m_meshletBufferAddress(0) {
		renderer = std::make_shared<Renderer>();
		textures = std::vector<Texture>();
	}

	void Application::Init()
 	{
		m_camera.InitPerspective();
		Log::Init();
		title = "Cravillac";
		// camera setup
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_window = glfwCreateWindow(WIDTH, HEIGHT, this->title, nullptr, nullptr);

		renderer->InitVulkan();

		glfwMakeContextCurrent(m_window);
		glfwSetWindowUserPointer(m_window, this);
		glfwFocusWindow(m_window);
		//glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		VkSurfaceKHR ret{};

		auto result = (glfwCreateWindowSurface(renderer->m_instance, m_window, nullptr, &ret));
		if (result != VK_SUCCESS)
			printl(Log::LogLevel::Error,"[VULKAN] GLFW Window surface");
		else printl(Log::LogLevel::Info,"[VULKAN] GLFW window surface");
		m_surface = vk::SurfaceKHR{ ret };
		// post surface stuff
		renderer->PickPhysicalDevice(m_surface);
		renderer->CreateLogicalDevice(m_surface);
		renderer->CreateSwapChain(m_surface, m_window);
		renderer->CreateDepthResources();
		renderer->CreateCommandPool(m_surface);

		glfwSetInputMode(m_window,GLFW_CURSOR, GLFW_CURSOR_NORMAL);

		glfwSetCursorPosCallback(m_window, [](auto* window, double x, double y) {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			mouseState.pos.x = static_cast<float>(x / width);
			mouseState.pos.y = 1.0f - static_cast<float>(y / height);
			});
		glfwSetMouseButtonCallback(m_window, [](auto* window, int button, int action, int mods) {
			if (button == GLFW_MOUSE_BUTTON_LEFT) {
				printl(Log::LogLevel::Info, "Yes its hit\n");
				mouseState.pressedLeft = action == GLFW_PRESS;
			}
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			// ...and reset the positioner's internal state to prevent a jump.
			positioner.resetMousePosition({
				static_cast<float>(xpos / width),
				1.0f - static_cast<float>(ypos / height)
				});
			});

		glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
			const bool pressed = action != GLFW_RELEASE;
			if (key == GLFW_KEY_ESCAPE && pressed)
				glfwSetWindowShouldClose(window, GLFW_TRUE);
			if (key == GLFW_KEY_W)
				positioner.movement_.forward_ = pressed;
			if (key == GLFW_KEY_S)
				positioner.movement_.backward_ = pressed;
			if (key == GLFW_KEY_A)
				positioner.movement_.left_ = pressed;
			if (key == GLFW_KEY_D)
				positioner.movement_.right_ = pressed;
			if (key == GLFW_KEY_1)
				positioner.movement_.up_ = pressed;
			if (key == GLFW_KEY_2)
				positioner.movement_.down_ = pressed;
			if (mods & GLFW_MOD_SHIFT)
				positioner.movement_.fastSpeed_ = pressed;
			if (key == GLFW_KEY_SPACE) {
				positioner.lookAt(kInitialCameraPos, kInitialCameraTarget, SM::Vector3(0.0f, 1.0f, 0.0f));
				positioner.setSpeed(SM::Vector3{ 0.,0.,0. });
			}
			});

		/*
		* TODO: remove this bs and replace with custom allocator. Refactor the code to separate Resources into their own structs
		* like Buffers/Samplers/Textures, etc and then have a build sorta function that takes in designated initializers structs
		* to build them. Remove the stupid classes and pointers with tangled lifetime. Just clean it all with the ArenaFree.
		*/
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
		//mod1.LoadModel(renderer, "../../../../assets/models/bistro/Bistro.glb");
		//mod1.LoadModel(renderer,"../../../../assets/models/Cube/cube.gltf");
		models.push_back(mod1);

#if MESH_SHADING
		// bda + pvp for meshlet buffer address
		vk::BufferDeviceAddressInfo meshletBufferAddressInfo{};
		meshletBufferAddressInfo.buffer = models[0].m_meshletBuffer;
		m_meshletBufferAddress = renderer->m_device.getBufferAddress(&meshletBufferAddressInfo);
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
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setDynamicStates({vk::DynamicState::eViewport, vk::DynamicState::eScissor})
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

			positioner.update(deltaTime, mouseState.pos, mouseState.pressedLeft);
			const SM::Vector3& pos = positioner.getPosition();
			SM::Vector4 cameraPos = SM::Vector4(pos.x, pos.y, pos.z, 1.0f);

			PushConstants pc;
		 	VK_ASSERT(renderer->m_device.waitForFences(1u, &m_inFlightFence[m_currentFrame], VK_TRUE, UINT64_MAX));
			VK_ASSERT(renderer->m_device.resetFences(1u, &m_inFlightFence[m_currentFrame]));

			uint32_t imageIndex{};
			VK_ASSERT(renderer->m_device.acquireNextImageKHR(renderer->m_swapChain, UINT64_MAX, m_imageAvailableSemaphore[m_currentFrame], VK_NULL_HANDLE, &imageIndex));

			m_cmdBuffers[m_currentFrame].reset(vk::CommandBufferResetFlagBits::eReleaseResources);

			RecordCmdBuffer(m_cmdBuffers[m_currentFrame], imageIndex, pc);

			vk::SubmitInfo submitInfo{};
			vk::Semaphore waitSemaphores[] = { m_imageAvailableSemaphore[m_currentFrame] };
			vk::PipelineStageFlags waitStages[] = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
			// after the fragment stage cuz the actual shading occurs after. fragment stage only computes the color, doesn't actually render to the frame
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_cmdBuffers[m_currentFrame];

			vk::Semaphore signalSemaphore[] = { m_renderFinishedSemaphore[imageIndex] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphore;

			VK_ASSERT(renderer->m_graphicsQueue.submit(1, &submitInfo, m_inFlightFence[m_currentFrame]));

			vk::PresentInfoKHR presentInfo{};
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &m_renderFinishedSemaphore[imageIndex];
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
	}

	void Application::RecordCmdBuffer(vk::CommandBuffer commandBuffer, uint32_t imageIndex, PushConstants& pc) const
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
			vk::ImageLayout::eDepthStencilAttachmentOptimal);
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

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0u, 1u, descriptorSets[m_currentFrame].data(), 0u, nullptr);


		PushConstants pushConstants{};

		pushConstants.vertexBufferAddress = m_vertexBufferAddress;
#if MESH_SHADING
		pushConstants.meshletBufferAddress = m_meshletBufferAddress;
#endif

		for (const auto& meshInfo : models[0].m_meshes) {
			auto [mvp, normalMatrix] = CameraUpdate(meshInfo);
			uint32_t materialIndex = meshInfo.materialIndex;

			pushConstants.mvp = mvp;
			pushConstants.normalMatrix = normalMatrix;
			pushConstants.materialIndex = materialIndex;
#if MESH_SHADING
			commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eMeshNV | vk::ShaderStageFlagBits::eFragment,
				0, sizeof(PushConstants), &pushConstants);
			//vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
			commandBuffer.drawMeshTasksNV(models[0].m_meshlets.size(), 0);
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

	UniformBufferObject Application::CameraUpdate(const MeshInfo& meshInfo) const
	{
		DirectX::XMMATRIX viewMatrix = positioner.getViewMatrix();
		DirectX::XMMATRIX projectionMatrix = m_camera.getProjMatrix();

		DirectX::XMMATRIX worldMatrix = meshInfo.transform.Matrix;

		DirectX::XMMATRIX normalMatrix = meshInfo.normalMatrix;

		//DirectX::XMMATRIX worldViewProjMatrix = projectionMatrix * viewMatrix * worldMatrix;
		DirectX::XMMATRIX worldViewProjMatrix = worldMatrix * viewMatrix * projectionMatrix;

		UniformBufferObject ubo{};

		//ubo.mvp = DirectX::XMMatrixTranspose(worldViewProjMatrix);
		ubo.mvp = (worldViewProjMatrix);
		//DirectX::XMStoreFloat3x3(&ubo.normalMatrix, DirectX::XMMatrixTranspose(normalMatrix));
		DirectX::XMStoreFloat3x3(&ubo.normalMatrix, (DirectX::XMMATRIX(normalMatrix)));
		return ubo;
	}

	vk::Device Application::GetDevice()
	{
		return renderer->m_device;
	}
}