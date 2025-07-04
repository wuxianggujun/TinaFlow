#include "BgfxBlockRenderer.hpp"
#include <QDebug>
#include <QApplication>
#include <QWindow>
#include <QPainter>
#include <QTimer>
#include <QtMath>
#include <cstring>

#ifdef Q_OS_WIN
#include <Windows.h>
#elif defined(Q_OS_LINUX)
#include <X11/Xlib.h>
#elif defined(Q_OS_MACOS)
#include <Cocoa/Cocoa.h>
#endif

BgfxBlockRenderer::BgfxBlockRenderer(QWidget* parent)
    : QWidget(parent)
    , m_font("Arial", 12)
    , m_fontMetrics(m_font)
{
    setMinimumSize(800, 600);

    // 这些属性对bgfx很重要
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    // 设置渲染定时器
    connect(&m_renderTimer, &QTimer::timeout, this, &BgfxBlockRenderer::onRenderTimer);
    m_renderTimer.start(16); // 60 FPS

    qDebug() << "BgfxBlockRenderer: Initialized";
}

BgfxBlockRenderer::~BgfxBlockRenderer()
{
    m_renderTimer.stop();
    shutdownBgfx();
    qDebug() << "BgfxBlockRenderer: Destroyed";
}

bool BgfxBlockRenderer::initializeBgfx()
{
    if (m_bgfxInitialized) {
        return true;
    }

    // 确保窗口已经显示并且有有效的句柄
    if (!isVisible() || width() <= 0 || height() <= 0) {
        qDebug() << "Widget not ready for bgfx initialization - visible:" << isVisible()
                 << "size:" << width() << "x" << height();
        return false;
    }

    // 强制创建原生窗口
    createWinId();

    // 获取原生窗口句柄
    WId windowId = winId();
    if (windowId == 0) {
        qCritical() << "Failed to get valid window ID";
        return false;
    }

#ifdef Q_OS_WIN
    // Windows: 直接使用HWND
    m_nativeHandle = reinterpret_cast<void*>(windowId);
#elif defined(Q_OS_LINUX)
    // Linux: 使用X11窗口ID
    m_nativeHandle = reinterpret_cast<void*>(windowId);
#elif defined(Q_OS_MACOS)
    // macOS: 使用NSView
    m_nativeHandle = reinterpret_cast<void*>(windowId);
#endif

    qDebug() << "Window ID:" << windowId << "Native handle:" << m_nativeHandle;

    // 设置bgfx平台数据
    bgfx::PlatformData platformData;
    memset(&platformData, 0, sizeof(platformData));
    platformData.nwh = m_nativeHandle;

#ifdef Q_OS_WIN
    platformData.ndt = nullptr; // Windows不需要display
#elif defined(Q_OS_LINUX)
    // Linux可能需要X11 Display，但通常nullptr也可以
    platformData.ndt = nullptr;
#endif

    // 先设置平台数据
    bgfx::setPlatformData(platformData);

    qDebug() << "Platform data set - nwh:" << platformData.nwh << "ndt:" << platformData.ndt;

    // 初始化bgfx
    bgfx::Init init;

    // 明确指定渲染器类型，而不是自动选择
#ifdef Q_OS_WIN
    init.type = bgfx::RendererType::Direct3D11; // Windows优先使用D3D11
#elif defined(Q_OS_LINUX)
    init.type = bgfx::RendererType::OpenGL;     // Linux使用OpenGL
#elif defined(Q_OS_MACOS)
    init.type = bgfx::RendererType::Metal;      // macOS使用Metal
#else
    init.type = bgfx::RendererType::OpenGL;     // 默认使用OpenGL
#endif

    init.vendorId = BGFX_PCI_ID_NONE;
    init.deviceId = 0;
    init.capabilities = UINT64_MAX;
    init.debug = true; // 启用调试模式
    init.profile = false;

    // 设置分辨率
    init.resolution.width = static_cast<uint32_t>(width());
    init.resolution.height = static_cast<uint32_t>(height());
    init.resolution.reset = BGFX_RESET_VSYNC | BGFX_RESET_MSAA_X4; // 添加抗锯齿
    init.resolution.format = bgfx::TextureFormat::BGRA8;
    init.resolution.numBackBuffers = 2;

    // 设置平台数据
    init.platformData = platformData;

    qDebug() << "Attempting to initialize bgfx with:";
    qDebug() << "  Resolution:" << width() << "x" << height();
    qDebug() << "  Renderer:" << (int)init.type;
    qDebug() << "  Debug:" << init.debug;

    bool success = bgfx::init(init);
    if (!success) {
        qCritical() << "bgfx::init() returned false";

        // 尝试使用更简单的设置
        qDebug() << "Trying fallback initialization...";
        bgfx::Init fallbackInit;
        fallbackInit.type = bgfx::RendererType::Count; // 自动选择
        fallbackInit.vendorId = BGFX_PCI_ID_NONE;
        fallbackInit.resolution.width = static_cast<uint32_t>(width());
        fallbackInit.resolution.height = static_cast<uint32_t>(height());
        fallbackInit.resolution.reset = BGFX_RESET_NONE; // 不使用特殊标志
        fallbackInit.platformData = platformData;

        success = bgfx::init(fallbackInit);
        if (!success) {
            qCritical() << "Fallback bgfx initialization also failed";
            return false;
        }
        qDebug() << "Fallback bgfx initialization succeeded";
    }
    
    // 设置调试标志
    bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);

    // 获取渲染器信息
    const bgfx::Caps* caps = bgfx::getCaps();
    qDebug() << "bgfx initialized successfully!";
    qDebug() << "Renderer:" << bgfx::getRendererName(bgfx::getRendererType());
    qDebug() << "Vendor ID:" << caps->vendorId;
    qDebug() << "Device ID:" << caps->deviceId;

    // 设置视图
    bgfx::setViewClear(m_viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
                      0x404040ff, 1.0f, 0); // 深灰色背景，更容易看到
    bgfx::setViewRect(m_viewId, 0, 0, static_cast<uint16_t>(width()), static_cast<uint16_t>(height()));

    // 创建uniform（暂时跳过着色器，先确保基本渲染工作）
    m_colorUniform = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);
    m_transformUniform = bgfx::createUniform("u_transform", bgfx::UniformType::Mat4);

    // 初始化矩阵
    bx::mtxIdentity(m_viewMatrix);
    bx::mtxOrtho(m_projMatrix, 0.0f, static_cast<float>(width()),
                static_cast<float>(height()), 0.0f, -1.0f, 1.0f, 0.0f, caps->homogeneousDepth);

    m_bgfxInitialized = true;

    // 创建示例积木
    createSampleBlocks();

    qDebug() << "bgfx setup completed successfully";

    return true;
}

void BgfxBlockRenderer::shutdownBgfx()
{
    if (!m_bgfxInitialized) {
        return;
    }
    
    // 清理bgfx资源
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
    }
    if (bgfx::isValid(m_vertexBuffer)) {
        bgfx::destroy(m_vertexBuffer);
    }
    if (bgfx::isValid(m_indexBuffer)) {
        bgfx::destroy(m_indexBuffer);
    }
    if (bgfx::isValid(m_colorUniform)) {
        bgfx::destroy(m_colorUniform);
    }
    if (bgfx::isValid(m_transformUniform)) {
        bgfx::destroy(m_transformUniform);
    }
    
    bgfx::shutdown();
    m_bgfxInitialized = false;
    
    qDebug() << "bgfx shutdown complete";
}

void BgfxBlockRenderer::createSampleBlocks()
{
    // 创建示例积木
    auto startBlock = BlockFactory::createBlock(BlockType::Start);
    startBlock->setPosition(QPointF(100, 100));
    addBlock(startBlock);
    
    auto variableBlock = BlockFactory::createBlock(BlockType::Variable);
    variableBlock->setPosition(QPointF(300, 100));
    addBlock(variableBlock);
    
    auto ifElseBlock = BlockFactory::createBlock(BlockType::IfElse);
    ifElseBlock->setPosition(QPointF(100, 200));
    addBlock(ifElseBlock);
    
    qDebug() << "BgfxBlockRenderer: Created" << m_blocks.size() << "sample blocks";
}

void BgfxBlockRenderer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    if (!m_bgfxInitialized) {
        // 尝试初始化bgfx
        if (!initializeBgfx()) {
            qDebug() << "bgfx initialization failed, will retry next frame";
            return;
        }
    }

    render();
}

void BgfxBlockRenderer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    if (m_bgfxInitialized) {
        bgfx::reset(width(), height(), BGFX_RESET_VSYNC);
        bgfx::setViewRect(m_viewId, 0, 0, width(), height());
        
        // 更新投影矩阵
        bx::mtxOrtho(m_projMatrix, 0.0f, static_cast<float>(width()),
                    static_cast<float>(height()), 0.0f, -1.0f, 1.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);
    }
    
    qDebug() << "BgfxBlockRenderer: Resized to" << width() << "x" << height();
}

void BgfxBlockRenderer::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    qDebug() << "BgfxBlockRenderer::showEvent - widget size:" << size() << "visible:" << isVisible();

    // 延迟初始化bgfx，确保窗口完全准备好
    QTimer::singleShot(200, this, [this]() {
        if (!m_bgfxInitialized && isVisible() && width() > 0 && height() > 0) {
            qDebug() << "Attempting delayed bgfx initialization";
            if (initializeBgfx()) {
                qDebug() << "bgfx initialization successful!";
                update(); // 触发重绘
            } else {
                qDebug() << "bgfx initialization failed, will retry";
            }
        }
    });
}

void BgfxBlockRenderer::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    // 不在这里关闭bgfx，因为可能只是临时隐藏
}

void BgfxBlockRenderer::onRenderTimer()
{
    if (isVisible() && m_bgfxInitialized) {
        update(); // 触发paintEvent
    }
}

void BgfxBlockRenderer::render()
{
    if (!m_bgfxInitialized) {
        return;
    }

    // 设置视图变换
    float viewMatrix[16];
    bx::mtxIdentity(viewMatrix);
    bx::mtxTranslate(viewMatrix, m_pan.x(), m_pan.y(), 0.0f);
    bx::mtxScale(viewMatrix, m_zoom, m_zoom, 1.0f);

    bgfx::setViewTransform(m_viewId, viewMatrix, m_projMatrix);

    // 清除屏幕 - 使用更明显的颜色来确认渲染工作
    uint32_t clearColor = 0x336699ff; // 蓝灰色背景
    bgfx::setViewClear(m_viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);

    // 暂时跳过复杂的渲染，先确保基本的清屏工作
    // renderGrid();
    // renderConnections();
    // renderBlocks();

    // 提交帧
    bgfx::frame();

    // 使用QPainter绘制文本覆盖层，显示状态信息
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16));

    QString statusText = QString("bgfx渲染器运行中\n分辨率: %1x%2\n渲染器: %3")
                        .arg(width())
                        .arg(height())
                        .arg(bgfx::getRendererName(bgfx::getRendererType()));

    painter.drawText(rect(), Qt::AlignCenter, statusText);
}

void BgfxBlockRenderer::renderGrid()
{
    // 暂时跳过网格渲染，需要创建线条几何体
}

void BgfxBlockRenderer::renderBlocks()
{
    for (auto& block : m_blocks) {
        renderBlock(block);
    }
}

void BgfxBlockRenderer::renderBlock(std::shared_ptr<Block> block)
{
    if (!block) return;
    
    QPointF pos = block->getPosition();
    QRectF bounds = block->getBounds();
    QColor color = block->getColor();
    
    // 如果选中，使用高亮颜色
    if (block->isSelected()) {
        color = color.lighter(120);
    }
    
    // 绘制积木主体
    QRectF blockRect(pos, QSizeF(bounds.width(), bounds.height()));
    renderRect(blockRect, color, true);
    
    // 绘制边框
    QColor borderColor = color.darker(150);
    renderRect(blockRect, borderColor, false);
    
    // 绘制连接点
    for (const auto& point : block->getConnectionPoints()) {
        QPointF pointPos = pos + point.position;
        QColor pointColor = point.isConnected ? QColor(0, 255, 0) : QColor(100, 100, 100);
        
        switch (point.type) {
            case ConnectionType::Input:
            case ConnectionType::Output:
                renderCircle(pointPos, 6, pointColor, true);
                break;
            case ConnectionType::Next:
            case ConnectionType::Previous:
                renderRect(QRectF(pointPos.x() - 6, pointPos.y() - 6, 12, 12), pointColor, true);
                break;
        }
    }
}

void BgfxBlockRenderer::renderConnections()
{
    for (auto& connection : m_connections) {
        renderConnection(connection);
    }
}

void BgfxBlockRenderer::renderConnection(std::shared_ptr<BlockConnection> connection)
{
    if (!connection) return;
    
    auto sourceBlock = getBlock(connection->sourceBlockId);
    auto targetBlock = getBlock(connection->targetBlockId);
    if (!sourceBlock || !targetBlock) return;
    
    auto sourcePoint = sourceBlock->getConnectionPoint(connection->sourcePointId);
    auto targetPoint = targetBlock->getConnectionPoint(connection->targetPointId);
    if (!sourcePoint || !targetPoint) return;
    
    QPointF startPos = sourceBlock->getPosition() + sourcePoint->position;
    QPointF endPos = targetBlock->getPosition() + targetPoint->position;
    
    renderLine(startPos, endPos, connection->color, 3.0f);
}

void BgfxBlockRenderer::renderText()
{
    // 使用QPainter绘制文本覆盖层
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(m_font);
    
    for (auto& block : m_blocks) {
        QPointF pos = worldToScreen(block->getPosition());
        QRectF bounds = block->getBounds();
        
        // 绘制积木标题
        QString title = block->getTitle();
        QRectF textRect(pos.x(), pos.y(), bounds.width() * m_zoom, bounds.height() * m_zoom);
        
        painter.setPen(Qt::white);
        painter.drawText(textRect, Qt::AlignCenter, title);
    }
}

// 几何渲染方法（暂时使用简化实现）
void BgfxBlockRenderer::renderRect(const QRectF& rect, const QColor& color, bool filled)
{
    // 暂时跳过bgfx矩形渲染，需要创建顶点缓冲区和着色器
    // 这里可以使用bgfx的immediate mode或者预创建的几何体
}

void BgfxBlockRenderer::renderCircle(const QPointF& center, float radius, const QColor& color, bool filled)
{
    // 暂时跳过bgfx圆形渲染
}

void BgfxBlockRenderer::renderLine(const QPointF& start, const QPointF& end, const QColor& color, float width)
{
    // 暂时跳过bgfx线条渲染
}

void BgfxBlockRenderer::renderRoundedRect(const QRectF& rect, float radius, const QColor& color, bool filled)
{
    // 暂时跳过bgfx圆角矩形渲染
}

void BgfxBlockRenderer::renderBezierCurve(const QPointF& start, const QPointF& end,
                                         const QPointF& control1, const QPointF& control2,
                                         const QColor& color, float width)
{
    // 暂时跳过bgfx贝塞尔曲线渲染
}

// 积木管理方法
void BgfxBlockRenderer::addBlock(std::shared_ptr<Block> block)
{
    if (block) {
        m_blocks.append(block);
        update();
    }
}

void BgfxBlockRenderer::removeBlock(const QString& blockId)
{
    for (int i = 0; i < m_blocks.size(); ++i) {
        if (m_blocks[i]->getId() == blockId) {
            m_blocks.removeAt(i);
            update();
            break;
        }
    }
}

std::shared_ptr<Block> BgfxBlockRenderer::getBlock(const QString& blockId)
{
    for (auto& block : m_blocks) {
        if (block->getId() == blockId) {
            return block;
        }
    }
    return nullptr;
}

void BgfxBlockRenderer::addConnection(std::shared_ptr<BlockConnection> connection)
{
    if (connection) {
        m_connections.append(connection);
        update();
    }
}

void BgfxBlockRenderer::removeConnection(const QString& connectionId)
{
    for (int i = 0; i < m_connections.size(); ++i) {
        if (m_connections[i]->id == connectionId) {
            m_connections.removeAt(i);
            update();
            break;
        }
    }
}

std::shared_ptr<BlockConnection> BgfxBlockRenderer::getConnection(const QString& connectionId)
{
    for (auto& connection : m_connections) {
        if (connection->id == connectionId) {
            return connection;
        }
    }
    return nullptr;
}

void BgfxBlockRenderer::selectBlock(const QString& blockId)
{
    for (auto& block : m_blocks) {
        block->setSelected(block->getId() == blockId);
    }
    update();
}

void BgfxBlockRenderer::deselectAll()
{
    for (auto& block : m_blocks) {
        block->setSelected(false);
    }
    update();
}

QVector<QString> BgfxBlockRenderer::getSelectedBlockIds() const
{
    QVector<QString> selectedIds;
    for (auto& block : m_blocks) {
        if (block->isSelected()) {
            selectedIds.append(block->getId());
        }
    }
    return selectedIds;
}

// 工具方法
std::shared_ptr<Block> BgfxBlockRenderer::getBlockAt(const QPointF& position)
{
    QPointF worldPos = screenToWorld(position);

    // 从后往前遍历，优先选择上层积木
    for (int i = m_blocks.size() - 1; i >= 0; --i) {
        auto& block = m_blocks[i];
        QRectF blockRect(block->getPosition(), QSizeF(block->getBounds().width(), block->getBounds().height()));
        if (blockRect.contains(worldPos)) {
            return block;
        }
    }
    return nullptr;
}

ConnectionPoint* BgfxBlockRenderer::getConnectionPointAt(const QPointF& position)
{
    QPointF worldPos = screenToWorld(position);

    for (auto& block : m_blocks) {
        QPointF blockPos = block->getPosition();
        auto& connectionPoints = const_cast<QVector<ConnectionPoint>&>(block->getConnectionPoints());
        for (auto& point : connectionPoints) {
            QPointF pointPos = blockPos + point.position;
            QRectF pointRect(pointPos.x() - 8, pointPos.y() - 8, 16, 16);
            if (pointRect.contains(worldPos)) {
                return &point;
            }
        }
    }
    return nullptr;
}

// 视图控制
void BgfxBlockRenderer::setZoom(float zoom)
{
    m_zoom = qBound(0.1f, zoom, 5.0f);
    update();
}

void BgfxBlockRenderer::setPan(const QPointF& pan)
{
    m_pan = pan;
    update();
}

// 坐标变换
QPointF BgfxBlockRenderer::screenToWorld(const QPointF& screenPos) const
{
    // 简化的坐标变换，考虑缩放和平移
    QPointF worldPos = (screenPos - m_pan) / m_zoom;
    return worldPos;
}

QPointF BgfxBlockRenderer::worldToScreen(const QPointF& worldPos) const
{
    // 简化的坐标变换，考虑缩放和平移
    QPointF screenPos = worldPos * m_zoom + m_pan;
    return screenPos;
}

// 鼠标事件处理
void BgfxBlockRenderer::mousePressEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();
    m_lastMousePos = mousePos;

    if (event->button() == Qt::LeftButton) {
        // 检查是否点击在连接点上
        ConnectionPoint* clickedPoint = getConnectionPointAt(mousePos);
        if (clickedPoint) {
            // 开始连接
            m_isConnecting = true;
            // 找到包含这个连接点的积木
            for (auto& block : m_blocks) {
                for (const auto& point : block->getConnectionPoints()) {
                    if (&point == clickedPoint) {
                        m_connectionSourceBlockId = block->getId();
                        m_connectionSourcePointId = point.id;
                        m_connectionEndPos = screenToWorld(mousePos);
                        break;
                    }
                }
                if (m_isConnecting) break;
            }
            return;
        }

        // 检查是否点击在积木上
        auto clickedBlock = getBlockAt(mousePos);
        if (clickedBlock) {
            // 选择积木并开始拖拽
            selectBlock(clickedBlock->getId());
            m_isDragging = true;
            m_draggedBlockId = clickedBlock->getId();
        } else {
            // 点击空白区域，取消选择
            deselectAll();
        }
    } else if (event->button() == Qt::MiddleButton) {
        // 中键平移
        m_isPanning = true;
    }

    QWidget::mousePressEvent(event);
}

void BgfxBlockRenderer::mouseMoveEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();
    QPointF delta = mousePos - m_lastMousePos;

    if (m_isDragging && !m_draggedBlockId.isEmpty()) {
        // 拖拽积木
        auto draggedBlock = getBlock(m_draggedBlockId);
        if (draggedBlock) {
            QPointF worldDelta = delta / m_zoom;
            draggedBlock->setPosition(draggedBlock->getPosition() + worldDelta);
            update();
        }
    } else if (m_isConnecting) {
        // 更新连接线终点
        m_connectionEndPos = screenToWorld(mousePos);
        update();
    } else if (m_isPanning) {
        // 平移视图
        m_pan += delta;
        update();
    }

    m_lastMousePos = mousePos;
    QWidget::mouseMoveEvent(event);
}

void BgfxBlockRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isConnecting) {
            // 完成连接
            QPointF mousePos = event->position();
            ConnectionPoint* targetPoint = getConnectionPointAt(mousePos);

            if (targetPoint && !m_connectionSourceBlockId.isEmpty()) {
                // 找到目标积木
                for (auto& block : m_blocks) {
                    for (const auto& point : block->getConnectionPoints()) {
                        if (&point == targetPoint) {
                            // 创建连接
                            QString connectionId = QUuid::createUuid().toString(QUuid::WithoutBraces);
                            auto connection = std::make_shared<BlockConnection>(
                                connectionId,
                                m_connectionSourceBlockId,
                                m_connectionSourcePointId,
                                block->getId(),
                                point.id,
                                QColor(100, 100, 100)
                            );
                            addConnection(connection);

                            // 更新连接点状态
                            auto sourceBlock = getBlock(m_connectionSourceBlockId);
                            if (sourceBlock) {
                                auto sourcePoint = sourceBlock->getConnectionPoint(m_connectionSourcePointId);
                                if (sourcePoint) {
                                    sourcePoint->isConnected = true;
                                    sourcePoint->connectedBlockId = block->getId();
                                    sourcePoint->connectedPointId = point.id;
                                }
                            }

                            const_cast<ConnectionPoint*>(targetPoint)->isConnected = true;
                            const_cast<ConnectionPoint*>(targetPoint)->connectedBlockId = m_connectionSourceBlockId;
                            const_cast<ConnectionPoint*>(targetPoint)->connectedPointId = m_connectionSourcePointId;

                            break;
                        }
                    }
                }
            }

            m_isConnecting = false;
            m_connectionSourceBlockId.clear();
            m_connectionSourcePointId.clear();
        }

        m_isDragging = false;
        m_draggedBlockId.clear();
    } else if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }

    QWidget::mouseReleaseEvent(event);
}

void BgfxBlockRenderer::wheelEvent(QWheelEvent* event)
{
    // 缩放
    float scaleFactor = 1.0f + (event->angleDelta().y() / 1200.0f);
    setZoom(m_zoom * scaleFactor);

    QWidget::wheelEvent(event);
}
