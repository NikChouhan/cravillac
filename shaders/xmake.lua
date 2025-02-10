-- Shader configuration target
target("shaders")
    set_group("shaders")
    set_kind("phony")  -- Changed to binary for output tracking
    
    -- Organized shader sources
    add_files("**.vert", {rule = "shader_vert"})
    add_files("**.frag", {rule = "shader_frag"})
    add_files("**.comp", {rule = "shader_comp"})
    
    -- IDE visibility
    add_headerfiles("**.vert")
    add_headerfiles("**.frag")
    add_headerfiles("**.comp")
    
    -- Dedicated output directory
    set_targetdir("$(buildir)/shaders")

-- Enhanced compilation function
function compile_spirv(target, sourcefile, opt)
    local outputdir = target:targetdir()
    os.mkdir(outputdir)

    local outputfile = path.join(outputdir, path.filename(sourcefile) .. ".spv")
    target:add("outputs", outputfile)  -- Critical for tracking
    
    local glslang_cmd = string.format(
        "C:/VulkanSDK/1.4.304.0/Bin/glslangValidator -V %s -o %s",
        sourcefile,
        outputfile
    )
    
    -- Execute with error handling
    local ok, errors = os.run(glslang_cmd)
    if not ok then
        raise("Shader compilation failed for %s:\n%s", sourcefile, errors)
    end
    
    os.cp(sourcefile, outputdir)
end

-- Proper rule definition
rule("shader_vert")
    set_extensions(".vert")
    on_build_file(function (target, sourcefile, opt)
        compile_spirv(target, sourcefile, opt)
        target:add("deps", sourcefile, 
            {path.join(target:targetdir(), path.filename(sourcefile) .. ".spv")})
    end)