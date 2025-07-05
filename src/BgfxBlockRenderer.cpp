#include "BgfxBlockRenderer.hpp"
#include "BgfxVertexTypes.hpp"
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

    // 初始化顶点布局
    PosColorTexVertex::init();
    m_vertexLayout = PosColorTexVertex::ms_layout;

    // 初始化几何体管理器
    if (!m_geometryManager.initialize(m_vertexLayout)) {
        qWarning() << "Failed to initialize geometry manager";
        return;
    }

    // 创建测试积木
    createTestBlocks();
}

void BgfxBlockRenderer::cleanupResources()
{
    // 清理bgfx资源
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
        m_program = BGFX_INVALID_HANDLE;
    }

    // 几何体管理器会自动清理资源

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
    // 检查缩放是否变化，如果变化则更新积木位置
    static float lastZoom = -1.0f;
    float currentZoom = getZoom();
    if (abs(currentZoom - lastZoom) > 0.001f) {
        lastZoom = currentZoom;
        updateBlockPositions();
    }

    // 获取基础变换矩阵
    float baseTransform[16];
    getTransformMatrix(baseTransform);

    // 使用几何体管理器渲染所有积木
    m_geometryManager.render(getViewId(), m_program,
                           m_roundedParamsUniform, m_connectorConfigUniform,
                           baseTransform);
}

void BgfxBlockRenderer::addBlock(float x, float y, int connectorType, uint32_t color)
{
    BlockInstance instance(x, y, 0.0f, color, connectorType);
    m_geometryManager.addBlock(instance);
}

void BgfxBlockRenderer::clearBlocks()
{
    m_geometryManager.clearBlocks();
}

void BgfxBlockRenderer::createTestBlocks()
{
    // 清除现有积木
    clearBlocks();

    // 使用与之前相同的逻辑：间距随缩放动态调整
    float zoom = getZoom();
    float blockWidth = 120.0f;
    float spacing = (blockWidth + 50.0f) * zoom; // 积木宽度 + 50像素间距，随缩放调整

    // 添加测试积木 - 使用动态间距
    addBlock(-spacing, 0.0f, 1, 0xffe2904a);  // 左侧凸起积木 (蓝色)
    addBlock(spacing, 0.0f, -1, 0xff4ae290);  // 右侧凹陷积木 (绿色)

    // 可以添加更多积木进行测试
    // addBlock(0.0f, spacing, 0, 0xffffff90);   // 中上简单积木 (黄色)
    // addBlock(0.0f, -spacing, 1, 0xffff4a90);  // 中下凸起积木 (紫色)

    qDebug() << "BgfxBlockRenderer: Created test blocks with zoom:" << zoom << "spacing:" << spacing;
}

void BgfxBlockRenderer::updateBlockPositions()
{
    // 重新创建积木以适应新的缩放级别
    createTestBlocks();
}

/*void BgfxBlockRenderer::renderTestGeometry()
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
}*/
