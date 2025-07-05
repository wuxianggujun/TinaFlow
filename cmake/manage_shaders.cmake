# CMake script to manage shader compilation and header generation
# Usage: cmake -DSHADER_SOURCE_DIR=<dir> -DSHADER_OUT_DIR=<dir> -DACTION=<action> -P manage_shaders.cmake
# Actions: CLEAN, COMPILE, GENERATE_HEADER, ALL

if(NOT SHADER_SOURCE_DIR)
    message(FATAL_ERROR "SHADER_SOURCE_DIR must be specified")
endif()

if(NOT SHADER_OUT_DIR)
    message(FATAL_ERROR "SHADER_OUT_DIR must be specified")
endif()

if(NOT ACTION)
    set(ACTION "ALL")
endif()

# Convert to absolute paths
get_filename_component(SHADER_SOURCE_DIR "${SHADER_SOURCE_DIR}" ABSOLUTE)
get_filename_component(SHADER_OUT_DIR "${SHADER_OUT_DIR}" ABSOLUTE)

message(STATUS "Shader management action: ${ACTION}")
message(STATUS "Source directory: ${SHADER_SOURCE_DIR}")
message(STATUS "Output directory: ${SHADER_OUT_DIR}")

# Function to check if shaders need recompilation
function(check_shader_freshness NEED_RECOMPILE)
    set(${NEED_RECOMPILE} TRUE PARENT_SCOPE)
    
    # Find all shader source files
    file(GLOB VS_SOURCES "${SHADER_SOURCE_DIR}/vs_*.sc")
    file(GLOB FS_SOURCES "${SHADER_SOURCE_DIR}/fs_*.sc")
    set(SHADER_SOURCES ${VS_SOURCES} ${FS_SOURCES})
    
    if(NOT SHADER_SOURCES)
        message(STATUS "No shader sources found")
        set(${NEED_RECOMPILE} FALSE PARENT_SCOPE)
        return()
    endif()
    
    # Check if output directory exists
    if(NOT EXISTS "${SHADER_OUT_DIR}")
        message(STATUS "Output directory doesn't exist - need to compile")
        return()
    endif()
    
    # Check if unified header exists
    if(NOT EXISTS "${SHADER_OUT_DIR}/shaders.h")
        message(STATUS "Unified header doesn't exist - need to compile")
        return()
    endif()
    
    # Get the timestamp of the newest source file
    set(NEWEST_SOURCE_TIME 0)
    foreach(SOURCE_FILE ${SHADER_SOURCES})
        file(TIMESTAMP "${SOURCE_FILE}" SOURCE_TIME "%s")
        if(SOURCE_TIME GREATER NEWEST_SOURCE_TIME)
            set(NEWEST_SOURCE_TIME ${SOURCE_TIME})
        endif()
    endforeach()
    
    # Get the timestamp of the unified header
    file(TIMESTAMP "${SHADER_OUT_DIR}/shaders.h" HEADER_TIME "%s")
    
    # Compare timestamps
    if(NEWEST_SOURCE_TIME GREATER HEADER_TIME)
        message(STATUS "Source files are newer than compiled headers - need to recompile")
        return()
    endif()
    
    message(STATUS "Shaders are up to date")
    set(${NEED_RECOMPILE} FALSE PARENT_SCOPE)
endfunction()

# Clean action - 只清理统一头文件，不清理编译后的着色器
if(ACTION STREQUAL "CLEAN")
    message(STATUS "Cleaning unified shader header...")
    if(EXISTS "${SHADER_OUT_DIR}/shaders.h")
        file(REMOVE "${SHADER_OUT_DIR}/shaders.h")
    endif()
endif()

# 对于ALL action，不自动清理，让bgfx处理着色器编译

# Check if compilation is needed (only if not cleaning)
if(NOT ACTION STREQUAL "CLEAN")
    if(ACTION STREQUAL "ALL")
        # For ALL action, always check freshness unless we just cleaned
        check_shader_freshness(NEED_RECOMPILE)
        if(NOT NEED_RECOMPILE)
            message(STATUS "Shaders are up to date, skipping compilation")
            return()
        endif()
    endif()
endif()

# Generate header action
if(ACTION STREQUAL "GENERATE_HEADER" OR ACTION STREQUAL "ALL")
    message(STATUS "Generating unified shader header...")
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -DSHADER_SOURCE_DIR=${SHADER_SOURCE_DIR}
            -DSHADER_OUT_DIR=${SHADER_OUT_DIR}
            -DOUTPUT_FILE=${SHADER_OUT_DIR}/shaders.h
            -P ${CMAKE_CURRENT_LIST_DIR}/generate_unified_shader_header.cmake
        RESULT_VARIABLE RESULT
    )
    
    if(NOT RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to generate unified shader header")
    endif()
    
    message(STATUS "Unified shader header generated successfully")
endif()

message(STATUS "Shader management completed")
