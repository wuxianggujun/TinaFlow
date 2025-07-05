#pragma once

#include <QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPointF>
#include <QColor>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <bx/allocator.h>
#include "BgfxManager.hpp"

/**
 * @brief 基础的bgfx渲染Widget
 *
 * 这个基类处理所有的bgfx初始化、窗口管理、视图变换等通用功能。
 * 子类只需要重写render()方法来实现具体的绘制逻辑。
 */
class BgfxWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BgfxWidget(QWidget* parent = nullptr);
    ~BgfxWidget() override;

    // 视图控制
    virtual void setZoom(float zoom);
    float getZoom() const { return m_zoom; }
    void setPan(const QPointF& pan);
    QPointF getPan() const { return m_pan; }
    
    // 坐标变换
    QPointF screenToWorld(const QPointF& screenPos) const;
    QPointF worldToScreen(const QPointF& worldPos) const;
    
    // 获取变换矩阵
    void getViewMatrix(float* viewMatrix) const;
    void getProjectionMatrix(float* projMatrix) const;
    void getTransformMatrix(float* transformMatrix) const;

protected:
    // 禁用Qt的paintEngine，避免警告
    QPaintEngine* paintEngine() const override { return nullptr; }

    // 确保Qt不会干扰bgfx渲染
    bool hasHeightForWidth() const override { return false; }

    // Qt事件处理
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

    // 鼠标事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    // 子类需要重写的渲染方法
    virtual void render() = 0;
    
    // 可选的初始化和清理方法
    virtual void initializeResources() {}
    virtual void cleanupResources() {}

    // 当bgfx重新初始化时调用（可选重写）
    virtual void onBgfxReset() {}
    
    // 获取bgfx相关信息
    bool isBgfxInitialized() const { return BgfxManager::instance().isInitialized() && m_viewId != UINT16_MAX; }
    bgfx::ViewId getViewId() const { return m_viewId; }

    // 获取窗口尺寸（受保护的方法供子类使用）
    qreal realWidth() const { return width() * devicePixelRatio(); }
    qreal realHeight() const { return height() * devicePixelRatio(); }
    


private slots:
    void onRenderTimer();

private:
    void initializeBgfx();
    void shutdownBgfx();
    void updateMatrices();

    // bgfx状态
    bgfx::ViewId m_viewId = UINT16_MAX;
    bool m_resourcesInitialized = false;

    // 视图状态
    float m_zoom = 1.0f;
    QPointF m_pan = QPointF(0, 0);
    float m_viewMatrix[16];
    float m_projMatrix[16];
    float m_transformMatrix[16];
    
    // 交互状态
    bool m_isPanning = false;
    QPointF m_lastMousePos;
    
    // 渲染定时器
    QTimer m_renderTimer;
    
    // 背景颜色 (RGBA格式)
    uint32_t m_clearColor = 0xffffffff; // 白色背景
};
