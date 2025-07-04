#include "BgfxBlockRenderer.hpp"
#include <QDebug>
#include <QTimer>

BgfxBlockRenderer::BgfxBlockRenderer(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(800, 600);
    
    // 禁用Qt双缓冲，这对bgfx很重要
    setAttribute(Qt::WA_PaintOnScreen, true);
    
    qDebug() << "BgfxBlockRenderer: Initialized";
}

BgfxBlockRenderer::~BgfxBlockRenderer()
{
    m_renderTimer.stop();
    shutdownBgfx();
    qDebug() << "BgfxBlockRenderer: Destroyed";
}

void BgfxBlockRenderer::shutdownBgfx()
{
    if (!m_bgfxInitialized) {
        return;
    }
    
    bgfx::shutdown();
    m_bgfxInitialized = false;
    
    qDebug() << "bgfx shutdown complete";
}

void BgfxBlockRenderer::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    
    if (m_bgfxInitialized) {
        return;
    }
    
    qDebug() << "BgfxBlockRenderer::showEvent - initializing bgfx";
    qDebug() << "Widget size:" << width() << "x" << height();
    qDebug() << "Real size:" << realWidth() << "x" << realHeight();
    
    // 初始化bgfx
    bgfx::Init init;
    init.type = bgfx::RendererType::Count; // 自动选择最佳渲染器
    init.vendorId = BGFX_PCI_ID_NONE;
    init.resolution.width = static_cast<uint32_t>(realWidth());
    init.resolution.height = static_cast<uint32_t>(realHeight());
    init.resolution.reset = BGFX_RESET_NONE;
    init.platformData.nwh = reinterpret_cast<void*>(winId());
    
    if (!bgfx::init(init)) {
        qCritical() << "Failed to initialize bgfx";
        return;
    }
    
    // 启用调试文本
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    
    // 设置视图0的清屏状态
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x336699ff, 1.0f, 0);
    
    m_bgfxInitialized = true;
    
    qDebug() << "bgfx initialized successfully!";
    qDebug() << "Renderer:" << bgfx::getRendererName(bgfx::getRendererType());
    
    // 启动渲染定时器
    connect(&m_renderTimer, &QTimer::timeout, this, &BgfxBlockRenderer::onRenderTimer);
    m_renderTimer.start(16); // 60 FPS
}

void BgfxBlockRenderer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    if (!m_bgfxInitialized) {
        return;
    }
    
    // 设置视图矩形
    bgfx::setViewRect(0, 0, 0, static_cast<uint16_t>(realWidth()), static_cast<uint16_t>(realHeight()));
    
    // 这个虚拟绘制调用确保视图0被清除
    bgfx::touch(0);
    
    
    // 提交帧
    bgfx::frame();
}

void BgfxBlockRenderer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    if (m_bgfxInitialized && bgfx::getInternalData()->context != nullptr) {
        bgfx::reset(static_cast<uint32_t>(realWidth()), static_cast<uint32_t>(realHeight()), BGFX_RESET_NONE);
        qDebug() << "bgfx reset to resolution:" << realWidth() << "x" << realHeight();
    }
    
    update();
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
    QPointF worldPos = (screenPos - m_pan) / m_zoom;
    return worldPos;
}

QPointF BgfxBlockRenderer::worldToScreen(const QPointF& worldPos) const
{
    QPointF screenPos = worldPos * m_zoom + m_pan;
    return screenPos;
}

// 鼠标事件处理
void BgfxBlockRenderer::mousePressEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();
    m_lastMousePos = mousePos;

    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
    }

    QWidget::mousePressEvent(event);
}

void BgfxBlockRenderer::mouseMoveEvent(QMouseEvent* event)
{
    QPointF mousePos = event->position();
    QPointF delta = mousePos - m_lastMousePos;

    if (m_isPanning) {
        m_pan += delta;
        update();
    }

    m_lastMousePos = mousePos;
    QWidget::mouseMoveEvent(event);
}

void BgfxBlockRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
    }

    QWidget::mouseReleaseEvent(event);
}

void BgfxBlockRenderer::wheelEvent(QWheelEvent* event)
{
    float scaleFactor = 1.0f + (event->angleDelta().y() / 1200.0f);
    setZoom(m_zoom * scaleFactor);
    
    QWidget::wheelEvent(event);
}
