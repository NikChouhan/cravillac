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

        -- Execute glslangValidator command
        batchcmds:execv("C:/VulkanSDK/1.4.304.0/Bin/glslangValidator.exe", {
            "-S", stage,
            "-gVS",
            "-Od", 
            "-V",
            "-e", "main",
            "--target-env", "spirv1.4",
            "-o", outputfile,
            sourcefile
        })
        -- Execute command
        batchcmds:execv(cmd)
        
        -- Add dependency
        batchcmds:add_depfiles(sourcefile)
        batchcmds:set_depmtime(os.mtime(outputfile))
    end)