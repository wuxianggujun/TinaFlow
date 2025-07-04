# Skia 库配置 (您自己编译的版本)
cmake_minimum_required(VERSION 3.20)

# 创建 Skia 库目标 - 使用 INTERFACE 库来正确传递头文件
add_library(Skia INTERFACE)

# 设置头文件路径 - Skia 期望根目录作为包含路径
target_include_directories(Skia INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# 链接主要库文件
target_link_libraries(Skia INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/skia.lib
)

# 添加 Skia 的所有依赖库
target_link_libraries(Skia INTERFACE
    # Skia 相关库
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/skcms.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/skshaper.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/svg.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/skottie.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/sksg.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/skresources.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/pathkit.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/bentleyottmann.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/expat.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/jsonreader.lib

    # 系统依赖库
    opengl32
    user32
    gdi32
    kernel32
    ole32
    oleaut32
    uuid
    advapi32
    shell32
    comdlg32
    winspool
)

message(STATUS "Using custom compiled Skia library with all dependencies")
