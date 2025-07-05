
# 通用的着色器编译函数
# 参数：
#   SHADER_DIR - 着色器源文件目录
#   OUTPUT_DIR - 编译输出目录
#   TARGET_NAME - 着色器编译目标名称
#   MAIN_TARGET - 主项目目标名称
function(compile_all_shaders SHADER_DIR OUTPUT_DIR TARGET_NAME MAIN_TARGET)
    message(STATUS "=== 着色器编译开始 ===")
    message(STATUS "源目录: ${SHADER_DIR}")
    message(STATUS "输出目录: ${OUTPUT_DIR}")
    
    # 确保输出目录存在
    file(MAKE_DIRECTORY ${OUTPUT_DIR})
    
    # 查找所有着色器文件，排除varying.def.sc
    file(GLOB ALL_SC_FILES "${SHADER_DIR}/*.sc")
    set(SHADER_FILES "")

    foreach(SC_FILE ${ALL_SC_FILES})
        get_filename_component(FILENAME ${SC_FILE} NAME)
        if(NOT FILENAME STREQUAL "varying.def.sc")
            list(APPEND SHADER_FILES ${SC_FILE})
        endif()
    endforeach()

    if(NOT SHADER_FILES)
        message(WARNING "在 ${SHADER_DIR} 中没有找到着色器文件")
        return()
    endif()

    message(STATUS "找到着色器文件: ${SHADER_FILES}")

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
            message(STATUS "编译顶点着色器: ${SHADER_FILE}")

        elseif(FILENAME MATCHES "^fs_")
            # 片段着色器
            bgfx_compile_shaders(
                TYPE FRAGMENT
                SHADERS ${SHADER_FILE}
                VARYING_DEF ${SHADER_DIR}/varying.def.sc
                OUTPUT_DIR ${OUTPUT_DIR}
                INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/third_party/bgfx.cmake/bgfx/src
                AS_HEADERS
            )
            message(STATUS "编译片段着色器: ${SHADER_FILE}")

        else()
            message(WARNING "未知着色器类型: ${SHADER_FILE} (文件名应以vs_或fs_开头)")
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

    # 按照你的工作代码，生成统一头文件
    set(UNIFIED_HEADER ${OUTPUT_DIR}/shaders.h)

    add_custom_command(
        OUTPUT ${UNIFIED_HEADER}
        COMMAND ${CMAKE_COMMAND}
            -DSHADER_SOURCE_DIR=${SHADER_DIR}
            -DSHADER_OUT_DIR=${OUTPUT_DIR}
            -DOUTPUT_FILE=${UNIFIED_HEADER}
            -P ${CMAKE_SOURCE_DIR}/cmake/generate_unified_shader_header.cmake
        DEPENDS ${SHADER_FILES} ${CMAKE_SOURCE_DIR}/cmake/generate_unified_shader_header.cmake
        COMMENT "Generating unified shader header"
    )

    # 按照你的工作代码，创建目标
    add_custom_target(${TARGET_NAME}
        DEPENDS ${UNIFIED_HEADER}
        COMMENT "Smart shader management"
    )

    # 按照你的工作代码，确保主目标依赖于着色器编译
    add_dependencies(${MAIN_TARGET} ${TARGET_NAME})

    # 设置父作用域变量，供调用者使用
    set(SHADER_HEADER_FILE ${UNIFIED_HEADER} PARENT_SCOPE)

    message(STATUS "=== 着色器编译配置完成 ===")
    message(STATUS "统一头文件: ${UNIFIED_HEADER}")
    message(STATUS "目标名称: ${TARGET_NAME}")
    message(STATUS "主目标: ${MAIN_TARGET}")
endfunction()

# 便捷的清理函数
function(add_shader_clean TARGET_NAME OUTPUT_DIR)
    add_custom_target(${TARGET_NAME}
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMENT "清理所有编译的着色器"
    )
endfunction()
