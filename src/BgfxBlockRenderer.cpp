#include "BgfxBlockRenderer.hpp"
#include <QDebug>
#include <QTimer>

// 包含bgfx工具函数用于加载着色器
#include <QFile>
#include <bgfx/bgfx.h>
#include <bx/file.h>

// 包含生成的统一着色器头文件
#include "shaders.h"

// 从内嵌数据加载着色器的函数
static bgfx::ShaderHandle loadShaderFromMemory(const uint8_t* data, uint32_t size)
{
    if (!data || size == 0) {
        return BGFX_INVALID_HANDLE;
    }
    const bgfx::Memory* mem = bgfx::copy(data, size);
    return bgfx::createShader(mem);
}

// 根据当前渲染器类型获取平台名称
static std::string getRendererPlatformName(bgfx::RendererType::Enum rendererType)
{
    switch (rendererType) {
        case bgfx::RendererType::Direct3D11:
        case bgfx::RendererType::Direct3D12:
            return "dx11"; // 使用DX11着色器作为DirectX的通用选择
        case bgfx::RendererType::OpenGL:
            return "glsl";
        case bgfx::RendererType::OpenGLES:
            return "essl";
        case bgfx::RendererType::Vulkan:
            return "spirv";
        case bgfx::RendererType::Metal:
            return "metal";
        default:
            return "dx11"; // 默认后备选择
    }
}

// 通用着色器加载函数 - 支持任何着色器名称
static bgfx::ShaderHandle loadShaderForCurrentRenderer(const char* shaderName)
{
    bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
    std::string platformName = getRendererPlatformName(rendererType);

    // 使用动态查找获取着色器数据
    ShaderData shaderData = getShaderData(shaderName, platformName);

    if (shaderData.data == nullptr) {
        qWarning() << "Shader not found:" << shaderName << "for platform:" << platformName.c_str();

        // 尝试后备平台
        if (platformName != "dx11") {
            qDebug() << "Trying fallback platform: dx11";
            shaderData = getShaderData(shaderName, "dx11");
        }

        if (shaderData.data == nullptr) {
            qWarning() << "Shader not found even with fallback platform";
            return BGFX_INVALID_HANDLE;
        }
    }

    qDebug() << "Loading shader:" << shaderName << "for platform:" << platformName.c_str()
             << "size:" << shaderData.size << "bytes";

    return loadShaderFromMemory(shaderData.data, shaderData.size);
}

static bgfx::ProgramHandle loadProgram(const char* vsName, const char* fsName)
{
    bgfx::ShaderHandle vsh = loadShaderForCurrentRenderer(vsName);
    bgfx::ShaderHandle fsh = loadShaderForCurrentRenderer(fsName);

    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        qWarning() << "Failed to load shaders:" << vsName << fsName;
        return BGFX_INVALID_HANDLE;
    }

    qDebug() << "Successfully loaded shaders for renderer:" << bgfx::getRendererName(bgfx::getRendererType());
    return bgfx::createProgram(vsh, fsh, true);
}

// 顶点结构
struct PosColorVertex {
    float x, y, z;
    uint32_t abgr;

    static void init() {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
    }

    static bgfx::VertexLayout ms_layout;
};

bgfx::VertexLayout PosColorVertex::ms_layout;

// 创建矩形几何体
static void createRectangleGeometry(bgfx::VertexBufferHandle& vbh, bgfx::IndexBufferHandle& ibh, bgfx::VertexLayout& layout)
{
    // 初始化顶点布局
    PosColorVertex::init();
    layout = PosColorVertex::ms_layout;

    // 创建矩形顶点数据 (中心在原点，大小200x100)
    static PosColorVertex vertices[] = {
        {-100.0f, -50.0f, 0.0f, 0xff0000ff}, // 左下 - 红色
        { 100.0f, -50.0f, 0.0f, 0xff00ff00}, // 右下 - 绿色
        { 100.0f,  50.0f, 0.0f, 0xffff0000}, // 右上 - 蓝色
        {-100.0f,  50.0f, 0.0f, 0xffffff00}, // 左上 - 黄色
    };

    // 创建索引数据 (两个三角形组成矩形)
    static uint16_t indices[] = {
        0, 1, 2, // 第一个三角形
        2, 3, 0  // 第二个三角形
    };

    // 创建顶点缓冲区
    vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices, sizeof(vertices)),
        layout
    );

    // 创建索引缓冲区
    ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(indices, sizeof(indices))
    );
}

BgfxBlockRenderer::BgfxBlockRenderer(QWidget* parent)
    : BgfxWidget(parent)
{
    setMinimumSize(800, 600);
    qDebug() << "BgfxBlockRenderer: Initialized";
}

BgfxBlockRenderer::~BgfxBlockRenderer()
{
    qDebug() << "BgfxBlockRenderer: Destroyed";
}

void BgfxBlockRenderer::initializeResources()
{
    // 创建着色器程序
    m_program = loadProgram("vs_simple", "fs_simple");

    if (bgfx::isValid(m_program)) {
        qDebug() << "Shader program created successfully";
    } else {
        qWarning() << "Failed to create shader program - will use default rendering";
    }

    // 创建几何体
    createRectangleGeometry(m_vertexBuffer, m_indexBuffer, m_vertexLayout);

    if (bgfx::isValid(m_vertexBuffer) && bgfx::isValid(m_indexBuffer)) {
        qDebug() << "Rectangle geometry created successfully";
    } else {
        qWarning() << "Failed to create rectangle geometry";
    }
}

void BgfxBlockRenderer::cleanupResources()
{
    // 清理bgfx资源
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
        m_program = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_vertexBuffer)) {
        bgfx::destroy(m_vertexBuffer);
        m_vertexBuffer = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_indexBuffer)) {
        bgfx::destroy(m_indexBuffer);
        m_indexBuffer = BGFX_INVALID_HANDLE;
    }
}

void BgfxBlockRenderer::render()
{
    static int renderCount = 0;
    renderCount++;

    if (renderCount % 60 == 1) { // 每秒打印一次
        qDebug() << "BgfxBlockRenderer::render - frame" << renderCount;
    }

    // 渲染调试信息
    renderDebugInfo();

    // 渲染测试几何体
    renderTestGeometry();
}



void BgfxBlockRenderer::renderDebugInfo()
{
    // 使用bgfx的调试文本系统
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 1, 0x4f, "=== TinaFlow Block Renderer ===");
    bgfx::dbgTextPrintf(0, 2, 0x0f, "Widget Size: %dx%d", width(), height());
    bgfx::dbgTextPrintf(0, 3, 0x0f, "Real Size: %.0fx%.0f", realWidth(), realHeight());
    bgfx::dbgTextPrintf(0, 4, 0x0f, "DPI Ratio: %.2f", devicePixelRatio());
    bgfx::dbgTextPrintf(0, 5, 0x0f, "Renderer: %s", bgfx::getRendererName(bgfx::getRendererType()));
    bgfx::dbgTextPrintf(0, 6, 0x0f, "View ID: %d", getViewId());
    bgfx::dbgTextPrintf(0, 7, 0x0f, "Initialized: %s", isBgfxInitialized() ? "YES" : "NO");
    bgfx::dbgTextPrintf(0, 8, 0x0f, "Shader Program: %s", bgfx::isValid(m_program) ? "VALID" : "INVALID");
    bgfx::dbgTextPrintf(0, 9, 0x0f, "Geometry: %s", (bgfx::isValid(m_vertexBuffer) && bgfx::isValid(m_indexBuffer)) ? "VALID" : "INVALID");
    bgfx::dbgTextPrintf(0, 10, 0x0f, "Zoom: %.2f", getZoom());
    bgfx::dbgTextPrintf(0, 11, 0x0f, "Pan: (%.1f, %.1f)", getPan().x(), getPan().y());
    bgfx::dbgTextPrintf(0, 12, 0x6f, "Middle mouse: Pan | Wheel: Zoom");
}

void BgfxBlockRenderer::renderTestGeometry()
{
    // 只有在几何体和着色器都有效时才渲染
    if (!bgfx::isValid(m_vertexBuffer) || !bgfx::isValid(m_indexBuffer)) {
        static bool warned = false;
        if (!warned) {
            qWarning() << "BgfxBlockRenderer: Invalid geometry buffers!";
            warned = true;
        }
        return;
    }

    // 获取基类计算好的变换矩阵
    float transform[16];
    getTransformMatrix(transform);

    // 设置变换矩阵
    bgfx::setTransform(transform);

    // 设置顶点和索引缓冲区
    bgfx::setVertexBuffer(0, m_vertexBuffer);
    bgfx::setIndexBuffer(m_indexBuffer);

    // 设置渲染状态 (启用深度测试和写入)
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);

    // 提交绘制调用
    if (bgfx::isValid(m_program)) {
        bgfx::submit(getViewId(), m_program);

        static int submitCount = 0;
        submitCount++;
        if (submitCount % 60 == 1) {
            qDebug() << "BgfxBlockRenderer: Submitted geometry with shader, frame" << submitCount;
        }
    } else {
        qWarning() << "BgfxBlockRenderer: Invalid shader program!";
        // 如果没有着色器程序，仍然提交以显示几何体（使用内置着色器）
        bgfx::submit(getViewId(), BGFX_INVALID_HANDLE);
    }
}
