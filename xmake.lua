set_xmakever("2.9.4")

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

includes("scripts/compile.lua") 


add_rules("mode.debug", "mode.release")
set_defaultmode("debug")
add_includedirs("src")
add_syslinks("user32.lib", "kernel32.lib", "shell32.lib")
add_defines("UNICODE", "_UNICODE")

set_languages("cxx23", "c17")

if(is_mode("debug")) then
    set_symbols("debug")
    add_defines("DEBUG")
    set_optimize("none")
    set_warnings("all","extra")

elseif(is_mode("release")) then
    set_symbols("release")
    add_defines("NDEBUG")
    set_optimize("fastest")
    set_warnings("none")
    --set_policy("build.optimization.lto", true)
end

--target("shaders")
--   set_kind("phony")
--    add_rules("compilation.shaders")
--    add_files("shaders/*.frag", "shaders/*.vert")
--target_end()

target("VulkanTest")
    set_default(true)
    set_kind("binary")
    add_files("src/*.cpp")
    add_headerfiles("src/*.h")
    add_packages("glfw", "stb", {public = true})
    local vulkan_sdk = os.getenv("VULKAN_SDK")
    if vulkan_sdk then
        add_includedirs(path.join(vulkan_sdk, "Include"))
        add_linkdirs(path.join(vulkan_sdk, "Lib"))
        add_links("vulkan-1")
    else
        print("Warning: VULKAN_SDK environment variable is not set.")
    end
target_end()


--
-- If you want to known more usage about xmake, please see https://xmake.io
--
-- ## FAQ
--
-- You can enter the project directory firstly before building project.
--
--   $ cd projectdir
--
-- 1. How to build project?
--
--   $ xmake
--
-- 2. How to configure project?
--
--   $ xmake f -p [macosx|linux|iphoneos ..] -a [x86_64|i386|arm64 ..] -m [debug|release]
--
-- 3. Where is the build output directory?
--
--   The default output directory is `./build` and you can configure the output directory.
--
--   $ xmake f -o outputdir
--   $ xmake
--
-- 4. How to run and debug target after building project?
--
--   $ xmake run [targetname]
--   $ xmake run -d [targetname]
--
-- 5. How to install target to the system directory or other output directory?
--
--   $ xmake install
--   $ xmake install -o installdir
--
-- 6. Add some frequently-used compilation flags in xmake.lua
--
-- @code
--    -- add debug and release modes
--    add_rules("mode.debug", "mode.release")
--
--    -- add macro definition
--    add_defines("NDEBUG", "_GNU_SOURCE=1")
--
--    -- set warning all as error
--    set_warnings("all", "error")
--
--    -- set language: c99, c++11
--    set_languages("c99", "c++11")
--
--    -- set optimization: none, faster, fastest, smallest
--    set_optimize("fastest")
--
--    -- add include search directories
--    add_includedirs("/usr/include", "/usr/local/include")
--
--    -- add link libraries and search directories
--    add_links("tbox")
--    add_linkdirs("/usr/local/lib", "/usr/lib")
--
--    -- add system link libraries
--    add_syslinks("z", "pthread")
--
--    -- add compilation and link flags
--    add_cxflags("-stdnolib", "-fno-strict-aliasing")
--    add_ldflags("-L/usr/local/lib", "-lpthread", {force = true})
--
-- @endcode
--