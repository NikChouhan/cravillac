# Cravillac
A simple vulkan renderer built to showcase various rendering techniques. It requires having the Vulkan SDK installed and path variable set. For Windows its easy as installing the SDK from https://vulkan.lunarg.com/sdk/home. 
(Note: Works with the Vulkan SDK version : 1.4.313.2, shader module validation issues with the latest 1.4.321.1)
# Dependencies
```
glfw
meshoptimizer
stb
cgltf
glm
imgui
```
# Build and run
## Windows
```
xmake project -k vsxmake -y
xmake build shaders
xmake run
```
### Compiling shaders after shader changes
```
xmake build shaders
```
## Linux (issues with the a prev few commits)
### Vscode
```
xmake project -k compile_commands
xmake build
xmake run
```
### CLion
```
xmake project -k cmake
```
All the projects and files are created and can be opened with CLion.