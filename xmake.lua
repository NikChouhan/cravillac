set_xmakever("2.9.4")

includes("scripts/compile.lua") 
includes("scripts/packages.lua")
includes("src/xmake.lua")
includes("shaders/xmake.lua")

add_rules("mode.debug", "mode.release")
set_defaultmode("debug")
add_includedirs("src")
add_syslinks("user32.lib", "kernel32.lib", "shell32.lib")
add_defines("UNICODE", "_UNICODE")

add_includedirs("src","src/common", "src/core", "src/renderer", {public = true})

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

target("VulkanTest")
    set_default(true)
    set_kind("binary")
    add_files("src/*.cpp")
    add_headerfiles("src/*.h")
    add_deps("core", "renderer")
    local vulkan_sdk = os.getenv("VULKAN_SDK")
    if is_os("windows") then
        if vulkan_sdk then
            --add_includedirs(path.join(vulkan_sdk, "Include"), {public = true})
            add_linkdirs(path.join(vulkan_sdk, "Lib"))
            add_links("vulkan-1", {public = true})
        else
            print("Warning: VULKAN_SDK environment variable is not set.")
        end
    end
target_end()

