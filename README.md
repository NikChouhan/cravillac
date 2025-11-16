# Cravillac
A simple vulkan renderer built to showcase various rendering techniques. It requires having the Vulkan SDK installed and path variable set. For Windows its easy as installing the SDK from https://vulkan.lunarg.com/sdk/home. For linux it depends on the package manager of your distribution

(All the work will be done on the deferred branch from now, will be merged later)
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