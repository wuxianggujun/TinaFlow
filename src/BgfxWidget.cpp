#include "BgfxWidget.hpp"
#include <QDebug>
#include <QTimer>

BgfxWidget::BgfxWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 300);

    // 禁用Qt双缓冲，这对bgfx很重要
    setAttribute(Qt::WA_PaintOnScreen, true);

    // 确保窗口属性正确设置
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);

    // 初始化矩阵
    bx::mtxIdentity(m_viewMatrix);
    bx::mtxIdentity(m_projMatrix);
    bx::mtxIdentity(m_transformMatrix);

    qDebug() << "BgfxWidget: Initialized";
}

BgfxWidget::~BgfxWidget()
{
    m_renderTimer.stop();
    shutdownBgfx();
    qDebug() << "BgfxWidget: Destroyed";
}

void BgfxWidget::initializeBgfx()
{
    if (m_viewId != UINT16_MAX) {
        return; // 已经初始化
    }

    qDebug() << "BgfxWidget::initializeBgfx - initializing";
    qDebug() << "Widget size:" << width() << "x" << height();
    qDebug() << "Real size:" << realWidth() << "x" << realHeight();

    // 使用BgfxManager初始化bgfx（如果还没有初始化）
    if (!BgfxManager::instance().initialize(reinterpret_cast<void*>(winId()),
                                           static_cast<uint32_t>(realWidth()),
                                           static_cast<uint32_t>(realHeight()))) {
        qCritical() << "BgfxWidget: Failed to initialize bgfx through BgfxManager";
        return;
    }

    // 获取一个视图ID
    m_viewId = BgfxManager::instance().getNextViewId();
    if (m_viewId == UINT16_MAX) {
        qCritical() << "BgfxWidget: Failed to get view ID";
        return;
    }

    // 设置视图的清屏状态
    bgfx::setViewClear(m_viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, m_clearColor, 1.0f, 0);

    // 更新矩阵
    updateMatrices();

    qDebug() << "BgfxWidget: Initialized with view ID:" << m_viewId;

    // 调用子类的初始化方法
    initializeResources();

    // 启动渲染定时器
    connect(&m_renderTimer, &QTimer::timeout, this, &BgfxWidget::onRenderTimer);
    m_renderTimer.start(16); // 60 FPS
}

void BgfxWidget::shutdownBgfx()
{
    if (m_viewId == UINT16_MAX) {
        return;
    }

    // 调用子类的清理方法
    cleanupResources();

    // 释放视图ID
    BgfxManager::instance().releaseViewId(m_viewId);
    m_viewId = UINT16_MAX;

    qDebug() << "BgfxWidget: Shutdown complete";
}

void BgfxWidget::updateMatrices()
{
    if (m_viewId == UINT16_MAX) {
        return;
    }
    
    // 创建正交投影矩阵 (适合2D渲染)
    bx::mtxOrtho(m_projMatrix, 0.0f, static_cast<float>(realWidth()), 
                 static_cast<float>(realHeight()), 0.0f, -1.0f, 1.0f, 
                 0.0f, bgfx::getCaps()->homogeneousDepth);
    
    // 创建视图矩阵 (单位矩阵)
    bx::mtxIdentity(m_viewMatrix);
    
    // 创建变换矩阵 (包含缩放和平移)
    float centerX = static_cast<float>(realWidth()) * 0.5f;
    float centerY = static_cast<float>(realHeight()) * 0.5f;
    
    // 组合变换：先缩放，再平移到中心，最后应用用户平移
    float scale[16], translate[16], userTranslate[16];
    bx::mtxScale(scale, m_zoom, m_zoom, 1.0f);
    bx::mtxTranslate(translate, centerX, centerY, 0.0f);
    bx::mtxTranslate(userTranslate, static_cast<float>(m_pan.x()), static_cast<float>(m_pan.y()), 0.0f);
    
    // 组合矩阵：userTranslate * translate * scale
    float temp[16];
    bx::mtxMul(temp, scale, translate);
    bx::mtxMul(m_transformMatrix, temp, userTranslate);
    
    // 设置视图变换
    bgfx::setViewTransform(m_viewId, m_viewMatrix, m_projMatrix);
}

// 视图控制
void BgfxWidget::setZoom(float zoom)
{
    m_zoom = qBound(0.1f, zoom, 5.0f);
    updateMatrices();
    update();
}

void BgfxWidget::setPan(const QPointF& pan)
{
    m_pan = pan;
    updateMatrices();
    update();
}

// 坐标变换
QPointF BgfxWidget::screenToWorld(const QPointF& screenPos) const
{
    QPointF worldPos = (screenPos - m_pan) / m_zoom;
    return worldPos;
}

QPointF BgfxWidget::worldToScreen(const QPointF& worldPos) const
{
    QPointF screenPos = worldPos * m_zoom + m_pan;
    return screenPos;
}

// 获取变换矩阵
void BgfxWidget::getViewMatrix(float* viewMatrix) const
{
    memcpy(viewMatrix, m_viewMatrix, sizeof(float) * 16);
}

void BgfxWidget::getProjectionMatrix(float* projMatrix) const
{
    memcpy(projMatrix, m_projMatrix, sizeof(float) * 16);
}

void BgfxWidget::getTransformMatrix(float* transformMatrix) const
{
    memcpy(transformMatrix, m_transformMatrix, sizeof(float) * 16);
}

// Qt事件处理
void BgfxWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    qDebug() << "BgfxWidget::showEvent - widget becoming visible";
    qDebug() << "Current view ID:" << m_viewId << "Timer active:" << m_renderTimer.isActive();
    qDebug() << "Window ID:" << winId() << "Widget visible:" << isVisible();

    initializeBgfx();

    // 确保渲染定时器在显示时启动
    if (m_viewId != UINT16_MAX) {
        // 重新设置视图清屏状态
        bgfx::setViewClear(m_viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, m_clearColor, 1.0f, 0);
        qDebug() << "BgfxWidget: Reset view clear state for view" << m_viewId;

        // 更新矩阵
        updateMatrices();

        if (!m_renderTimer.isActive()) {
            m_renderTimer.start(16); // 60 FPS
            qDebug() << "BgfxWidget: Started render timer";
        }
        // 立即触发一次渲染
        update();
        qDebug() << "BgfxWidget: Triggered immediate update";
    }
}

void BgfxWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    static int paintCount = 0;
    paintCount++;

    if (m_viewId == UINT16_MAX) {
        qDebug() << "BgfxWidget::paintEvent - invalid view ID, skipping render";
        return;
    }

    if (paintCount % 60 == 1) { // 每秒打印一次（60 FPS）
        qDebug() << "BgfxWidget::paintEvent - rendering frame" << paintCount << "view ID:" << m_viewId;
        qDebug() << "Widget size:" << width() << "x" << height() << "Real size:" << realWidth() << "x" << realHeight();
    }

    // 设置视图矩形
    bgfx::setViewRect(m_viewId, 0, 0, static_cast<uint16_t>(realWidth()), static_cast<uint16_t>(realHeight()));

    // 重新设置视图变换（确保矩阵是最新的）
    float viewMatrix[16], projMatrix[16];
    getViewMatrix(viewMatrix);
    getProjectionMatrix(projMatrix);
    bgfx::setViewTransform(m_viewId, viewMatrix, projMatrix);

    // 确保视图被清除（这个调用很重要）
    bgfx::touch(m_viewId);

    // 调用子类的渲染方法
    render();

    // 提交帧
    bgfx::frame();
}

void BgfxWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    if (m_viewId != UINT16_MAX) {
        BgfxManager::instance().reset(static_cast<uint32_t>(realWidth()), static_cast<uint32_t>(realHeight()));
        updateMatrices();
    }

    update();
}

void BgfxWidget::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    // 停止渲染定时器以节省资源
    m_renderTimer.stop();
    qDebug() << "BgfxWidget: Hidden, stopped render timer";
}

void BgfxWidget::onRenderTimer()
{
    if (isVisible() && m_viewId != UINT16_MAX) {
        update(); // 触发paintEvent
    }
}

// 鼠标事件处理
void BgfxWidget::mousePressEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();
    m_lastMousePos = mousePos;

    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
    }

    QWidget::mousePressEvent(event);
}

void BgfxWidget::mouseMoveEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();
    QPointF delta = mousePos - m_lastMousePos;

    if (m_isPanning) {
        m_pan += delta;
        updateMatrices();
        update();
    }

    m_lastMousePos = mousePos;
    QWidget::mouseMoveEvent(event);
}

void BgfxWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }

    QWidget::mouseReleaseEvent(event);
}

void BgfxWidget::wheelEvent(QWheelEvent* event)
{
    float scaleFactor = 1.0f + (event->angleDelta().y() / 1200.0f);
    setZoom(m_zoom * scaleFactor);

    QWidget::wheelEvent(event);
}
