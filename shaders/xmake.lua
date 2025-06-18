target("shaders")
    set_kind("object")
    
    -- Add shader files to the project
    add_files("**.vert", "**.frag", "**.comp", "**.mesh", "**.task")
    
    -- Make shaders visible in Visual Studio
    add_headerfiles("**.vert", "**.frag", "**.comp", "**.mesh", "**.task")
    
    -- Custom build rule for shader compilation
    add_rules("shader_compile")

rule("shader_compile")
    set_extensions(".vert", ".frag", ".comp", ".mesh", ".task")
    
    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        local outputdir = path.join(target:targetdir(), "shaders")
        local filename = path.filename(sourcefile)
        local extension = path.extension(sourcefile)
        local outputfile = path.join(outputdir, filename .. ".spv")
        
        -- Map shader extensions to glslangValidator stage types
        local stage_map = {
            [".vert"] = "vert",
            [".frag"] = "frag", 
            [".comp"] = "comp",
            [".mesh"] = "mesh",
            [".task"] = "task"
        }
        
        local stage = stage_map[extension] or "vert"
        
        -- Ensure output directory exists
        batchcmds:mkdir(outputdir)

        if is_host("windows") then
            local vulkan_sdk = os.getenv("VULKAN_SDK")
            if vulkan_sdk then
                -- Construct the full path to glslangValidator
                local glslang_path = path.join(vulkan_sdk, "Bin", "glslangValidator.exe")
                
                -- Execute glslangValidator command
                batchcmds:execv(glslang_path, {
                    "-S", stage,
                    "-gVS",
                    "-Od", 
                    "-V",
                    "-e", "main",
                    "--target-env", "spirv1.4",
                    "-o", outputfile,
                    sourcefile
                })
                
                -- Add dependency
                batchcmds:add_depfiles(sourcefile)
                batchcmds:set_depmtime(os.mtime(outputfile))
            else
                -- Handle case where VULKAN_SDK is not set
                print("Warning: VULKAN_SDK environment variable not found")
            end
        else
            -- Handle non-Windows platforms
            -- Try to find glslangValidator in PATH first
            local glslang_cmd = "glslangValidator"
            
            batchcmds:execv(glslang_cmd, {
                "-S", stage,
                "-gVS",
                "-Od",
                "-V", 
                "-e", "main",
                "--target-env", "spirv1.4",
                "-o", outputfile,
                sourcefile
            })
            
            batchcmds:add_depfiles(sourcefile)
            batchcmds:set_depmtime(os.mtime(outputfile))
        end
    end)