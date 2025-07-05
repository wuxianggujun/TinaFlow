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

// 创建凸起积木几何体
static void createConnectorBlockGeometry(bgfx::VertexBufferHandle& vbh, bgfx::IndexBufferHandle& ibh, bgfx::VertexLayout& layout)
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

// 创建凹陷积木几何体 - 简化版本，只有主体
static void createReceptorBlockGeometry(bgfx::VertexBufferHandle& vbh, bgfx::IndexBufferHandle& ibh, bgfx::VertexLayout& layout)
{
    // 初始化顶点布局
    PosColorTexVertex::init();
    layout = PosColorTexVertex::ms_layout;

    // 积木尺寸定义
    const float blockWidth = 120.0f;
    const float blockHeight = 40.0f;

    // 主体颜色 (ABGR格式) - 使用不同的颜色来区分
    const uint32_t mainColor = 0xff4ae290;      // 绿色主体 (ABGR: FF 4A E2 90)

    // 创建简单的绿色矩形积木 - 先确保主体能正确显示
    static PosColorTexVertex vertices[] = {
        // 主体矩形 - 完整的绿色矩形
        {-blockWidth/2, -blockHeight/2, 0.0f, mainColor, -1.0f, -1.0f}, // 0: 左下
        { blockWidth/2, -blockHeight/2, 0.0f, mainColor,  1.0f, -1.0f}, // 1: 右下
        { blockWidth/2,  blockHeight/2, 0.0f, mainColor,  1.0f,  1.0f}, // 2: 右上
        {-blockWidth/2,  blockHeight/2, 0.0f, mainColor, -1.0f,  1.0f}, // 3: 左上
    };

    // 创建简单的索引数据 - 只有主体矩形
    static uint16_t indices[] = {
        // 主体矩形
        0, 1, 2,  2, 3, 0,
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
        if (!bgfx::isValid(m_roundedParamsUniform)) {
            qWarning() << "Failed to create rounded params uniform";
        }

        // 创建连接器配置的uniform
        m_connectorConfigUniform = bgfx::createUniform("u_connectorConfig", bgfx::UniformType::Vec4);
        if (!bgfx::isValid(m_connectorConfigUniform)) {
            qWarning() << "Failed to create connector config uniform";
        }
    } else {
        qWarning() << "Failed to create rounded shader program - falling back to simple shader";
        // 回退到简单着色器
        m_program = loadProgram("vs_simple", "fs_simple");
    }

    // 创建凸起积木几何体
    createConnectorBlockGeometry(m_connectorVertexBuffer, m_connectorIndexBuffer, m_vertexLayout);

    if (!bgfx::isValid(m_connectorVertexBuffer) || !bgfx::isValid(m_connectorIndexBuffer)) {
        qWarning() << "Failed to create connector block geometry";
    }

    // 创建凹陷积木几何体
    createReceptorBlockGeometry(m_receptorVertexBuffer, m_receptorIndexBuffer, m_vertexLayout);

    if (!bgfx::isValid(m_receptorVertexBuffer) || !bgfx::isValid(m_receptorIndexBuffer)) {
        qWarning() << "Failed to create receptor block geometry";
    }
}

void BgfxBlockRenderer::cleanupResources()
{
    // 清理bgfx资源
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
        m_program = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_connectorVertexBuffer)) {
        bgfx::destroy(m_connectorVertexBuffer);
        m_connectorVertexBuffer = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_connectorIndexBuffer)) {
        bgfx::destroy(m_connectorIndexBuffer);
        m_connectorIndexBuffer = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_receptorVertexBuffer)) {
        bgfx::destroy(m_receptorVertexBuffer);
        m_receptorVertexBuffer = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_receptorIndexBuffer)) {
        bgfx::destroy(m_receptorIndexBuffer);
        m_receptorIndexBuffer = BGFX_INVALID_HANDLE;
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
    // 渲染测试几何体
    renderTestGeometry();
}



void BgfxBlockRenderer::renderDebugInfo()
{
    // 简化的调试信息 - 只在需要时显示
    // bgfx::dbgTextClear();
    // bgfx::dbgTextPrintf(0, 1, 0x4f, "TinaFlow Block Renderer");
}

void BgfxBlockRenderer::renderTestGeometry()
{
    // 检查资源是否有效 - 分别检查
    bool connectorValid = bgfx::isValid(m_connectorVertexBuffer) && bgfx::isValid(m_connectorIndexBuffer);
    bool receptorValid = bgfx::isValid(m_receptorVertexBuffer) && bgfx::isValid(m_receptorIndexBuffer);

    static bool warned = false;
    if (!warned) {
        qDebug() << "BgfxBlockRenderer: Connector buffers valid:" << connectorValid;
        qDebug() << "BgfxBlockRenderer: Receptor buffers valid:" << receptorValid;
        qDebug() << "BgfxBlockRenderer: Connector VB:" << bgfx::isValid(m_connectorVertexBuffer)
                 << "IB:" << bgfx::isValid(m_connectorIndexBuffer);
        qDebug() << "BgfxBlockRenderer: Receptor VB:" << bgfx::isValid(m_receptorVertexBuffer)
                 << "IB:" << bgfx::isValid(m_receptorIndexBuffer);
        warned = true;
    }

    if (!connectorValid && !receptorValid) {
        qWarning() << "BgfxBlockRenderer: No valid geometry buffers!";
        return;
    }

    // 获取基类计算好的变换矩阵
    float transform[16];
    getTransformMatrix(transform);

    // 计算合适的间距 - 基于积木宽度和缩放
    float zoom = getZoom();
    float blockWidth = 120.0f;
    float spacing = (blockWidth + 50.0f) * zoom; // 积木宽度 + 50像素间距，随缩放调整

    // 渲染凸起积木 (左侧) - 只有在资源有效时才渲染
    if (connectorValid) {
        // 设置变换矩阵 - 左侧位置
        float leftTransform[16];
        getTransformMatrix(leftTransform);
        // 向左偏移
        leftTransform[12] -= spacing;
        bgfx::setTransform(leftTransform);

        // 设置顶点和索引缓冲区
        bgfx::setVertexBuffer(0, m_connectorVertexBuffer);
        bgfx::setIndexBuffer(m_connectorIndexBuffer);

        // 设置渲染状态 (每次都重新设置)
        bgfx::setState(BGFX_STATE_WRITE_RGB
                     | BGFX_STATE_WRITE_A
                     | BGFX_STATE_WRITE_Z
                     | BGFX_STATE_DEPTH_TEST_LESS
                     | BGFX_STATE_BLEND_ALPHA);

        // 设置圆角参数uniform
        if (bgfx::isValid(m_roundedParamsUniform)) {
            float roundedParams[4] = {120.0f, 40.0f, 8.0f, 0.0f};
            bgfx::setUniform(m_roundedParamsUniform, roundedParams);
        }

        // 设置连接器配置uniform (顶部凸起)
        if (bgfx::isValid(m_connectorConfigUniform)) {
            float connectorConfig[4] = {1.0f, 0.0f, 0.0f, 0.0f}; // top=1
            bgfx::setUniform(m_connectorConfigUniform, connectorConfig);
        }

        // 提交凸起积木
        if (bgfx::isValid(m_program)) {
            bgfx::submit(getViewId(), m_program);
            static bool logged1 = false;
            if (!logged1) {
                qDebug() << "BgfxBlockRenderer: Submitted connector block";
                logged1 = true;
            }
        }
    }

    // 渲染凹陷积木 (右侧) - 只有在资源有效时才渲染
    if (receptorValid) {
        // 设置变换矩阵 - 右侧位置
        float rightTransform[16];
        getTransformMatrix(rightTransform);
        // 向右偏移
        rightTransform[12] += spacing;
        bgfx::setTransform(rightTransform);

        // 设置顶点和索引缓冲区
        bgfx::setVertexBuffer(0, m_receptorVertexBuffer);
        bgfx::setIndexBuffer(m_receptorIndexBuffer);

        // 设置渲染状态 (每次都重新设置)
        bgfx::setState(BGFX_STATE_WRITE_RGB
                     | BGFX_STATE_WRITE_A
                     | BGFX_STATE_WRITE_Z
                     | BGFX_STATE_DEPTH_TEST_LESS
                     | BGFX_STATE_BLEND_ALPHA);

        // 设置圆角参数uniform
        if (bgfx::isValid(m_roundedParamsUniform)) {
            float roundedParams[4] = {120.0f, 40.0f, 8.0f, 0.0f};
            bgfx::setUniform(m_roundedParamsUniform, roundedParams);
        }

        // 设置连接器配置uniform (暂时使用相同配置测试)
        if (bgfx::isValid(m_connectorConfigUniform)) {
            float connectorConfig[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // 无连接器，只有主体
            bgfx::setUniform(m_connectorConfigUniform, connectorConfig);
        }

        // 提交凹陷积木
        if (bgfx::isValid(m_program)) {
            bgfx::submit(getViewId(), m_program);
            static bool logged2 = false;
            if (!logged2) {
                qDebug() << "BgfxBlockRenderer: Submitted receptor block";
                logged2 = true;
            }
        }
    }

    // 减少日志输出 - 只在出错时打印
    // static int submitCount = 0;
    // submitCount++;
    // if (submitCount % 300 == 1) { // 每5秒打印一次
    //     qDebug() << "BgfxBlockRenderer: Rendering blocks, frame" << submitCount;
    // }
}
