#include "SkiaRenderer.hpp"
#include "Block.hpp"
#include <QtMath>
#include <QMouseEvent>
#include "include/core/SkPaint.h"
#include "include/core/SkFont.h"
#include "include/core/SkPath.h"
#include "include/core/SkRRect.h"
#include "include/effects/SkGradientShader.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/GrBackendSurface.h"

SkiaRenderer::SkiaRenderer(QWidget* parent)
    : QOpenGLWidget(parent), skContext(nullptr), skSurface(nullptr), m_animationTime(0.0f)
{
    setMinimumSize(800, 600);

    // 设置动画定时器
    connect(&m_animationTimer, &QTimer::timeout, this, &SkiaRenderer::onTick);
}

SkiaRenderer::~SkiaRenderer()
{
    makeCurrent();
    skSurface.reset();
    skContext.reset();
    doneCurrent();
}

void SkiaRenderer::initializeGL()
{
    initializeOpenGLFunctions();

    // 创建Skia GL接口
    sk_sp<const GrGLInterface> interface(GrGLMakeNativeInterface());
    if (!interface) {
        qCritical() << "SkiaRenderer: Failed to create GL interface";
        return;
    }

    // 创建Skia上下文
    skContext = GrDirectContexts::MakeGL(interface);
    if (!skContext) {
        qCritical() << "SkiaRenderer: Failed to create Skia context";
        return;
    }

    qDebug() << "SkiaRenderer: Skia context created successfully";

    // 创建Skia表面
    createSkiaSurface();
}

void SkiaRenderer::resizeGL(int w, int h)
{
    createSkiaSurface();
}

void SkiaRenderer::paintGL()
{
    if (!skSurface || !skContext) {
        qWarning() << "SkiaRenderer: Skia surface or context not available";
        return;
    }

    SkCanvas* canvas = skSurface->getCanvas();
    if (!canvas) {
        qWarning() << "SkiaRenderer: Canvas not available";
        return;
    }

    // 获取设备像素比例和尺寸
    qreal dpr = devicePixelRatioF();
    QSize logicalSize = size();

    // 保存画布状态
    canvas->save();

    // 清除画布为白色
    canvas->clear(SK_ColorWHITE);

    // 设置正确的坐标变换
    // 1. 缩放到设备像素比例
    canvas->scale(dpr, dpr);

    // 2. 翻转Y轴，因为OpenGL使用BottomLeft原点，而我们想要TopLeft
    canvas->translate(0, logicalSize.height());
    canvas->scale(1, -1);

    // 测试绘制一个简单的矩形，确保Skia工作正常
    SkPaint testPaint;
    testPaint.setColor(SK_ColorRED);
    canvas->drawRect(SkRect::MakeXYWH(50, 50, 100, 100), testPaint);

    testPaint.setColor(SK_ColorBLUE);
    canvas->drawCircle(200, 100, 50, testPaint);

    // 绘制积木编程内容
    drawBlockProgrammingContent(canvas);

    // 恢复画布状态
    canvas->restore();

    // 提交到GPU
    if (skContext) {
        skContext->flushAndSubmit();
    }

    // Qt will swap buffers automatically
}

void SkiaRenderer::resizeEvent(QResizeEvent* event)
{
    QOpenGLWidget::resizeEvent(event);
    // Skia surface is recreated in resizeGL
}

void SkiaRenderer::showEvent(QShowEvent* e)
{
    QOpenGLWidget::showEvent(e);

    qDebug() << "SkiaRenderer: showEvent called - NOT starting animation timer";

    // 完全禁用动画定时器
    // 在第一次显示时创建示例积木
    static bool samplesCreated = false;
    if (!samplesCreated) {
        qDebug() << "SkiaRenderer: Creating sample blocks...";
        createSampleBlocks();
        samplesCreated = true;
        qDebug() << "SkiaRenderer: Sample blocks created, triggering update";
        update(); // 手动触发一次更新
    }
}

void SkiaRenderer::hideEvent(QHideEvent* e)
{
    QOpenGLWidget::hideEvent(e);
    m_animationTimer.stop();
    qDebug() << "SkiaRenderer: Animation timer stopped";
}

void SkiaRenderer::onTick()
{
    qDebug() << "SkiaRenderer: onTick called - this should NOT happen!";
    // 完全禁用动画更新
    return;
}

void SkiaRenderer::createSkiaSurface()
{
    if (!skContext) {
        qWarning() << "SkiaRenderer: No Skia context available";
        return;
    }

    // 安全地刷新上下文
    try {
        skContext->flushAndSubmit();
    } catch (...) {
        qWarning() << "SkiaRenderer: Error flushing context during surface creation";
    }

    skSurface.reset();

    // 确保OpenGL状态正确
    makeCurrent();

    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = defaultFramebufferObject();
    fbInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;

    // 获取实际的framebuffer尺寸
    // 在高DPI显示器上，Qt会自动处理缩放，我们需要获取实际的像素尺寸
    QSize actualSize = size() * devicePixelRatioF();
    int w = qMax(1, actualSize.width());
    int h = qMax(1, actualSize.height());

    qDebug() << "SkiaRenderer: Creating surface - Widget size:" << size()
             << "Actual size:" << actualSize << "DPR:" << devicePixelRatioF();

    GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(
        w, h,  // 使用实际像素尺寸
        this->format().samples(),  // sample count
        this->format().stencilBufferSize(),  // stencil bits
        fbInfo
    );

    skSurface = SkSurfaces::WrapBackendRenderTarget(
        skContext.get(),
        backendRenderTarget,
        kBottomLeft_GrSurfaceOrigin,  // 回到BottomLeft，这是OpenGL的标准
        colorType,
        nullptr,
        nullptr
    );

    if (!skSurface) {
        qCritical() << "SkiaRenderer: Failed to create Skia surface, size:" << w << "x" << h;
        qCritical() << "SkiaRenderer: FBO ID:" << fbInfo.fFBOID << "Format:" << fbInfo.fFormat;
        qCritical() << "SkiaRenderer: Samples:" << this->format().samples() << "Stencil:" << this->format().stencilBufferSize();
    } else {
        qDebug() << "SkiaRenderer: Skia surface created successfully";
    }
}

void SkiaRenderer::drawBlockProgrammingContent(SkCanvas* canvas)
{
    if (!canvas) {
        qWarning() << "SkiaRenderer: Canvas is null";
        return;
    }

    // 暂时不绘制网格，先测试积木是否正常
    // drawGrid(canvas);

    // 绘制示例积木形状（使用与测试矩形相同的简单方法）
    SkPaint blockPaint;
    blockPaint.setAntiAlias(true);

    // 绘制开始积木 - 使用简单的drawRect，不用drawRoundRect
    blockPaint.setColor(SkColorSetARGB(255, 76, 175, 80)); // 绿色
    canvas->drawRect(SkRect::MakeXYWH(300, 200, 120, 40), blockPaint);

    // 绘制变量积木
    blockPaint.setColor(SkColorSetARGB(255, 156, 39, 176)); // 紫色
    canvas->drawRect(SkRect::MakeXYWH(450, 200, 120, 40), blockPaint);

    // 绘制条件积木
    blockPaint.setColor(SkColorSetARGB(255, 255, 152, 0)); // 橙色
    canvas->drawRect(SkRect::MakeXYWH(300, 280, 120, 40), blockPaint);

    // 绘制简单的文本标签
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SkColorSetARGB(255, 0, 0, 0)); // 黑色文本

    SkFont font;
    font.setSize(12);

    canvas->drawString("Start", 320, 225, font, textPaint);
    canvas->drawString("Var", 480, 225, font, textPaint);
    canvas->drawString("If", 330, 305, font, textPaint);

    // 绘制标题
    textPaint.setColor(SkColorSetARGB(255, 50, 50, 50)); // 深灰色
    font.setSize(18);
    canvas->drawString("Block Programming - Skia", 50, 50, font, textPaint);

    // 暂时不绘制连接点，先确保积木本身正常
    /*
    // 绘制连接点示例
    SkPaint pointPaint;
    pointPaint.setAntiAlias(true);
    pointPaint.setColor(SkColorSetARGB(255, 100, 100, 100)); // 灰色

    // 开始积木的输出连接点
    canvas->drawCircle(420, 220, 6, pointPaint);
    // 变量积木的输出连接点
    canvas->drawRect(SkRect::MakeXYWH(564, 214, 12, 12), pointPaint);
    // 条件积木的输入连接点
    canvas->drawCircle(300, 300, 6, pointPaint);
    */

    // 绘制状态信息（使用固定位置）
    font.setSize(14);
    textPaint.setColor(SkColorSetARGB(255, 100, 100, 100)); // 灰色
    QString statusText = QString("Block Demo | Skia Renderer | Status: OK");
    canvas->drawString(statusText.toUtf8().constData(), 50, 400, font, textPaint);  // 使用固定Y坐标
}

// 积木管理方法
void SkiaRenderer::addBlock(std::shared_ptr<Block> block)
{
    if (block) {
        m_blocks.append(block);
        update();
    }
}

void SkiaRenderer::removeBlock(const QString& blockId)
{
    for (int i = 0; i < m_blocks.size(); ++i) {
        if (m_blocks[i]->getId() == blockId) {
            m_blocks.removeAt(i);
            update();
            break;
        }
    }
}

std::shared_ptr<Block> SkiaRenderer::getBlock(const QString& blockId)
{
    for (auto& block : m_blocks) {
        if (block->getId() == blockId) {
            return block;
        }
    }
    return nullptr;
}

void SkiaRenderer::addConnection(std::shared_ptr<BlockConnection> connection)
{
    if (connection) {
        m_connections.append(connection);
        update();
    }
}

void SkiaRenderer::removeConnection(const QString& connectionId)
{
    for (int i = 0; i < m_connections.size(); ++i) {
        if (m_connections[i]->id == connectionId) {
            m_connections.removeAt(i);
            update();
            break;
        }
    }
}

std::shared_ptr<BlockConnection> SkiaRenderer::getConnection(const QString& connectionId)
{
    for (auto& connection : m_connections) {
        if (connection->id == connectionId) {
            return connection;
        }
    }
    return nullptr;
}

void SkiaRenderer::selectBlock(const QString& blockId)
{
    for (auto& block : m_blocks) {
        block->setSelected(block->getId() == blockId);
    }
    update();
}

void SkiaRenderer::deselectAll()
{
    for (auto& block : m_blocks) {
        block->setSelected(false);
    }
    update();
}

QVector<QString> SkiaRenderer::getSelectedBlockIds() const
{
    QVector<QString> selectedIds;
    for (auto& block : m_blocks) {
        if (block->isSelected()) {
            selectedIds.append(block->getId());
        }
    }
    return selectedIds;
}

std::shared_ptr<Block> SkiaRenderer::getBlockAt(const QPointF& position)
{
    // 从后往前遍历，优先选择上层积木
    for (int i = m_blocks.size() - 1; i >= 0; --i) {
        auto& block = m_blocks[i];
        QRectF blockRect(block->getPosition(), QSizeF(block->getBounds().width(), block->getBounds().height()));
        if (blockRect.contains(position)) {
            return block;
        }
    }
    return nullptr;
}

ConnectionPoint* SkiaRenderer::getConnectionPointAt(const QPointF& position)
{
    for (auto& block : m_blocks) {
        QPointF blockPos = block->getPosition();
        auto& connectionPoints = const_cast<QVector<ConnectionPoint>&>(block->getConnectionPoints());
        for (auto& point : connectionPoints) {
            QPointF pointPos = blockPos + point.position;
            QRectF pointRect(pointPos.x() - 8, pointPos.y() - 8, 16, 16);
            if (pointRect.contains(position)) {
                return &point;
            }
        }
    }
    return nullptr;
}

// 绘制方法实现
void SkiaRenderer::drawGrid(SkCanvas* canvas)
{
    if (!canvas) return;

    SkPaint gridPaint;
    gridPaint.setColor(SkColorSetARGB(30, 150, 150, 150));  // 淡灰色网格线
    gridPaint.setStrokeWidth(1);
    gridPaint.setStyle(SkPaint::kStroke_Style);

    int gridSize = 20;
    int w = width();
    int h = height();

    // 绘制垂直网格线
    for (int x = 0; x < w; x += gridSize) {
        canvas->drawLine(x, 0, x, h, gridPaint);
    }

    // 绘制水平网格线
    for (int y = 0; y < h; y += gridSize) {
        canvas->drawLine(0, y, w, y, gridPaint);
    }
}

void SkiaRenderer::drawBlock(SkCanvas* canvas, std::shared_ptr<Block> block)
{
    if (!block || !canvas) return;

    try {
        QPointF pos = block->getPosition();
        QRectF bounds = block->getBounds();
        QColor color = block->getColor();

        qDebug() << "SkiaRenderer: Drawing block at" << pos << "size" << bounds.size() << "color" << color;

        // 绘制简单的矩形积木
        SkPaint blockPaint;
        blockPaint.setAntiAlias(true);
        blockPaint.setColor(SkColorSetARGB(255, color.red(), color.green(), color.blue()));

        SkRect blockRect = SkRect::MakeXYWH(pos.x(), pos.y(), bounds.width(), bounds.height());
        canvas->drawRoundRect(blockRect, 8, 8, blockPaint);

        // 绘制简单的标题
        SkPaint textPaint;
        textPaint.setAntiAlias(true);
        textPaint.setColor(SK_ColorWHITE);

        SkFont font;
        font.setSize(12);

        QString title = block->getTitle();
        float textX = pos.x() + 10;
        float textY = pos.y() + 25;

        canvas->drawString(title.toUtf8().constData(), textX, textY, font, textPaint);

        qDebug() << "SkiaRenderer: Block drawn successfully";

    } catch (const std::exception& e) {
        qCritical() << "SkiaRenderer: Exception in drawBlock:" << e.what();
    } catch (...) {
        qCritical() << "SkiaRenderer: Unknown exception in drawBlock";
    }
}

void SkiaRenderer::drawConnectionPoint(SkCanvas* canvas, const ConnectionPoint& point, const QPointF& blockPos)
{
    QPointF pointPos = blockPos + point.position;

    SkPaint pointPaint;
    pointPaint.setAntiAlias(true);
    pointPaint.setColor(SkColorSetARGB(255, point.color.red(), point.color.green(), point.color.blue()));

    // 根据连接点类型绘制不同形状
    float radius = 6;
    switch (point.type) {
        case ConnectionType::Input:
            // 输入连接点 - 圆形
            canvas->drawCircle(pointPos.x(), pointPos.y(), radius, pointPaint);
            break;
        case ConnectionType::Output:
            // 输出连接点 - 方形
            canvas->drawRect(SkRect::MakeXYWH(pointPos.x() - radius, pointPos.y() - radius,
                                            radius * 2, radius * 2), pointPaint);
            break;
        case ConnectionType::Next:
        case ConnectionType::Previous:
            // 控制流连接点 - 菱形
            SkPath diamondPath;
            diamondPath.moveTo(pointPos.x(), pointPos.y() - radius);
            diamondPath.lineTo(pointPos.x() + radius, pointPos.y());
            diamondPath.lineTo(pointPos.x(), pointPos.y() + radius);
            diamondPath.lineTo(pointPos.x() - radius, pointPos.y());
            diamondPath.close();
            canvas->drawPath(diamondPath, pointPaint);
            break;
    }

    // 绘制连接点边框
    SkPaint borderPaint;
    borderPaint.setAntiAlias(true);
    borderPaint.setStyle(SkPaint::kStroke_Style);
    borderPaint.setStrokeWidth(1);
    borderPaint.setColor(point.isConnected ? SK_ColorGREEN : SK_ColorBLACK);

    switch (point.type) {
        case ConnectionType::Input:
            canvas->drawCircle(pointPos.x(), pointPos.y(), radius, borderPaint);
            break;
        case ConnectionType::Output:
            canvas->drawRect(SkRect::MakeXYWH(pointPos.x() - radius, pointPos.y() - radius,
                                            radius * 2, radius * 2), borderPaint);
            break;
        case ConnectionType::Next:
        case ConnectionType::Previous:
            SkPath diamondPath;
            diamondPath.moveTo(pointPos.x(), pointPos.y() - radius);
            diamondPath.lineTo(pointPos.x() + radius, pointPos.y());
            diamondPath.lineTo(pointPos.x(), pointPos.y() + radius);
            diamondPath.lineTo(pointPos.x() - radius, pointPos.y());
            diamondPath.close();
            canvas->drawPath(diamondPath, borderPaint);
            break;
    }
}

void SkiaRenderer::drawConnection(SkCanvas* canvas, std::shared_ptr<BlockConnection> connection)
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

    SkPaint connectionPaint;
    connectionPaint.setAntiAlias(true);
    connectionPaint.setColor(SkColorSetARGB(255, connection->color.red(),
                                          connection->color.green(), connection->color.blue()));
    connectionPaint.setStrokeWidth(3);
    connectionPaint.setStyle(SkPaint::kStroke_Style);

    // 绘制贝塞尔曲线连接
    SkPath connectionPath;
    connectionPath.moveTo(startPos.x(), startPos.y());

    // 计算控制点
    float dx = endPos.x() - startPos.x();
    float controlOffset = qAbs(dx) * 0.5f + 50;

    QPointF control1(startPos.x() + controlOffset, startPos.y());
    QPointF control2(endPos.x() - controlOffset, endPos.y());

    connectionPath.cubicTo(control1.x(), control1.y(),
                          control2.x(), control2.y(),
                          endPos.x(), endPos.y());

    canvas->drawPath(connectionPath, connectionPaint);

    // 绘制箭头
    float arrowSize = 8;
    float angle = atan2(endPos.y() - control2.y(), endPos.x() - control2.x());

    SkPath arrowPath;
    arrowPath.moveTo(endPos.x(), endPos.y());
    arrowPath.lineTo(endPos.x() - arrowSize * cos(angle - M_PI/6),
                    endPos.y() - arrowSize * sin(angle - M_PI/6));
    arrowPath.lineTo(endPos.x() - arrowSize * cos(angle + M_PI/6),
                    endPos.y() - arrowSize * sin(angle + M_PI/6));
    arrowPath.close();

    SkPaint arrowPaint;
    arrowPaint.setAntiAlias(true);
    arrowPaint.setColor(connectionPaint.getColor());
    canvas->drawPath(arrowPath, arrowPaint);
}

void SkiaRenderer::drawSelectionBox(SkCanvas* canvas)
{
    // 这里可以添加选择框的绘制逻辑
    // 暂时留空，后续可以扩展
}

// 鼠标事件处理
void SkiaRenderer::mousePressEvent(QMouseEvent* event)
{
    QPointF mousePos(event->position().x(), event->position().y());
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
                        m_connectionEndPos = mousePos;
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
    }

    QOpenGLWidget::mousePressEvent(event);
}

void SkiaRenderer::mouseMoveEvent(QMouseEvent* event)
{
    QPointF mousePos(event->position().x(), event->position().y());
    QPointF delta = mousePos - m_lastMousePos;

    if (m_isDragging && !m_draggedBlockId.isEmpty()) {
        // 拖拽积木
        auto draggedBlock = getBlock(m_draggedBlockId);
        if (draggedBlock) {
            draggedBlock->setPosition(draggedBlock->getPosition() + delta);
            update();
        }
    } else if (m_isConnecting) {
        // 更新连接线终点
        m_connectionEndPos = mousePos;
        update();
    }

    m_lastMousePos = mousePos;
    QOpenGLWidget::mouseMoveEvent(event);
}

void SkiaRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_isConnecting) {
            // 完成连接
            QPointF mousePos(event->position().x(), event->position().y());
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

                            targetPoint->isConnected = true;
                            targetPoint->connectedBlockId = m_connectionSourceBlockId;
                            targetPoint->connectedPointId = m_connectionSourcePointId;

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
    }

    QOpenGLWidget::mouseReleaseEvent(event);
}

void SkiaRenderer::createSampleBlocks()
{
    qDebug() << "SkiaRenderer: createSampleBlocks called - COMPLETELY DISABLED for debugging";
    // 完全禁用积木创建，只测试基本渲染
    return;
}
