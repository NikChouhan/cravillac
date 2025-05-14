set_xmakever("2.9.4")

-- includes("scripts/compile.lua")
includes("scripts/packages.lua")
includes("src/xmake.lua")
-- includes("shaders/xmake.lua")

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

set_languages("cxx20", "c17")

if (is_mode("debug")) then
    set_symbols("debug")
    add_defines("DEBUG")
    set_optimize("none")
    set_warnings("all", "extra")
elseif (is_mode("release")) then
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
target_end()