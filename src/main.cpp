#include <pch.h>
#define GLM_ENABLE_EXPERIMENTAL

#include "common.h"
#include "renderer.h"
#include "Camera.h"
#include "ImguiRenderer.h"
#include "Model.h"
#include "Vertex.h"
#include "vk_utils.h"

namespace
{
	using Log = CV::Log;
	using Model = CV::Model;
	constexpr uint32_t WIDTH = 1920;
	constexpr uint32_t HEIGHT = 1080;

	using glm::mat4;
	using glm::mat3;
	using glm::vec2;
	using glm::vec3;
	using glm::vec4;

	struct CameraPlex
	{
		mat4 mvp;
		mat3 normalMatrix;
	};

	struct MouseState {
		glm::vec2 pos = glm::vec2(0.0f);
		bool pressedLeft = false;
	} mouseState;

	const vec3 kInitialCameraPos = vec3(0.0f, 1.0f, -1.5f);
	const vec3 kInitialCameraTarget = vec3(0.0f, 0.5f, 0.0f);

	CameraPositioner_FirstPerson positioner(kInitialCameraPos, kInitialCameraTarget, vec3(0.0f, 1.0f, 0.0f));
	Camera camera(positioner);

	bool _showDemoWindow = true;

	vec4 _clearColor = { 0.45f, 0.55f, 0.60f, 1.00f };
}

int main()
{
	camera.InitPerspective();
	Log::Init();
	const char* title = "Cravillac";

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* _window = glfwCreateWindow(WIDTH, HEIGHT, title, nullptr, nullptr);
	glfwMakeContextCurrent(_window);
	//glfwSetWindowUserPointer(window, this); //dk the use case?
	glfwFocusWindow(_window);
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// setup resource creation objects/pointers
	const std::shared_ptr<CV::Renderer> renderer = std::make_shared<CV::Renderer>();
	CV::ResourceManager* _resourceManager = new CV::ResourceManager(renderer);
	CV::PipelineManager* _pipelineManager = _resourceManager->getPipelineManager();
	renderer->InitVulkan();

	VkSurfaceKHR ret{};
	auto res = (glfwCreateWindowSurface(renderer->_instance, _window, nullptr, &ret));
	if (res != VK_SUCCESS)
		printl(Log::LogLevel::Error, "[VULKAN] GLFW Window surface");
	else printl(Log::LogLevel::Info, "[VULKAN] GLFW window surface");
	vk::SurfaceKHR _surface = vk::SurfaceKHR{ ret };

	// globally valid handles
	std::vector<vk::CommandBuffer> _cmdBuffers{ VK_NULL_HANDLE };
	// sync primitives
	std::vector<vk::Semaphore> _imageAvailableSemaphore{ VK_NULL_HANDLE };
	std::vector<vk::Semaphore> _renderFinishedSemaphore{ VK_NULL_HANDLE };
	std::vector<vk::Fence> _inFlightFence{ VK_NULL_HANDLE };
	int _currentFrame{0};

	// post surface stuff
	renderer->PickPhysicalDevice(_surface);
	renderer->CreateLogicalDevice(_surface);
	renderer->CreateSwapChain(_surface, _window);
	renderer->CreateDepthResources();
	renderer->CreateCommandPool(_surface);

	// glfw callback stuff
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	glfwSetCursorPosCallback(_window, [](auto* window, double x, double y) {
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		mouseState.pos.x = static_cast<float>(x / width);
		mouseState.pos.y = 1.0f - static_cast<float>(y / height);
		});
	glfwSetMouseButtonCallback(_window, [](auto* window, int button, int action, int mods) {
		if (button == GLFW_MOUSE_BUTTON_LEFT) {
			mouseState.pressedLeft = action == GLFW_PRESS;
		}
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		});

	glfwSetKeyCallback(_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
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
			positioner.lookAt(kInitialCameraPos, kInitialCameraTarget, vec3(0.0f, 1.0f, 0.0f));
			positioner.setSpeed(vec3(0));
		}
		});
	// set resources
	Model mod1;
	//mod1.LoadModel(renderer,"../../../../assets/models/suzanne/Suzanne.gltf");
	//mod1.LoadModel(renderer,"../../../../assets/models/flighthelmet/FlightHelmet.gltf");
	mod1.LoadModel(renderer, "../../../../assets/models/sponza2/sponza2.gltf");
	//mod1.LoadModel(renderer, "../../../../assets/models/bistro2/bistro2.gltf");
	//mod1.LoadModel(renderer,"../../../../assets/models/Cube/cube.gltf");

#if MESH_SHADING
// bda + pvp for meshlet buffer address
	vk::DeviceAddress meshBDA;
	vk::BufferDeviceAddressInfo meshletBufferAddressInfo{};
	meshletBufferAddressInfo.buffer = mod1._meshletBuffer;
	meshBDA = renderer->_device.getBufferAddress(&meshletBufferAddressInfo);
#else
		// bda + pvp get vertex buffer address
	vk::BufferDeviceAddressInfo vertexBufferAddressInfo{};
	vertexBufferAddressInfo.buffer = mod1._vertexBuffer;
	vk::DeviceAddress vertexBDA = renderer->_device.getBufferAddress(&vertexBufferAddressInfo);
#endif

	std::vector<vk::DescriptorPoolSize> poolSizes;


	MAX_TEXTURES = static_cast<uint32_t>(mod1.modelTextures.size()) * MAX_FRAMES_IN_FLIGHT * 2;	// kept it as large as possible cuz lazy

	poolSizes.push_back({vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES});

	_resourceManager->ConfigureDescriptorPoolSizes(poolSizes, MAX_FRAMES_IN_FLIGHT * poolSizes.size() * 4);
	std::vector<vk::DescriptorSetLayout> descLayout;
	descLayout.resize(1);
	descLayout[0] = _resourceManager->getDescriptorSetLayout("textures");

	std::array<std::vector<vk::DescriptorSet>, MAX_FRAMES_IN_FLIGHT> descriptorSets;

	for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		descriptorSets[i].resize(1);
		descriptorSets[i][0] = _resourceManager->CreateDescriptorSet(descLayout[0]);
		// texture descriptor
		vk::Buffer buffer{};
		descriptorSets[i][0] = _resourceManager->UpdateDescriptorSet(
			descriptorSets[i][0], 0, vk::DescriptorType::eCombinedImageSampler, buffer, static_cast<uint64_t>(0),
			mod1.modelTextures);
	}
	// manage pipelines
#if MESH_SHADING
	CV::PipelineManager::Builder(pipelineManager)
		.setMeshShader("shaders/meshlet.mesh.spv")
		.setFragmentShader("shaders/mesh.frag.spv")
		.addDescriptorSetLayout("textures")
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setDynamicStates({ vk::DynamicState::eViewport, vk::DynamicState::eScissor })
		.setDepthTest(true)
		.setBlendMode(false)
		.build("meshlet_raster");
#else
	// raster graphics pipeline
	CV::PipelineManager::Builder(_pipelineManager)
		.setVertexShader("shaders/mesh.vert.spv")
		.setFragmentShader("shaders/mesh.frag.spv")
		.addDescriptorSetLayout("textures")
		.setTopology(vk::PrimitiveTopology::eTriangleList)
		.setDynamicStates({ vk::DynamicState::eViewport, vk::DynamicState::eScissor })
		.setDepthTest(true)
		.setBlendMode(false)
		.build("mesh_raster");
#endif
	// cmd buffer and sync objects
	renderer->CreateCommandBuffer(_cmdBuffers, 2);
	renderer->CreateSynObjects(_imageAvailableSemaphore, _renderFinishedSemaphore, _inFlightFence);

	//imgui init
	CV::ImguiRenderer gui = {};
	gui.InitImgui(renderer, _window);

	auto _cameraUpdate = [&](const MeshInfo& meshInfo) -> CameraPlex
	{
		mat4 viewMatrix = positioner.getViewMatrix();
		mat4 projectionMatrix = camera.getProjMatrix();
		mat4 worldMatrix = meshInfo.transform.Matrix;
		mat4 modelView = viewMatrix * worldMatrix;
		//mat4 worldViewProjMatrix = worldMatrix * viewMatrix * projectionMatrix;
		mat4 worldViewProjMatrix = projectionMatrix * viewMatrix * worldMatrix;

		CameraPlex camMat;
		camMat.mvp = worldViewProjMatrix;
		camMat.normalMatrix = glm::transpose(glm::inverse(modelView));
		return camMat;
	};

	// lambda for record command buffer
	auto _recordCommandBuffer = [&](vk::CommandBuffer commandBuffer, uint32_t imageIndex)
		{
			CV::PipelineManager* pipelineManager = _resourceManager->getPipelineManager();
			vk::PipelineLayout pipelineLayout = pipelineManager->getPipelineLayout("textures;");

			vk::CommandBufferBeginInfo beginInfo{};
			beginInfo.flags = {};
			beginInfo.pInheritanceInfo = nullptr;

			// begin cmd buffer -> do the transition stuff, fill rendering struct -> begin rendering ->bind pipeline
			// -> bind vert/idx buffers -> bind desc sets -> draw -> end rendering
			// -> do the remaining stuff like transition -> end command buffer

			VK_ASSERT((commandBuffer.begin(&beginInfo)));
			// transition color image from undefined to optimal for rendering
			CV::TransitionImage(commandBuffer, renderer->_swapChainImages[imageIndex], vk::ImageLayout::eUndefined,
			                    vk::ImageLayout::eColorAttachmentOptimal);

			// transition depth image from undefined to optimal for rendering
			CV::TransitionImage(commandBuffer, renderer->_depthImage, vk::ImageLayout::eUndefined,
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
			colorAttachmentInfo.imageView = renderer->_swapChainImageViews[imageIndex];
			colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
			colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
			colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			colorAttachmentInfo.clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 0.0f, 1.0f);
			vk::RenderingAttachmentInfo depthAttachmentInfo{};
			depthAttachmentInfo.imageView = renderer->_depthImageView;
			depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
			depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
			depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
			depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
			vk::RenderingInfo renderingInfo{};
			renderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
			renderingInfo.renderArea.extent = renderer->_swapChainExtent;
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

			vk::Buffer vertexBuffers[] = { mod1._vertexBuffer };
			vk::DeviceSize offsets[] = { 0 };
			commandBuffer.bindVertexBuffers(0u, 1u, vertexBuffers, offsets);
			commandBuffer.bindIndexBuffer(mod1._indexBuffer, 0u, vk::IndexType::eUint32);
#endif

			vk::Viewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(renderer->_swapChainExtent.width);
			viewport.height = static_cast<float>(renderer->_swapChainExtent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			commandBuffer.setViewport(0u, 1u, &viewport);

			vk::Rect2D scissor{};
			scissor.offset = vk::Offset2D{ 0, 0 };
			scissor.extent = renderer->_swapChainExtent;
			commandBuffer.setScissor(0u, 1u, &scissor);

			commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0u, 1u,
			                                 descriptorSets[_currentFrame].data(), 0u, nullptr);

			CV::PushConstants pushConstants{};

			pushConstants.vertexBufferAddress = vertexBDA;
#if MESH_SHADING
			pushConstants.meshletBufferAddress = m_meshletBufferAddress;
#endif

			for (const auto& meshInfo : mod1._meshes) {
				const auto& material = mod1._materials[meshInfo.materialIndex];

				auto [mvp, normalMatrix] = _cameraUpdate(meshInfo);

				pushConstants.mvp = mvp;
				pushConstants.normalMatrix = normalMatrix;
				pushConstants.albedoIndex = material.albedoIndex;
				pushConstants.normalIndex = material.normalIndex;
				pushConstants.emissiveIndex = material.emmisiveIndex;
#if MESH_SHADING
				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eMeshNV | vk::ShaderStageFlagBits::eFragment,
					0, sizeof(PushConstants), &pushConstants);
				//vkCmdDrawMeshTasksEXT(commandBuffer, 32, 1, 1);
				commandBuffer.drawMeshTasksNV(models[0].m_meshlets.size(), 0);
#else
				commandBuffer.pushConstants(pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
					0, sizeof(CV::PushConstants), &pushConstants);

				commandBuffer.drawIndexed(meshInfo.indexCount, 1u, meshInfo.startIndex,
				                          static_cast<int32_t>(meshInfo.startVertex), 0u);
#endif
			}

			vkCmdEndRendering(commandBuffer);
			// transition color image to present mode. No need for depth image, it is used directly for depth purposes,
			// and we don't need to store or use it elsewhere (at least currently)
			CV::TransitionImage(commandBuffer, renderer->_swapChainImages[imageIndex],
			                    vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);

			commandBuffer.end();
		};

	// render frame loop
	static double frameTimestamp = glfwGetTime();
	static float lastFrameTime = 0.f;
	while (!glfwWindowShouldClose(_window))
	{
		double frameDelta = glfwGetTime() - frameTimestamp;
		frameTimestamp = glfwGetTime();

		glfwPollEvents();

		float currentFrameTime = static_cast<float>(glfwGetTime());
		float deltaTime = currentFrameTime - lastFrameTime;
		lastFrameTime = currentFrameTime;

		positioner.update(deltaTime, mouseState.pos, mouseState.pressedLeft);
		const glm::vec3& pos = positioner.getPosition();

		// imgui thingy
		CV::ImguiRenderer::BeginFrame();

		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &_showDemoWindow);      // Edit bools storing our window open/close state

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit3("clear color", reinterpret_cast<float*>(&_clearColor)); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			/*ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);*/
			ImGui::End();
		}

		VK_ASSERT(renderer->_device.waitForFences(1u, &_inFlightFence[_currentFrame], VK_TRUE, UINT64_MAX));
		VK_ASSERT(renderer->_device.resetFences(1u, &_inFlightFence[_currentFrame]));

		uint32_t imageIndex{};
		VK_ASSERT(
			renderer->_device.acquireNextImageKHR(renderer->_swapChain, UINT64_MAX, _imageAvailableSemaphore[
				_currentFrame], VK_NULL_HANDLE, &imageIndex));

		_cmdBuffers[_currentFrame].reset(vk::CommandBufferResetFlagBits::eReleaseResources);

		_recordCommandBuffer(_cmdBuffers[_currentFrame], imageIndex);

		vk::SubmitInfo submitInfo{};
		vk::Semaphore waitSemaphores[] = { _imageAvailableSemaphore[_currentFrame] };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		// after the fragment stage cuz the actual shading occurs after.
		// fragment stage only computes the color, doesn't actually render to the frame
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &_cmdBuffers[_currentFrame];

		vk::Semaphore signalSemaphore[] = { _renderFinishedSemaphore[imageIndex] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphore;

		VK_ASSERT(renderer->_graphicsQueue.submit(1, &submitInfo, _inFlightFence[_currentFrame]));

		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &_renderFinishedSemaphore[imageIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &renderer->_swapChain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		VK_ASSERT(renderer->_presentQueue.presentKHR(&presentInfo));

		_currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		char newTitle[256];
		snprintf(newTitle, sizeof(newTitle), "CV --- CPU time: %.2fms", frameDelta * 1000);
		glfwSetWindowTitle(_window, newTitle);
	}
}