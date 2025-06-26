cmake_minimum_required(VERSION 3.20)

project(TinaFlow VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 分别定义 Qt 和 vcpkg 的路径
set(Qt_PREFIX_PATH "D:/Programs/Qt/6.8.0/msvc2022_64")
set(VCPKG_PREFIX_PATH "D:/Programs/vcpkg/installed/x64-windows/share")


set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 明确指定 UI 文件搜索路径
set(CMAKE_AUTOUIC_SEARCH_PATHS "${PROJECT_SOURCE_DIR}/ui")

if (MSVC)
    add_compile_options(/MP /bigobj /EHsc /Zc:preprocessor /Zc:__cplusplus /permissive- /wd4576)
    add_definitions(-D_WIN32_WINNT=0x0601 -DNOMINMAX -D_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS)
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets Core OpenGL)

option(OPENXLSX_CREATE_DOCS "Build library documentation (requires Doxygen and Graphviz/Dot to be installed)" OFF)
option(OPENXLSX_BUILD_SAMPLES "Build sample programs" OFF)
option(OPENXLSX_BUILD_TESTS "Build unit tests" OFF)

# 添加OpenXLSX子目录
add_subdirectory(third_party/OpenXLSX)

add_subdirectory(third_party/nodeeditor)

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

target_link_libraries(TinaFlow PRIVATE
        Qt6::Widgets
        Qt6::Core
        Qt6::OpenGL
        OpenXLSX
        QtNodes
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

