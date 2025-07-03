cmake_minimum_required(VERSION 3.20)

project(TinaFlow VERSION 0.1 LANGUAGES CXX)

# 运行时库配置 - 为了与Skia兼容
if (MSVC)
    # 强制使用静态Release运行时库以匹配Skia
    # 注意：即使在Debug模式下也使用Release运行时库
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

    # 为了与Skia Release库兼容，在Debug模式下使用Release级别的迭代器调试
    add_compile_definitions($<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>)

    # 添加编译器特定的优化选项
    add_compile_options(
        /MP          # 多处理器编译
        /bigobj      # 大对象文件支持
        /EHsc        # 异常处理模型
        /Zc:preprocessor  # 符合标准的预处理器
        /Zc:__cplusplus   # 正确的__cplusplus宏
        /permissive-      # 禁用非标准扩展
        /wd4576      # 禁用特定警告
        /MT          # 强制使用静态Release运行时库（所有配置）
        $<$<CONFIG:Release>:/O2>     # Release模式优化
        $<$<CONFIG:Release>:/GL>     # 全程序优化
    )

    # 链接器优化
    add_link_options(
        $<$<CONFIG:Release>:/LTCG>   # 链接时代码生成
        $<$<CONFIG:Release>:/OPT:REF> # 移除未引用的函数
        $<$<CONFIG:Release>:/OPT:ICF> # 相同函数合并
    )

    # Windows特定定义
    add_definitions(
        -D_WIN32_WINNT=0x0601  # Windows 7+
        -DNOMINMAX             # 禁用min/max宏
        -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
    )
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

# 分别定义 Qt 和 vcpkg 的路径
set(Qt_PREFIX_PATH "D:/Programs/Qt/6.9.1/msvc2022_64")

# 将Qt路径添加到CMAKE_PREFIX_PATH中
list(APPEND CMAKE_PREFIX_PATH ${Qt_PREFIX_PATH})

option(OPENXLSX_CREATE_DOCS "Build library documentation (requires Doxygen and Graphviz/Dot to be installed)" OFF)
option(OPENXLSX_BUILD_SAMPLES "Build sample programs" OFF)
option(OPENXLSX_BUILD_TESTS "Build unit tests" OFF)

# 统一输出目录配置 - 确保exe和dll都在同一目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 为不同配置分别设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# 强制子项目使用相同的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_FOR_BUILD ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_FOR_BUILD ${CMAKE_BINARY_DIR}/bin)

# 添加OpenXLSX子目录
add_subdirectory(third_party/OpenXLSX)

add_subdirectory(third_party/nodeeditor)

add_subdirectory(third_party/Qt-Advanced-Docking-System)

# 添加Skia库
add_subdirectory(third_party/skia)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 明确指定 UI 文件搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR}/ui")

# MSVC设置已在上面统一配置

find_package(Qt6 REQUIRED COMPONENTS Widgets Core OpenGL OpenGLWidgets)



file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/*.cpp
        ${PROJECT_SOURCE_DIR}/include/*.hpp
        ${PROJECT_SOURCE_DIR}/ui/*.ui
        ${PROJECT_SOURCE_DIR}/resources/*.qrc  # 确保 .qrc 文件被包含
)


qt_add_executable(TinaFlow MANUAL_FINALIZATION
        ${SOURCES}
)

target_include_directories(TinaFlow PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

if(WIN32 AND QT_VERSION_MAJOR LESS 6)
    target_link_libraries(TinaFlow
            PRIVATE Qt${QT_VERSION_MAJOR}::AxContainer
    )
endif()

target_link_libraries(TinaFlow PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::OpenGL
        Qt6::OpenGLWidgets
        OpenXLSX
        QtNodes
        qtadvanceddocking-qt6
        Skia
)

set_target_properties(TinaFlow PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)
include(GNUInstallDirs)
install(TARGETS TinaFlow
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_finalize_executable(TinaFlow)

# 添加自定义命令，确保所有依赖的dll都在正确的目录
if(WIN32)
    # 获取Qt安装路径并部署Qt库
    get_target_property(QT_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(QT_WINDEPLOYQT_EXECUTABLE ${QT_QMAKE_EXECUTABLE} PATH)
    set(QT_WINDEPLOYQT_EXECUTABLE "${QT_WINDEPLOYQT_EXECUTABLE}/windeployqt.exe")

    # 使用生成器表达式来根据配置选择不同的windeployqt参数
    add_custom_command(TARGET TinaFlow POST_BUILD
        COMMAND ${QT_WINDEPLOYQT_EXECUTABLE}
            $<$<CONFIG:Debug>:--debug>
            $<$<CONFIG:Release>:--release>
            --compiler-runtime --force --verbose 2
            $<TARGET_FILE:TinaFlow>
        COMMENT "Deploying Qt libraries with windeployqt")

    # 复制ADS库的dll到输出目录
    add_custom_command(TARGET TinaFlow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:qtadvanceddocking-qt6>
        $<TARGET_FILE_DIR:TinaFlow>
        COMMENT "Copying ADS dll to output directory"
    )

    # 复制Skia的ICU数据文件到输出目录（统一使用Release版本）
    add_custom_command(TARGET TinaFlow POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/third_party/skia/release/out/Release-windows-x64/icudtl.dat
        $<TARGET_FILE_DIR:TinaFlow>
        COMMENT "Copying Skia ICU data file to output directory"
    )
endif()

