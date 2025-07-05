#pragma once

#include <QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPointF>
#include <QColor>
#include <memory>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <bx/allocator.h>

/**
 * @brief 基于bgfx的渲染器
 *
 * 使用bgfx跨平台渲染库实现高性能的GPU加速渲染。
 * 专注于基本的bgfx功能：清屏、视图管理、简单几何体渲染。
 */
class BgfxBlockRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit BgfxBlockRenderer(QWidget* parent = nullptr);
    ~BgfxBlockRenderer() override;

    // 视图控制
    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }
    void setPan(const QPointF& pan);
    QPointF getPan() const { return m_pan; }

protected:
    // 禁用Qt的paintEngine，避免警告
    QPaintEngine* paintEngine() const override { return nullptr; }

    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    // 鼠标事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onRenderTimer();

private:
    // DPI支持
    qreal realWidth() const { return width() * devicePixelRatio(); }
    qreal realHeight() const { return height() * devicePixelRatio(); }
    
    void shutdownBgfx();

    // 渲染方法
    void renderDebugInfo();
    void renderTestGeometry();

    // 坐标变换
    QPointF screenToWorld(const QPointF& screenPos) const;
    QPointF worldToScreen(const QPointF& worldPos) const;
    
    // bgfx资源
    bgfx::ViewId m_viewId = 0;
    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    bgfx::VertexBufferHandle m_vertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_indexBuffer = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout m_vertexLayout;

    // 视图状态
    float m_zoom = 1.0f;
    QPointF m_pan = QPointF(0, 0);
    float m_viewMatrix[16];
    float m_projMatrix[16];

    // 交互状态
    bool m_isPanning = false;
    QPointF m_lastMousePos;

    // 渲染定时器
    QTimer m_renderTimer;
    bool m_bgfxInitialized = false;

    // 窗口句柄
    void* m_nativeHandle = nullptr;
};
