#include "BgfxBlockRenderer.hpp"
#include "BgfxVertexTypes.hpp"
#include "BgfxResourceManager.hpp"
#include <QDebug>
#include <QTimer>
#include <QMouseEvent>
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

    // 简化测试：只创建一个积木在明显的位置
    addBlock(200.0f, 150.0f, 1, 0xff0000ff);   // 红色积木，在屏幕中央偏上

    qDebug() << "BgfxBlockRenderer: Created single test block at (200, 150)";
    qDebug() << "Block boundary should be: [140, 130] to [260, 170]";
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

// 积木选择和交互接口实现
void BgfxBlockRenderer::selectBlock(int blockId, bool multiSelect)
{
    if (!multiSelect) {
        // 单选模式：清除其他选择
        m_geometryManager.clearSelection();
    }

    // 选中指定积木
    m_geometryManager.setBlockSelected(blockId, true);

    // 添加调试信息
    std::vector<int> selectedBlocks = m_geometryManager.getSelectedBlocks();
    qDebug() << "BgfxBlockRenderer: Selected blocks:" << selectedBlocks.size();
    for (int id : selectedBlocks) {
        qDebug() << "  Block" << id << "is selected";
    }

    update(); // 触发重绘
}

void BgfxBlockRenderer::clearSelection()
{
    m_geometryManager.clearSelection();
    update(); // 触发重绘
}

std::vector<int> BgfxBlockRenderer::getSelectedBlocks() const
{
    return m_geometryManager.getSelectedBlocks();
}

void BgfxBlockRenderer::moveSelectedBlocks(float deltaX, float deltaY)
{
    std::vector<int> selectedBlocks = getSelectedBlocks();
    for (int blockId : selectedBlocks) {
        BlockInstance* block = m_geometryManager.getBlockById(blockId);
        if (block) {
            m_geometryManager.moveBlock(blockId, block->x + deltaX, block->y + deltaY);
        }
    }
    update(); // 触发重绘
}

void BgfxBlockRenderer::resetView()
{
    // 重置缩放和平移到初始状态
    setZoom(1.0f);
    setPan(QPointF(0, 0));
    qDebug() << "BgfxBlockRenderer: View reset to initial state";
}

// 鼠标事件处理实现
void BgfxBlockRenderer::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF mousePos = event->position();

        // 转换为世界坐标
        QPointF worldPos = screenToWorld(mousePos);

        // 简化调试信息
        qDebug() << "Click at:" << worldPos;

        // 查找点击的积木
        BlockInstance* clickedBlock = m_geometryManager.findBlockAt(worldPos.x(), worldPos.y());

        if (clickedBlock) {
            // 检查是否按住Ctrl键进行多选
            bool multiSelect = (event->modifiers() & Qt::ControlModifier) != 0;

            if (clickedBlock->isSelected) {
                if (multiSelect) {
                    // 如果积木已选中且按住Ctrl，则取消选择
                    qDebug() << "BgfxBlockRenderer: Deselecting block" << clickedBlock->blockId << "(Ctrl+click)";
                    m_geometryManager.setBlockSelected(clickedBlock->blockId, false);
                } else {
                    // 如果积木已选中且没按Ctrl，则取消选择（切换模式）
                    m_geometryManager.clearSelection();
                }
            } else {
                // 选择积木
                selectBlock(clickedBlock->blockId, multiSelect);
            }

            // 开始拖动
            m_isBlockDragging = true;
            m_draggingBlocks = getSelectedBlocks();
            m_dragStartPos = mousePos;
            m_dragLastPos = mousePos;

            // 设置拖动状态
            for (int blockId : m_draggingBlocks) {
                BlockInstance* block = m_geometryManager.getBlockById(blockId);
                if (block) {
                    block->isDragging = true;
                }
            }

            qDebug() << "BgfxBlockRenderer: Started dragging" << m_draggingBlocks.size() << "blocks";
        } else {
            // 点击空白区域，清除选择
            if (!(event->modifiers() & Qt::ControlModifier)) {
                clearSelection();
            }
        }

        update(); // 触发重绘
        return; // 不调用基类，避免视图平移
    }

    // 其他按键传递给基类处理（如中键平移）
    BgfxWidget::mousePressEvent(event);
}

void BgfxBlockRenderer::mouseMoveEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();

    // 发射鼠标世界坐标变化信号
    QPointF worldPos = screenToWorld(mousePos);
    emit mouseWorldPosChanged(worldPos.x(), worldPos.y());

    if (m_isBlockDragging && !m_draggingBlocks.empty()) {
        // 计算鼠标移动的距离（屏幕坐标）
        QPointF screenDelta = mousePos - m_dragLastPos;

        // 转换为世界坐标的移动距离
        // 由于缩放的影响，需要除以当前缩放级别
        float worldDeltaX = screenDelta.x() / getZoom();
        float worldDeltaY = screenDelta.y() / getZoom();

        // 移动所有拖动中的积木
        for (int blockId : m_draggingBlocks) {
            BlockInstance* block = m_geometryManager.getBlockById(blockId);
            if (block) {
                m_geometryManager.moveBlock(blockId, block->x + worldDeltaX, block->y + worldDeltaY);
            }
        }

        m_dragLastPos = mousePos;
        update(); // 触发重绘
        return; // 不调用基类，避免视图平移
    }

    // 传递给基类处理（如视图平移）
    BgfxWidget::mouseMoveEvent(event);
}

void BgfxBlockRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_isBlockDragging) {
        // 结束拖动
        m_isBlockDragging = false;

        // 清除拖动状态
        for (int blockId : m_draggingBlocks) {
            BlockInstance* block = m_geometryManager.getBlockById(blockId);
            if (block) {
                block->isDragging = false;
            }
        }

        qDebug() << "BgfxBlockRenderer: Finished dragging" << m_draggingBlocks.size() << "blocks";
        m_draggingBlocks.clear();

        update(); // 触发重绘
        return; // 不调用基类
    }

    // 传递给基类处理
    BgfxWidget::mouseReleaseEvent(event);
}
