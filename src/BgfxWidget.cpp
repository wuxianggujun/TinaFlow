#include "BgfxWidget.hpp"
#include <QDebug>
#include <QTimer>

BgfxWidget::BgfxWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(400, 300);

    // 确保窗口属性正确设置，让bgfx完全控制渲染
    setAttribute(Qt::WA_NoSystemBackground, true);  // 不绘制系统背景
    setAttribute(Qt::WA_OpaquePaintEvent, true);    // widget完全不透明，提高性能

    // 初始化矩阵
    bx::mtxIdentity(m_viewMatrix);
    bx::mtxIdentity(m_projMatrix);
    bx::mtxIdentity(m_transformMatrix);

    // 连接渲染定时器信号
    connect(&m_renderTimer, &QTimer::timeout, this, &BgfxWidget::onRenderTimer);

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
    qDebug() << "BgfxWidget::initializeBgfx - initializing";
    qDebug() << "Widget size:" << width() << "x" << height();
    qDebug() << "Real size:" << realWidth() << "x" << realHeight();

    void* currentWindowHandle = reinterpret_cast<void*>(winId());
    uint32_t currentWidth = static_cast<uint32_t>(realWidth());
    uint32_t currentHeight = static_cast<uint32_t>(realHeight());

    // 检查是否需要重新初始化
    bool needsReinitialization = false;
    if (m_viewId != UINT16_MAX) {
        // 检查窗口句柄是否变化
        if (BgfxManager::instance().getCurrentWindowHandle() != currentWindowHandle) {
            qDebug() << "BgfxWidget: Window handle changed from"
                     << BgfxManager::instance().getCurrentWindowHandle()
                     << "to" << currentWindowHandle;
            needsReinitialization = true;
        }
    }

    // 如果需要重新初始化，先清理资源
    if (needsReinitialization) {
        qDebug() << "BgfxWidget::initializeBgfx - reinitializing due to window handle change";
        shutdownBgfx();
    }

    // 使用BgfxManager初始化bgfx
    if (!BgfxManager::instance().initialize(currentWindowHandle, currentWidth, currentHeight)) {
        qCritical() << "BgfxWidget: Failed to initialize bgfx through BgfxManager";
        return;
    }

    // 获取视图ID（只在还没有时才获取新的）
    if (m_viewId == UINT16_MAX) {
        m_viewId = BgfxManager::instance().getNextViewId();
        if (m_viewId == UINT16_MAX) {
            qCritical() << "BgfxWidget: Failed to get view ID";
            return;
        }
        qDebug() << "BgfxWidget: Allocated new view ID:" << m_viewId;
    } else {
        qDebug() << "BgfxWidget: Reusing existing view ID:" << m_viewId;
    }

    // 设置视图的清屏状态
    bgfx::setViewClear(m_viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, m_clearColor, 1.0f, 0);

    // 更新矩阵
    updateMatrices();

    qDebug() << "BgfxWidget: Initialized with view ID:" << m_viewId;

    // 调用子类的初始化方法（只在重新初始化时调用）
    if (needsReinitialization || !m_resourcesInitialized) {
        if (needsReinitialization) {
            // 通知子类bgfx重新初始化
            onBgfxReset();
        }
        initializeResources();
        m_resourcesInitialized = true;
    }

    // 启动渲染定时器
    if (!m_renderTimer.isActive()) {
        m_renderTimer.start(16); // 60 FPS
        qDebug() << "BgfxWidget: Started render timer";
    }
}

void BgfxWidget::shutdownBgfx()
{
    if (m_viewId == UINT16_MAX) {
        return;
    }

    // 停止渲染定时器
    m_renderTimer.stop();

    // 调用子类的清理方法
    cleanupResources();
    m_resourcesInitialized = false;

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
    
    // 创建包含缩放和平移的变换矩阵
    float centerX = static_cast<float>(realWidth()) * 0.5f;
    float centerY = static_cast<float>(realHeight()) * 0.5f;

    // 先缩放，再平移到屏幕中心
    float scale[16], translate[16];
    bx::mtxScale(scale, m_zoom, m_zoom, 1.0f);
    bx::mtxTranslate(translate, centerX + static_cast<float>(m_pan.x()),
                     centerY + static_cast<float>(m_pan.y()), 0.0f);

    // 组合矩阵：translate * scale
    bx::mtxMul(m_transformMatrix, translate, scale);
    
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
    float centerX = static_cast<float>(realWidth()) * 0.5f;
    float centerY = static_cast<float>(realHeight()) * 0.5f;

    float wx = (screenPos.x() - (centerX + m_pan.x())) / m_zoom;
    float wy = (screenPos.y() - (centerY + m_pan.y())) / m_zoom;

    return QPointF(wx, wy);
}

QPointF BgfxWidget::worldToScreen(const QPointF& worldPos) const
{
    float centerX = static_cast<float>(realWidth()) * 0.5f;
    float centerY = static_cast<float>(realHeight()) * 0.5f;

    float sx = (worldPos.x() * m_zoom) + centerX + m_pan.x();
    float sy = (worldPos.y() * m_zoom) + centerY + m_pan.y();

    return QPointF(sx, sy);
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

    // 减少日志输出
    // qDebug() << "BgfxWidget::showEvent - widget becoming visible";

    initializeBgfx();

    // 确保渲染定时器在显示时启动
    if (m_viewId != UINT16_MAX) {
        // 重新设置视图清屏状态
        bgfx::setViewClear(m_viewId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, m_clearColor, 1.0f, 0);
        // 更新矩阵
        updateMatrices();

        if (!m_renderTimer.isActive()) {
            m_renderTimer.start(16); // 60 FPS
        }
        // 立即触发一次渲染
        update();
    }
}

void BgfxWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    static int paintCount = 0;
    paintCount++;

    if (m_viewId == UINT16_MAX) {
        // 静默跳过，避免频繁日志
        return;
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
    QPointF mousePos = event->position();

    // 1. 获取鼠标在世界坐标系中的位置（缩放前）
    QPointF worldPosBeforeZoom = screenToWorld(mousePos);

    // 2. 计算新的缩放级别
    float scaleFactor = 1.0f + (event->angleDelta().y() / 1200.0f);
    float newZoom = m_zoom * scaleFactor;
    newZoom = qBound(0.1f, newZoom, 5.0f); // 限制缩放范围

    // 3. 更新缩放级别
    m_zoom = newZoom;

    // 4. 计算新的平移量，以保持鼠标下的点在屏幕上的位置不变
    float centerX = static_cast<float>(realWidth()) * 0.5f;
    float centerY = static_cast<float>(realHeight()) * 0.5f;

    QPointF newPan;
    newPan.setX(mousePos.x() - centerX - (worldPosBeforeZoom.x() * m_zoom));
    newPan.setY(mousePos.y() - centerY - (worldPosBeforeZoom.y() * m_zoom));
    
    // 5. 应用新的平移量
    m_pan = newPan;

    // 6. 更新矩阵并重绘
    updateMatrices();
    update();

    QWidget::wheelEvent(event);
}
