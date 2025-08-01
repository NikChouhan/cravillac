add_requires("stb")

add_requires("directxmath",
{
    configs =
    {
        shared = false,
        runtimes = "MTd",
    }
})

add_requires("glfw",
{
})

add_requires("cgltf")

add_requires("imgui docking",
{
   configs =
   {
       glfw = true,
       vulkan = true
   }
})

add_requires("glm")
--add_requires("vulkan-memory-allocator")

add_requires("meshoptimizer")