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

void BgfxBlockRenderer::setZoom(float zoom)
{
    // 调用基类方法
    BgfxWidget::setZoom(zoom);

    // 更新积木位置以适应新的缩放级别
    updateBlockPositions();
}

void BgfxBlockRenderer::onBgfxReset()
{
    // 标记所有几何体资源为无效
    m_geometryManager.invalidateResources();

    // 重新初始化资源
    initializeResources();
}
