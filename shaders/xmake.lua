target("shaders")
    set_kind("object")
    
    -- Add shader files to the project
    add_files("**.slang")
    
    -- Make shaders visible in Visual Studio
    add_headerfiles("**.slang")
    
    -- Custom build rule for shader compilation
    add_rules("shader_compile")

rule("shader_compile")
    set_extensions(".slang")
    
    on_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        local outputdir = path.join(target:targetdir(), "shaders")
        local filename = path.filename(sourcefile)
        local basename = path.basename(sourcefile)  -- Get filename without extension
        local outputfile = path.join(outputdir, basename .. ".spv")
        
        -- Extract stage from filename (e.g., "mesh.frag.slang" -> "frag")  
        local stage_extension = nil
        
        -- Match the pattern: anything.STAGE.slang where STAGE is what we want
        local stage_part = sourcefile:match("%.([^%.]+)%.slang$")
        if stage_part then
            stage_extension = "." .. stage_part
        end
        
        -- Map shader extensions to Slang stage types
        local stage_map = {
            [".vert"] = "vertex",
            [".frag"] = "pixel",    -- Slang uses "pixel" for fragment shaders
            [".comp"] = "compute",
            [".mesh"] = "mesh",
            [".task"] = "amplification"  -- Slang uses "amplification" for task shaders
        }
        
        local stage = stage_map[stage_extension] or "vertex"
        
        -- Ensure output directory exists
        batchcmds:mkdir(outputdir)

        if is_host("windows") then
            -- Try to find slangc.exe in common locations
            local slang_paths = {
                "slangc.exe",  -- Try PATH first
                path.join(os.getenv("SLANG_HOME") or "", "bin", "slangc.exe"),
                path.join(os.getenv("SLANG_SDK") or "", "bin", "slangc.exe"),
                "C:\\Program Files\\Slang\\bin\\slangc.exe",
                "C:\\tools\\slang\\bin\\slangc.exe"
            }
            
            local slang_cmd = nil
            for _, slang_path in ipairs(slang_paths) do
                if os.isfile(slang_path) or (slang_path == "slangc.exe" and os.execv("where", {"slangc.exe"}, {try = true})) then
                    slang_cmd = slang_path
                    break
                end
            end
            
            if slang_cmd then
                -- Execute slangc command
                batchcmds:execv(slang_cmd, {
                    "-target", "spirv",
                    "-stage", stage,
                    "-entry", "main",
                    "-profile", "spirv_1_4",
                    "-force-glsl-scalar-layout",
                    "-O0",  -- No optimization for debug builds
                    "-g",   -- Generate debug info
                    "-o", outputfile,
                    sourcefile
                })
                
                -- Add dependency
                batchcmds:add_depfiles(sourcefile)
                batchcmds:set_depmtime(os.mtime(outputfile))
            else
                -- Handle case where slangc is not found
                print("Warning: slangc.exe not found. Please install Slang compiler or set SLANG_HOME/SLANG_SDK environment variable")
            end
        else
            -- Handle non-Windows platforms (Linux/macOS)
            local slang_paths = {
                "slangc",  -- Try PATH first
                path.join(os.getenv("SLANG_HOME") or "", "bin", "slangc"),
                path.join(os.getenv("SLANG_SDK") or "", "bin", "slangc"),
                "/usr/local/bin/slangc",
                "/opt/slang/bin/slangc"
            }
            
            local slang_cmd = nil
            for _, slang_path in ipairs(slang_paths) do
                if os.isfile(slang_path) or (slang_path == "slangc" and os.execv("which", {"slangc"}, {try = true})) then
                    slang_cmd = slang_path
                    break
                end
            end
            
            if slang_cmd then
                batchcmds:execv(slang_cmd, {
                    "-target", "spirv",
                    "-stage", stage,
                    "-entry", "main", 
                    "-profile", "spirv_1_4",
                    "-O0",  -- No optimization for debug builds
                    "-g",   -- Generate debug info
                    "-o", outputfile,
                    sourcefile
                })
                
                batchcmds:add_depfiles(sourcefile)
                batchcmds:set_depmtime(os.mtime(outputfile))
            else
                print("Warning: slangc not found in PATH or common locations. Please install Slang compiler")
            end
        end
    end)