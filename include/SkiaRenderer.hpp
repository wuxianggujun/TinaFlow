#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>

// Skia Ganesh-GL 新 API (m132+)
#include "include/core/SkBitmap.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/GrContextOptions.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPath.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"

/**
 * @brief Skia OpenGL渲染器，基于您提供的AI示例代码
 */
class SkiaRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit SkiaRenderer(QWidget* parent = nullptr);
    ~SkiaRenderer() override;

protected:
    // Qt-OpenGL lifecycle
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    // 窗口状态管理
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void updateAnimation();

private:
    // helper methods
    SkPath makePuzzlePath(const QRectF& r, qreal notchW = 8, qreal notchH = 4);
    void drawBlockProgrammingContent(SkCanvas* canvas);
    void cleanupGPUResources(); // 安全清理GPU资源
    void rebuildSurface(int w, int h, GLuint fbo); // 重建Surface的辅助方法

    // Skia objects - 使用新的API
    sk_sp<GrDirectContext> fContext;
    sk_sp<SkSurface> fSurface;

    // FBO缓存 - 解决Intel Xe等显卡每帧换FBO的问题
    GLuint m_cachedFbo;

    // Animation
    QTimer* m_animationTimer;
    float m_animationTime;
};
