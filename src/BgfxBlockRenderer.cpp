#include "BgfxBlockRenderer.hpp"
#include "BgfxVertexTypes.hpp"
#include "BgfxResourceManager.hpp"
#include <QDebug>
#include <QTimer>
#include <vector>
#include <cmath>

// 旧的着色器加载函数已移除，现在使用BgfxResourceManager

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
    // 几何体管理器会自动清理资源
    // 统一资源管理器会处理着色器和uniform的清理
}

void BgfxBlockRenderer::render()
{
    // 获取基础变换矩阵
    float baseTransform[16];
    getTransformMatrix(baseTransform);

    // 从统一资源管理器获取资源
    auto& resourceManager = BgfxResourceManager::instance();
    bgfx::ProgramHandle program = resourceManager.getShaderProgram(BgfxResources::ROUNDED_SHADER);
    bgfx::UniformHandle roundedParamsUniform = resourceManager.getUniform(BgfxResources::ROUNDED_PARAMS, bgfx::UniformType::Vec4);
    bgfx::UniformHandle connectorConfigUniform = resourceManager.getUniform(BgfxResources::CONNECTOR_CONFIG, bgfx::UniformType::Vec4);

    // 使用几何体管理器渲染所有积木
    m_geometryManager.render(getViewId(), program,
                           roundedParamsUniform, connectorConfigUniform,
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

    // 使用固定的逻辑坐标 - 不随缩放变化
    float blockWidth = 120.0f;
    float spacing = blockWidth + 50.0f; // 固定间距：170像素

    // 添加测试积木 - 使用固定坐标
    addBlock(-spacing, 0.0f, 1, 0xffe2904a);  // 左侧凸起积木 (蓝色)
    addBlock(spacing, 0.0f, -1, 0xff4ae290);  // 右侧凹陷积木 (绿色)

    // 可以添加更多积木进行测试
    // addBlock(0.0f, spacing, 0, 0xffffff90);   // 中上简单积木 (黄色)
    // addBlock(0.0f, -spacing, 1, 0xffff4a90);  // 中下凸起积木 (紫色)

    qDebug() << "BgfxBlockRenderer: Created test blocks with fixed spacing:" << spacing;
    qDebug() << "Block positions: left(" << -spacing << ", 0) right(" << spacing << ", 0)";
}

void BgfxBlockRenderer::updateBlockPositions()
{
    // 不需要重新创建积木 - 使用固定坐标
    // createTestBlocks();
}

void BgfxBlockRenderer::setZoom(float zoom)
{
    // 调用基类方法
    BgfxWidget::setZoom(zoom);

    // 不需要更新积木位置 - 让变换矩阵处理缩放
    // updateBlockPositions();
}

void BgfxBlockRenderer::onBgfxReset()
{
    // 标记统一资源管理器的资源为无效
    BgfxResourceManager::instance().invalidateResources();

    // 标记所有几何体资源为无效
    m_geometryManager.invalidateResources();

    // 重新初始化资源
    initializeResources();
}
