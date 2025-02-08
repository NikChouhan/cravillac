target("core")
    set_kind("static")
    set_group("VKEngine")
    add_files("core/*.cpp")
    add_headerfiles("core/*.h")
target_end()

target("renderer")
    set_kind("static")
    set_group("VKEngine")
    add_files("renderer/*.cpp")
    add_headerfiles("renderer/*.h")
    add_packages("glfw", "stb", {public = true})
    local vulkan_sdk = os.getenv("VULKAN_SDK")
    if vulkan_sdk then
        add_includedirs(path.join(vulkan_sdk, "Include"), {public = true})
        add_linkdirs(path.join(vulkan_sdk, "Lib"))
        add_links("vulkan-1")
    else
        print("Warning: VULKAN_SDK environment variable is not set.")
    end
target_end()