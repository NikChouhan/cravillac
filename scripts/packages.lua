add_requires("stb")

add_requires("glm",
{
    debug =	is_mode("debug"),
    configs =
    {
        shared = false,
        runtimes = "MTd",
    }
})

add_requires("vulkan-headers",
{
    debug =	is_mode("debug"),
    configs =
    {
        shared = false,
        runtimes = "MTd",
    }
})

add_requires("directxmath",
{
    debug = is_mode("debug"),
    configs =
    {
        shared = false,
        runtimes = "MTd",
    }
})

add_requires("glfw")

add_requires("fastgltf")