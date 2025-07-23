set_xmakever("2.9.4")

-- includes("scripts/compile.lua")
includes("scripts/packages.lua")
includes("src/xmake.lua")
includes("shaders/xmake.lua")

add_rules("mode.debug", "mode.release")
set_defaultmode("debug")
add_includedirs("src")
if is_os("windows") then
    add_syslinks("user32.lib", "kernel32.lib", "shell32.lib", "comctl32.lib")
end
if is_os("linux") then
    add_syslinks("dl", "pthread", "X11", "Xxf86vm", "Xrandr", "Xi")
end 

add_defines("UNICODE", "_UNICODE")

add_includedirs("src", "src/core", "src/renderer", "src/includes", { public = true })

if is_host("windows") then
    local vulkan_sdk = os.getenv("VULKAN_SDK")
    if vulkan_sdk then
        add_includedirs(path.join(vulkan_sdk, "Include"), {public = true})
        add_linkdirs(path.join(vulkan_sdk, "Lib"), {public = true})
        add_links("vulkan-1", {public = true})
    end
end

set_languages("cxx23", "c17")
set_policy("check.auto_ignore_flags", false)

if (is_mode("debug")) then
    set_symbols("debug")
    add_defines("DEBUG")
    set_optimize("none")
    set_warnings("all", "extra")
    set_runtimes("MDd")
elseif (is_mode("release")) then
    set_symbols("release")
    add_defines("NDEBUG")
    set_optimize("fastest")
    set_symbols("hidden")
    set_strip("all")
    set_warnings("none")
    set_policy("build.optimization.lto", true)
    set_runtimes("MD")
end

target("game")
    set_default(true)
    set_kind("binary")
    add_files("src/*.cpp")
    add_headerfiles("src/*.h")
    add_deps("engine")
target_end()