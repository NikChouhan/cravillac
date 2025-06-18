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
		Log::Init();
		if (volkInitialize() == VK_SUCCESS)
		{
			Log::Info("[VULKAN] Volk works");
		}
		else 
		{
			Log::Error("[VULKAN] Volk fails!");
			return;
		}
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
		volkLoadInstance(renderer->m_instance);

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
		// load all entrypoints directly from driver
		volkLoadDevice(renderer->m_device);

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
		VkBufferDeviceAddressInfo meshletBufferAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
		meshletBufferAddressInfo.buffer = models[0].m_meshletBuffer;
		m_meshletBufferAddress = vkGetBufferDeviceAddress(renderer->m_device, &meshletBufferAddressInfo);
#else
		// bda + pvp get vertex buffer address
		VkBufferDeviceAddressInfo vertexBufferAddressInfo {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO};
		vertexBufferAddressInfo.buffer = models[0].m_vertexBuffer;
		m_vertexBufferAddress = vkGetBufferDeviceAddress(renderer->m_device, &vertexBufferAddressInfo);
#endif
		// descriptor pool/sets
		std::vector<VkDescriptorPoolSize> poolSizes;

		poolSizes.resize(1);

		MAX_TEXTURES = static_cast<uint32_t>(mod1.modelTextures.size()) * MAX_FRAMES_IN_FLIGHT * 2;	// kept it as large as possible cuz lazy

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT * MAX_TEXTURES);

		m_resourceManager->ConfigureDescriptorPoolSizes(poolSizes, MAX_FRAMES_IN_FLIGHT * poolSizes.size() *4);

		std::vector<VkDescriptorSetLayout> descLayout;

		descLayout.resize(1);

		descLayout[0] = m_resourceManager->getDescriptorSetLayout("textures");

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			descriptorSets[i].resize(1);
			descriptorSets[i][0] = m_resourceManager->CreateDescriptorSet(descLayout[0]);
			// texture descriptor
			VkBuffer buffer = VK_NULL_HANDLE;
			descriptorSets[i][0] = m_resourceManager->UpdateDescriptorSet(
				descriptorSets[i][0], 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, buffer, static_cast<uint64_t>(0),
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
			.setTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
			.setDynamicStates({VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR})
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

			vkWaitForFences(renderer->m_device, 1, &m_inFlightFence[m_currentFrame], VK_TRUE, UINT64_MAX);
			vkResetFences(renderer->m_device, 1, &m_inFlightFence[m_currentFrame]);

			uint32_t imageIndex{};
			vkAcquireNextImageKHR(renderer->m_device, renderer->m_swapChain, UINT64_MAX, m_imageAvailableSemaphore[m_currentFrame], VK_NULL_HANDLE, &imageIndex);

			vkResetCommandBuffer(m_cmdBuffers[m_currentFrame], 0);

			RecordCmdBuffer(m_cmdBuffers[m_currentFrame], imageIndex, m_currentFrame);

			VkSubmitInfo submitInfo{
				.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO };
			VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore[m_currentFrame] };
			VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
			// after the fragment stage cuz the actual shading occurs after. fragment stage only computes the color, doesn't actually render to the frame
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = waitSemaphores;
			submitInfo.pWaitDstStageMask = waitStages;

			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &m_cmdBuffers[m_currentFrame];

			VkSemaphore signalSemaphore[] = { m_renderFinishedSemaphore[m_currentFrame] };
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = signalSemaphore;

			if (vkQueueSubmit(renderer->m_graphicsQueue, 1, &submitInfo, m_inFlightFence[m_currentFrame]) != VK_SUCCESS)
			{
				Log::Error("[DRAW] Submit Draw Command buffer Failed");
			}
			// else Log::Info("[DRAW] Submit Draw Command buffer Success");

			VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &m_renderFinishedSemaphore[m_currentFrame];
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &renderer->m_swapChain;
			presentInfo.pImageIndices = &imageIndex;
			presentInfo.pResults = nullptr;

			vkQueuePresentKHR(renderer->m_presentQueue, &presentInfo);

			m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
			char newTitle[256];

			snprintf(newTitle,sizeof(newTitle), "Cravillac --- CPU time: %.2fms", frameDelta*1000);

			glfwSetWindowTitle(m_window, newTitle);
		}
		vkDeviceWaitIdle(renderer->m_device);
	}

	void Application::RecordCmdBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t currentFrame) const
	{
		PipelineManager* pipelineManager = m_resourceManager->getPipelineManager();
		VkPipelineLayout pipelineLayout = pipelineManager->getPipelineLayout("textures;");

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
		// likely redundant cuz ubo sent to the ends
		// for (const auto& meshInfo : models[0].m_meshitives)
		// {
		// 	UpdateUniformBuffer(currentFrame, meshInfo);
		//
		// 	VkMemoryBarrier memoryBarrier = {};
		// 	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		// 	memoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
		// 	memoryBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
		// 	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 1,
		// 	                     &memoryBarrier, 0, nullptr, 0, nullptr);
		// }

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
#if MESH_SHADING
		auto meshPipeline = pipelineManager->getPipeline("meshlet_raster");
		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipeline);
#else
		auto graphicsPipeline = pipelineManager->getPipeline("mesh_raster");

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = { models[0].m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, models[0].m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
#endif

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


		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
		                        descriptorSets[currentFrame].data(), 0, nullptr);

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
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			                   0, sizeof(PushConstants), &pushConstants);

			vkCmdDrawIndexed(commandBuffer, meshInfo.indexCount, 1, meshInfo.startIndex, static_cast<int32_t>(meshInfo.startVertex), 0);
#endif
		}

		vkCmdEndRendering(commandBuffer);
		// transition color image to present mode. No need for depth image, it is used directly for depth purposes,
		// and we don't need to store or use it elsewhere (at least currently)
		TransitionImage(commandBuffer, renderer->m_swapChainImages[imageIndex],
		                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
		{
			Log::Error("[DRAW] Recording Command buffer Failure");
		}
		// else Log::Info("[DRAW] Recording Command buffer Success");
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

}

namespace 
{
	static void HandleCameraMovement(const std::shared_ptr<Cravillac::Camera>& camera, GLFWwindow* window, float deltaTime, float sensitivity)
	{
		float moveSpeed = sensitivity * 5;


		SM::Vector3 forward = camera->GetLookAtTarget() - camera->GetPosition();
		forward.y = 0.0f;
		forward.Normalize();

		SM::Vector3 worldUp(0.0f, 1.0f, 0.0f);
		SM::Vector3 right = worldUp.Cross(forward);
		right.Normalize();

		// Calculate movement in local space
		SM::Vector3 movement(0.0f, 0.0f, 0.0f);
		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement += forward * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement -= forward * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement -= right * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement += right * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) movement -= worldUp * moveSpeed * deltaTime;
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) movement += worldUp * moveSpeed * deltaTime;
		//Spar::Log::InfoDebug("movement: ", movement);
		camera->Translate(movement);
	}

	void HandleMouseMovement(const std::shared_ptr<Cravillac::Camera>& camera, float deltaX, float deltaY, float sensitivity) {
		deltaX *= -sensitivity;
		deltaY *= -sensitivity;

		float yaw = 0.0f;
		yaw += deltaX;
		float pitch = 0.0f;	
		pitch += deltaY;

		bool constrainPitch = true;

		if (constrainPitch) {
			if (pitch > 89.0f && pitch < -89.0f) // Fixed to 89.0f for consistency
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