#include <pch.h>
#define GLM_ENABLE_EXPERIMENTAL

#include "common.h"
#include "Camera.h"
#include "GfxDevice.h"
#include "ImguiRenderer.h"
#include "Model.h"
#include "Vertex.h"

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
	//Model mod1;
	//mod1.LoadModel(renderer,"../../../../assets/models/suzanne/Suzanne.gltf");
	//mod1.LoadModel(renderer,"../../../../assets/models/flighthelmet/FlightHelmet.gltf");
	//mod1.LoadModel(gfxDevice, "../../../../assets/models/sponza2/sponza2.gltf");
	//mod1.LoadModel(renderer, "../../../../assets/models/bistro2/bistro2.gltf");
	//mod1.LoadModel(renderer,"../../../../assets/models/Cube/cube.gltf");

//#if MESH_SHADING
//	// bda + pvp for meshlet buffer address
//	vk::DeviceAddress meshBDA;
//	vk::BufferDeviceAddressInfo meshletBufferAddressInfo{};
//	meshletBufferAddressInfo.buffer = mod1._meshletBuffer;
//	meshBDA = renderer->_device.getBufferAddress(&meshletBufferAddressInfo);
//#else
//	// bda + pvp get vertex buffer address
//	vk::BufferDeviceAddressInfo vertexBufferAddressInfo{};
//	vertexBufferAddressInfo.buffer = mod1._vertexBuffer;
//	vk::DeviceAddress vertexBDA = gfxDevice._device.getBufferAddress(&vertexBufferAddressInfo);
//#endif

	auto cameraUpdate = [&](const MeshInfo& meshInfo) -> CameraPlex
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

		char newTitle[256];
		snprintf(newTitle, sizeof(newTitle), "CV --- CPU time: %.2fms", frameDelta * 1000);
		glfwSetWindowTitle(_window, newTitle);
	}
}