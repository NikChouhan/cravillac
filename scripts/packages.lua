add_requires("stb",
{
    debug =	is_mode("debug"),
    configs =
    {
        shared = false,
        runtimes = "MTd",
    }
})

add_requires("glfw",
{
    debug =	is_mode("debug"),
    configs =
    {
        shared = false,
        runtimes = "MTd",
    }
})