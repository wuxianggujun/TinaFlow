#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QSurfaceFormat>
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"

// 前向声明
class SkCanvas;

/**
 * @brief 基于GitHub示例的简化Skia渲染器
 */
class SkiaRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit SkiaRenderer(QWidget* parent = nullptr);
    ~SkiaRenderer() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private slots:
    void onTick();

private:
    void resizeSkiaSurface(int w, int h);
    void drawBlockProgrammingContent(SkCanvas* canvas);

    sk_sp<GrDirectContext> fContext;
    sk_sp<SkSurface> fSurface;
    GLuint fBoundFbo = 0;  // 缓存的FBO ID

    // 动画
    QTimer m_animationTimer;
    float m_animationTime;
};
