# Skia Graphics Library CMake Configuration
cmake_minimum_required(VERSION 3.20)

project(Skia)

# 设置Skia库的路径（只使用Release版本）
set(SKIA_RELEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/release")

# 创建Skia接口库
add_library(Skia INTERFACE)

# 设置头文件包含目录
# Skia头文件使用相对路径如"include/core/SkArc.h"，所以我们需要将Skia根目录添加到包含路径
target_include_directories(Skia INTERFACE
    ${SKIA_RELEASE_DIR}
)

# 使用Release版本的Skia库
set(SKIA_LIB_DIR "${SKIA_RELEASE_DIR}/out/Release-windows-x64")

# 定义所有Skia相关的库文件
set(SKIA_LIBRARIES
    skia
    skottie
    skparagraph
    skresources
    sksg
    skshaper
    skunicode_core
    skunicode_icu
    svg
    skcms
    harfbuzz
    icu
    expat
    libjpeg
    libpng
    libwebp
    libwebp_sse41
    wuffs
    zlib
    bentleyottmann
    d3d12allocator
    spirv_cross
)

# 处理所有Skia库文件
foreach(lib ${SKIA_LIBRARIES})
    # 创建导入库目标
    add_library(${lib}_imported STATIC IMPORTED)

    # 使用Release版本的库文件
    set_target_properties(${lib}_imported PROPERTIES
        IMPORTED_LOCATION_DEBUG "${SKIA_RELEASE_DIR}/out/Release-windows-x64/${lib}.lib"
        IMPORTED_LOCATION_RELEASE "${SKIA_RELEASE_DIR}/out/Release-windows-x64/${lib}.lib"
        IMPORTED_LOCATION "${SKIA_RELEASE_DIR}/out/Release-windows-x64/${lib}.lib"
    )

    # 将导入的库链接到Skia接口库
    target_link_libraries(Skia INTERFACE ${lib}_imported)
endforeach()

# 添加系统库依赖（Windows平台）
if(WIN32)
    target_link_libraries(Skia INTERFACE
        user32
        gdi32
        winspool
        comdlg32
        advapi32
        shell32
        ole32
        oleaut32
        uuid
        odbc32
        odbccp32
        opengl32
        d3d12
        dxgi
        d3dcompiler
    )
endif()

# 设置编译定义
target_compile_definitions(Skia INTERFACE
    SK_GANESH=1
    SK_GRAPHITE=1
    SK_VULKAN=1
    SK_DIRECT3D=1
    SK_GL=1
    SK_UNICODE_ICU_IMPLEMENTATION=1
    SK_CODEC_DECODES_JPEG=1
    SK_CODEC_DECODES_PNG=1
    SK_CODEC_DECODES_WEBP=1
    SK_ENCODE_JPEG=1
    SK_ENCODE_PNG=1
    SK_ENCODE_WEBP=1
    SK_XML=1
    SK_ENABLE_SVG=1
    SK_ENABLE_SKOTTIE=1
    SK_ENABLE_PARAGRAPH=1
)

# 由于使用Release版本的库，统一使用Release定义
target_compile_definitions(Skia INTERFACE
    SK_RELEASE
    NDEBUG
)

# 设置C++标准
target_compile_features(Skia INTERFACE cxx_std_17)

# 输出配置信息
message(STATUS "Skia configuration:")
message(STATUS "  Release libraries: ${SKIA_RELEASE_DIR}/out/Release-windows-x64")
message(STATUS "  Include directories: ${SKIA_RELEASE_DIR}/include")
message(STATUS "  Currently using: ${SKIA_LIB_DIR}")
message(STATUS "  Note: Using only Release libraries for compatibility")
