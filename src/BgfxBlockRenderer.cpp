#include "BgfxBlockRenderer.hpp"
#include <QDebug>
#include <QTimer>
#include <vector>
#include <cmath>

// 包含bgfx工具函数用于加载着色器
#include <QFile>
#include <bgfx/bgfx.h>
#include <bx/file.h>

// 包含生成的统一着色器头文件
#include "shaders.h"

// 定义包含纹理坐标的顶点结构
struct PosColorTexVertex
{
    float x, y, z;
    uint32_t abgr;
    float u, v;

    static void init()
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
    }

    static bgfx::VertexLayout ms_layout;
};

// 定义静态成员
bgfx::VertexLayout PosColorTexVertex::ms_layout;

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

// 使用着色器实现圆角的简单矩形几何体
static void createBlockGeometry(bgfx::VertexBufferHandle& vbh, bgfx::IndexBufferHandle& ibh, bgfx::VertexLayout& layout)
{
    // 初始化顶点布局
    PosColorTexVertex::init();
    layout = PosColorTexVertex::ms_layout;

    // 积木尺寸定义
    const float blockWidth = 120.0f;
    const float blockHeight = 40.0f;

    // 主体颜色 (ABGR格式)
    const uint32_t mainColor = 0xffe2904a;      // 蓝色主体 (ABGR: FF E2 90 4A)
    const uint32_t connectorColor = mainColor;  // 连接器使用相同颜色

    // 创建完整的积木几何体，包含主体和连接器的连续网格
    static PosColorTexVertex vertices[] = {
        // 主体矩形的基础部分
        {-blockWidth/2, -blockHeight/2, 0.0f, mainColor, -1.0f, -1.0f}, // 0: 左下
        { blockWidth/2, -blockHeight/2, 0.0f, mainColor,  1.0f, -1.0f}, // 1: 右下
        { blockWidth/2,  blockHeight/2, 0.0f, mainColor,  1.0f,  1.0f}, // 2: 右上
        {-blockWidth/2,  blockHeight/2, 0.0f, mainColor, -1.0f,  1.0f}, // 3: 左上

        // 左连接器区域的顶点 - 确保与主体无缝连接
        {-blockWidth/4 - 6.0f,  blockHeight/2, 0.0f, mainColor, -0.2f,  0.95f}, // 4: 左连接器左下
        {-blockWidth/4 + 6.0f,  blockHeight/2, 0.0f, mainColor,  0.2f,  0.95f}, // 5: 左连接器右下
        {-blockWidth/4 + 6.0f,  blockHeight/2 + 4.0f, 0.0f, mainColor,  0.2f,  1.5f}, // 6: 左连接器右上
        {-blockWidth/4 - 6.0f,  blockHeight/2 + 4.0f, 0.0f, mainColor, -0.2f,  1.5f}, // 7: 左连接器左上

        // 右连接器区域的顶点 - 确保与主体无缝连接
        { blockWidth/4 - 6.0f,  blockHeight/2, 0.0f, mainColor, -0.2f,  0.95f}, // 8: 右连接器左下
        { blockWidth/4 + 6.0f,  blockHeight/2, 0.0f, mainColor,  0.2f,  0.95f}, // 9: 右连接器右下
        { blockWidth/4 + 6.0f,  blockHeight/2 + 4.0f, 0.0f, mainColor,  0.2f,  1.5f}, // 10: 右连接器右上
        { blockWidth/4 - 6.0f,  blockHeight/2 + 4.0f, 0.0f, mainColor, -0.2f,  1.5f}, // 11: 右连接器左上
    };

    // 创建连续的索引数据，形成完整的积木形状
    static uint16_t indices[] = {
        // 主体矩形
        0, 1, 2,  2, 3, 0,

        // 左连接器
        4, 5, 6,  6, 7, 4,

        // 右连接器
        8, 9, 10, 10, 11, 8,
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
    // 创建圆角着色器程序
    m_program = loadProgram("vs_rounded", "fs_rounded");

    if (bgfx::isValid(m_program)) {
        qDebug() << "Rounded shader program created successfully";

        // 创建圆角参数的uniform
        m_roundedParamsUniform = bgfx::createUniform("u_roundedParams", bgfx::UniformType::Vec4);
        if (bgfx::isValid(m_roundedParamsUniform)) {
            qDebug() << "Created rounded params uniform";
        } else {
            qWarning() << "Failed to create rounded params uniform";
        }

        // 创建连接器配置的uniform
        m_connectorConfigUniform = bgfx::createUniform("u_connectorConfig", bgfx::UniformType::Vec4);
        if (bgfx::isValid(m_connectorConfigUniform)) {
            qDebug() << "Created connector config uniform";
        } else {
            qWarning() << "Failed to create connector config uniform";
        }
    } else {
        qWarning() << "Failed to create rounded shader program - falling back to simple shader";
        // 回退到简单着色器
        m_program = loadProgram("vs_simple", "fs_simple");
    }

    // 创建积木几何体
    createBlockGeometry(m_vertexBuffer, m_indexBuffer, m_vertexLayout);

    if (bgfx::isValid(m_vertexBuffer) && bgfx::isValid(m_indexBuffer)) {
        qDebug() << "Block geometry created successfully";
    } else {
        qWarning() << "Failed to create block geometry";
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

    if (bgfx::isValid(m_roundedParamsUniform)) {
        bgfx::destroy(m_roundedParamsUniform);
        m_roundedParamsUniform = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_connectorConfigUniform)) {
        bgfx::destroy(m_connectorConfigUniform);
        m_connectorConfigUniform = BGFX_INVALID_HANDLE;
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

    // 设置圆角参数uniform
    if (bgfx::isValid(m_roundedParamsUniform)) {
        // 设置圆角参数：width=120, height=40, cornerRadius=8, unused=0
        float roundedParams[4] = {120.0f, 40.0f, 8.0f, 0.0f};
        bgfx::setUniform(m_roundedParamsUniform, roundedParams);
    }

    // 设置连接器配置uniform
    if (bgfx::isValid(m_connectorConfigUniform)) {
        // 设置连接器配置：top=1, bottom=0, left=0, right=0 (当前只有顶部连接器)
        float connectorConfig[4] = {1.0f, 0.0f, 0.0f, 0.0f};
        bgfx::setUniform(m_connectorConfigUniform, connectorConfig);
    }

    // 设置渲染状态 (启用透明度混合和深度测试)
    bgfx::setState(BGFX_STATE_WRITE_RGB
                 | BGFX_STATE_WRITE_A
                 | BGFX_STATE_WRITE_Z
                 | BGFX_STATE_DEPTH_TEST_LESS
                 | BGFX_STATE_BLEND_ALPHA);

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
