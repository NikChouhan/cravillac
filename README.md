# Cravillac
A simple cross platform renderer built to showcase various rendering techniques.
# Dependancis
```
vulkan-headers
glm
stb
glfw
```
# Build and run
## Windows
```
xmake project -k vsxmake
xmake run 
```
## Linux
### Vscode
```
xmake project -k compile_commands.json
```
Create `c_cpp_properties.json` in `.vscode` folder and add a configuration something of this sort:
```
{
    "configurations": [
        {
            "compileCommands": "compile_commands.json"
        }
    ],
    "version": 4
}
```
```
xmake build
xmake run
```
### CLion
```
xmake project -k cmake
```
All the projects and files are created and can be opened with CLion.


