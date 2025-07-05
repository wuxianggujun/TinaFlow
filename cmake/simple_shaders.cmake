
# Universal shader compilation function
# Parameters:
#   SHADER_DIR - Shader source directory
#   OUTPUT_DIR - Compilation output directory
#   MAIN_TARGET - Main project target name
function(compile_all_shaders SHADER_DIR OUTPUT_DIR MAIN_TARGET)
    message(STATUS "=== Shader Compilation Started ===")
    message(STATUS "Source directory: ${SHADER_DIR}")
    message(STATUS "Output directory: ${OUTPUT_DIR}")
    message(STATUS "Main target: ${MAIN_TARGET}")

    # Generate shader target name based on main target
    set(SHADER_TARGET_NAME "${MAIN_TARGET}_shaders")

    # Ensure output directory exists
    file(MAKE_DIRECTORY ${OUTPUT_DIR})

    # Find all shader files, excluding varying.def.sc
    file(GLOB ALL_SC_FILES "${SHADER_DIR}/*.sc")
    set(SHADER_FILES "")

    foreach(SC_FILE ${ALL_SC_FILES})
        get_filename_component(FILENAME ${SC_FILE} NAME)
        if(NOT FILENAME STREQUAL "varying.def.sc")
            list(APPEND SHADER_FILES ${SC_FILE})
        endif()
    endforeach()

    if(NOT SHADER_FILES)
        message(WARNING "No shader files found in ${SHADER_DIR}")
        return()
    endif()

    list(LENGTH SHADER_FILES SHADER_COUNT)
    message(STATUS "Found ${SHADER_COUNT} shader files")

    # 编译每个着色器文件 - 根据文件名判断类型
    foreach(SHADER_FILE ${SHADER_FILES})
        get_filename_component(FILENAME ${SHADER_FILE} NAME)

        if(FILENAME MATCHES "^vs_")
            # 顶点着色器
            bgfx_compile_shaders(
                TYPE VERTEX
                SHADERS ${SHADER_FILE}
                VARYING_DEF ${SHADER_DIR}/varying.def.sc
                OUTPUT_DIR ${OUTPUT_DIR}
                INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/bgfx.cmake/bgfx/src
                AS_HEADERS
            )
            message(STATUS "Configuring vertex shader: ${SHADER_FILE}")

        elseif(FILENAME MATCHES "^fs_")
            # Fragment shader
            bgfx_compile_shaders(
                TYPE FRAGMENT
                SHADERS ${SHADER_FILE}
                VARYING_DEF ${SHADER_DIR}/varying.def.sc
                OUTPUT_DIR ${OUTPUT_DIR}
                INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/bgfx.cmake/bgfx/src
                AS_HEADERS
            )
            message(STATUS "Configuring fragment shader: ${SHADER_FILE}")

        else()
            message(WARNING "Unknown shader type: ${SHADER_FILE} (filename should start with vs_ or fs_)")
        endif()
    endforeach()
    
    # 生成预期的编译后头文件列表
    set(COMPILED_HEADERS "")
    set(PLATFORMS dx10 dx11 glsl essl spirv)

    foreach(SHADER_FILE ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
        foreach(PLATFORM ${PLATFORMS})
            set(COMPILED_HEADER "${OUTPUT_DIR}/${PLATFORM}/${SHADER_NAME}.sc.bin.h")
            list(APPEND COMPILED_HEADERS ${COMPILED_HEADER})
        endforeach()
    endforeach()

    # 按照你的工作代码，添加着色器源文件到目标
    target_sources(${MAIN_TARGET} PRIVATE ${SHADER_FILES})

    # Generate unified header file directly here (no external script needed)
    set(UNIFIED_HEADER ${OUTPUT_DIR}/shaders.h)

    # Write unified header file content
    file(WRITE ${UNIFIED_HEADER} "// Auto-generated unified shader header\n")
    file(APPEND ${UNIFIED_HEADER} "#pragma once\n\n")
    file(APPEND ${UNIFIED_HEADER} "#include <cstdint>\n")
    file(APPEND ${UNIFIED_HEADER} "#include <unordered_map>\n")
    file(APPEND ${UNIFIED_HEADER} "#include <string>\n\n")

    # Define platforms
    set(PLATFORMS dx10 dx11 glsl essl spirv)
    set(PLATFORM_NAMES "DirectX 10" "DirectX 11" "OpenGL" "OpenGL ES" "Vulkan/SPIR-V")

    list(LENGTH PLATFORMS PLATFORM_COUNT)
    math(EXPR PLATFORM_COUNT "${PLATFORM_COUNT} - 1")

    # Include platform shader headers
    foreach(INDEX RANGE ${PLATFORM_COUNT})
        list(GET PLATFORMS ${INDEX} PLATFORM)
        list(GET PLATFORM_NAMES ${INDEX} PLATFORM_NAME)

        file(APPEND ${UNIFIED_HEADER} "// ${PLATFORM_NAME} shaders\n")

        foreach(SHADER_FILE ${SHADER_FILES})
            get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
            file(APPEND ${UNIFIED_HEADER} "#include \"${PLATFORM}/${SHADER_NAME}.sc.bin.h\"\n")
        endforeach()

        file(APPEND ${UNIFIED_HEADER} "\n")
    endforeach()

    # Generate shader data structure and lookup function
    file(APPEND ${UNIFIED_HEADER} "struct ShaderData {\n")
    file(APPEND ${UNIFIED_HEADER} "    const uint8_t* data;\n")
    file(APPEND ${UNIFIED_HEADER} "    uint32_t size;\n")
    file(APPEND ${UNIFIED_HEADER} "};\n\n")

    file(APPEND ${UNIFIED_HEADER} "inline ShaderData getShaderData(const std::string& name, const std::string& platform) {\n")
    file(APPEND ${UNIFIED_HEADER} "    static const std::unordered_map<std::string, ShaderData> shaders = {\n")

    # Generate shader mapping
    foreach(SHADER_FILE ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)

        foreach(PLATFORM ${PLATFORMS})
            if(PLATFORM STREQUAL "spirv")
                set(SUFFIX "spv")
            else()
                set(SUFFIX ${PLATFORM})
            endif()

            set(ARRAY_NAME "${SHADER_NAME}_${SUFFIX}")
            set(KEY "${SHADER_NAME}:${PLATFORM}")

            file(APPEND ${UNIFIED_HEADER} "        {\"${KEY}\", {${ARRAY_NAME}, sizeof(${ARRAY_NAME})}},\n")
        endforeach()
    endforeach()

    file(APPEND ${UNIFIED_HEADER} "    };\n")
    file(APPEND ${UNIFIED_HEADER} "    \n")
    file(APPEND ${UNIFIED_HEADER} "    auto it = shaders.find(name + \":\" + platform);\n")
    file(APPEND ${UNIFIED_HEADER} "    return (it != shaders.end()) ? it->second : ShaderData{nullptr, 0};\n")
    file(APPEND ${UNIFIED_HEADER} "}\n")

    # Create target that depends on shader source files (triggers compilation)
    add_custom_target(${SHADER_TARGET_NAME}
        DEPENDS ${SHADER_FILES}
        COMMENT "Shader compilation complete"
    )

    # Ensure main target depends on shader compilation
    add_dependencies(${MAIN_TARGET} ${SHADER_TARGET_NAME})

    # Set parent scope variables for caller use
    set(SHADER_HEADER_FILE ${UNIFIED_HEADER} PARENT_SCOPE)
    set(SHADER_INCLUDE_DIR ${OUTPUT_DIR} PARENT_SCOPE)

    message(STATUS "=== Shader Compilation Configured ===")
    message(STATUS "Unified header: ${UNIFIED_HEADER}")
    message(STATUS "Include directory: ${OUTPUT_DIR}")
    message(STATUS "Shader target: ${SHADER_TARGET_NAME}")
    message(STATUS "Main target: ${MAIN_TARGET}")
endfunction()

# Convenient cleanup function
function(add_shader_clean_target TARGET_NAME OUTPUT_DIR)
    add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMENT "Clean all compiled shaders"
    )
endfunction()
