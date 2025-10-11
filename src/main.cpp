#include <pch.h>
#define GLM_ENABLE_EXPERIMENTAL


#include "GfxDevice.h"
#include "Camera.h"
#include "Descriptor.h"
#include "Model.h"
#include "Pipeline.h"
#include "Swapchain.h"
#include "Vertex.h"

namespace
{
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
	// glfw callback stuff
	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

	GfxDevice gfxDevice = CreateDevice(_window,
		{ ._bEnableValidationLayers = true });
	
	int currentFrame{ 0 };

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

	Swapchain swapChain;
	Texture depthTexture;

	auto InitialiseSwapchainTextures = [&]()
		{
			swapChain = CreateSwapchain(gfxDevice, _window,
				{
				._vsyncOption = true,
				._preferresSWImageCount = 2,
				._oldSwapchain = nullptr });
			depthTexture = CreateTexture(gfxDevice,
				{
				._width = swapChain._extent.width,
				._height = swapChain._extent.height,
				._mipCount = 1,
				._format = vk::Format::eD32Sfloat ,
				._usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
				._layout = vk::ImageLayout::eDepthAttachmentOptimal,
				/*._access = vk::AccessFlagBits::eDepthStencilAttachmentWrite,
				._sampler = {
					._filterMode = vk::Filter::eLinear,
					._reductionMode = vk::SamplerReductionMode::eMin
				},*/
				._resource = nullptr,
				._copyBuffer = nullptr });
		};

	InitialiseSwapchainTextures();

	FrameSync frameSync = CreateFrameSync(gfxDevice, swapChain._textures.size(), { ._isTrue = true });

	const char* vShaderPath = "shaders/mesh.vert.spv";
	const char* fShaderPath = "shaders/mesh.frag.spv";

	Shader vertexShader = CreateShader(gfxDevice,
		{
		.path = vShaderPath,
		.pEntry = "main" });
	Shader fragShader = CreateShader(gfxDevice,
		{
		.path = fShaderPath,
		.pEntry = "main" });

	// set resources
	const std::string modelPath = "../../../../assets/models/bistro2/bistro2.gltf";
	Model mod1 = LoadModel(gfxDevice, frameSync,
		{
		._path = modelPath });

	DescriptorPool descriptorPool = CreateDescriptorPool(gfxDevice,
		{
		._poolSizes = {{vk::DescriptorType::eCombinedImageSampler, MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES}},
		._flags = vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind,
		._maxSets = MAX_FRAMES_IN_FLIGHT * 4 });

	// make it such that you pass shaders, and it creates the sets implicitly
	// or rather just automate it without the explicit mention of sets and pools and bindings later

	Pipeline graphicsPipeline = CreateGraphicsPipeline(gfxDevice,
		{
		._shaders = {
			vertexShader, fragShader
		},
		._attachmentLayout = {
			._colorAttachments = {
				{
					._format = swapChain._format,
					._bBlendEnable = false
				}
			},
			._depthStencilFormat = {
				depthTexture._format
			}
			},
		._rasterizationDesc = {
			{vk::CullModeFlagBits::eBack},
			{vk::FrontFace::eCounterClockwise}
		},
		._depthStencilDesc = {
			._bDepthTestEnable = true,
			._bDepthWriteEnable = true
		} });

	Descriptor descriptor = CreateDescriptorSet(gfxDevice, descriptorPool, graphicsPipeline,
		{
		._set = 0,
		._binding = fragShader._layoutBindings[0].binding,
		._type = fragShader._layoutBindings[0].descriptorType,
		._descriptorCount = fragShader._layoutBindings[0].descriptorCount,
		._stageFlags = fragShader._stage,
		._textures = mod1._modelTextures});

#if MESH_SHADING
	// bda + pvp for meshlet buffer address
	vk::DeviceAddress meshBDA;
	vk::BufferDeviceAddressInfo meshletBufferAddressInfo{};
	meshletBufferAddressInfo.buffer = mod1._meshletBuffer;
	meshBDA = renderer->_device.getBufferAddress(&meshletBufferAddressInfo);
#else
	// bda + pvp get vertex buffer address
	vk::BufferDeviceAddressInfo vertexBufferAddressInfo{};
	vertexBufferAddressInfo.buffer = mod1._vertexBuffer._resource;
	vk::DeviceAddress vertexBDA = gfxDevice._device.getBufferAddress(&vertexBufferAddressInfo);
#endif

	std::vector<vk::CommandBuffer> commandBuffers = CreateCommandBuffer(gfxDevice, 2);

	auto cameraUpdate = [&](const MeshInfo& meshInfo) -> CameraPlex
	{
		mat4 viewMatrix = positioner.getViewMatrix();
		mat4 projectionMatrix = camera.getProjMatrix();
		mat4 worldMatrix = meshInfo._transform.Matrix;
		mat4 modelView = viewMatrix * worldMatrix;
		//mat4 worldViewProjMatrix = worldMatrix * viewMatrix * projectionMatrix;
		mat4 worldViewProjMatrix = projectionMatrix * viewMatrix * worldMatrix;

		CameraPlex camMat;
		camMat.mvp = worldViewProjMatrix;
		camMat.normalMatrix = glm::transpose(glm::inverse(modelView));
		return camMat;
	};

	auto recordCommandBuffer = [&](vk::CommandBuffer commandBuffer, u32 imageIndex)
	{
		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = {};
		beginInfo.pInheritanceInfo = nullptr;

		VK_ASSERT(commandBuffer.begin(&beginInfo));

		TextureBarrier(commandBuffer, swapChain._textures[imageIndex], vk::ImageLayout::eUndefined,
			vk::ImageLayout::eColorAttachmentOptimal, vk::AccessFlagBits::eNone,
			vk::AccessFlagBits::eColorAttachmentWrite);

		TextureBarrier(commandBuffer, depthTexture, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eDepthAttachmentOptimal, vk::AccessFlagBits::eNone,
			vk::AccessFlagBits::eDepthStencilAttachmentWrite);

		vk::RenderingAttachmentInfo colorAttachmentInfo{};
		colorAttachmentInfo.imageView = swapChain._textures[imageIndex]._imageView;
		colorAttachmentInfo.imageLayout = vk::ImageLayout::eColorAttachmentOptimal;
		colorAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachmentInfo.clearValue.color = vk::ClearColorValue(0.0f, 0.0f, 1.0f, 1.0f);
		vk::RenderingAttachmentInfo depthAttachmentInfo{};
		depthAttachmentInfo.imageView = depthTexture._imageView;
		depthAttachmentInfo.imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
		depthAttachmentInfo.loadOp = vk::AttachmentLoadOp::eClear;
		depthAttachmentInfo.storeOp = vk::AttachmentStoreOp::eStore;
		depthAttachmentInfo.clearValue.depthStencil = vk::ClearDepthStencilValue(1.0f, 0);
		vk::RenderingInfo renderingInfo{};
		renderingInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
		renderingInfo.renderArea.extent = swapChain._extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachmentInfo;
		renderingInfo.pDepthAttachment = &depthAttachmentInfo;

		commandBuffer.beginRendering(&renderingInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline._pipeline);

		vk::Buffer vertexBuffers[] = { mod1._vertexBuffer._resource };
		vk::DeviceSize offsets[] = { 0 };
		commandBuffer.bindVertexBuffers(0u, 1u, vertexBuffers, offsets);
		commandBuffer.bindIndexBuffer(mod1._indexBuffer._resource, 0u, vk::IndexType::eUint32);

		vk::Viewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChain._extent.width);
		viewport.height = static_cast<float>(swapChain._extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		commandBuffer.setViewport(0u, 1u, &viewport);

		vk::Rect2D scissor{};
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = swapChain._extent;
		commandBuffer.setScissor(0u, 1u, &scissor);

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, graphicsPipeline._pipelineLayout,
			0, 1, &descriptor._descriptorSet, 0, nullptr);

		PushConstants pushConstants;

		pushConstants.vertexBufferAddress = vertexBDA;

		for (const auto& meshInfo : mod1._meshes) {
			const auto& material = mod1._materials[meshInfo._materialIndex];

			auto [mvp, normalMatrix] = cameraUpdate(meshInfo);

			pushConstants.mvp = mvp;
			pushConstants.normalMatrix = normalMatrix;
			pushConstants.albedoIndex = material._albedoIndex;
			pushConstants.normalIndex = material._normalIndex;
			pushConstants.emissiveIndex = material._emmisiveIndex;

			commandBuffer.pushConstants(graphicsPipeline._pipelineLayout, vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
				0, sizeof(PushConstants), &pushConstants);

			commandBuffer.drawIndexed(meshInfo._indexCount, 1u, meshInfo._startIndex,
				static_cast<int32_t>(meshInfo._startVertex), 0u);
		}

		commandBuffer.endRendering();

		TextureBarrier(commandBuffer, swapChain._textures[imageIndex], vk::ImageLayout::eColorAttachmentOptimal,
			vk::ImageLayout::ePresentSrcKHR, vk::AccessFlagBits::eColorAttachmentWrite,
			{}, vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::PipelineStageFlagBits::eNone);
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

		// vulkan things

		VK_ASSERT(gfxDevice._device.waitForFences(1u, &frameSync._inFlightFences[currentFrame], VK_TRUE, UINT64_MAX));
		VK_ASSERT(gfxDevice._device.resetFences(1u, &frameSync._inFlightFences[currentFrame]));

		uint32_t imageIndex{};
		VK_ASSERT(
			gfxDevice._device.acquireNextImageKHR(swapChain._swapchain, UINT64_MAX, 
				frameSync._imgAvailableSem[currentFrame], VK_NULL_HANDLE, &imageIndex));

		commandBuffers[currentFrame].reset(vk::CommandBufferResetFlagBits::eReleaseResources);

		recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

		vk::SubmitInfo submitInfo{};
		vk::Semaphore waitSemaphores[] = { frameSync._imgAvailableSem[currentFrame]};
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		// after the fragment stage cuz the actual shading occurs after.
		// fragment stage only computes the color, doesn't actually render to the frame
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		vk::Semaphore signalSemaphore[] = { frameSync._renderFinishedSem[imageIndex]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphore;

		VK_ASSERT(gfxDevice._graphicsQueue._queue.submit(1, &submitInfo, frameSync._inFlightFences[currentFrame]));

		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &frameSync._renderFinishedSem[imageIndex];
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain._swapchain;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		VK_ASSERT(gfxDevice._graphicsQueue._queue.presentKHR(&presentInfo));

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		char newTitle[256];
		snprintf(newTitle, sizeof(newTitle), "CV --- CPU time: %.2fms", frameDelta * 1000);
		glfwSetWindowTitle(_window, newTitle);
	}
}