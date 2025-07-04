#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QSurfaceFormat>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QDebug>

// Skia
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLTypes.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

/**
 * @brief 基于工作示例的Skia渲染器
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
    void resizeEvent(QResizeEvent* event) override;

    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private slots:
    void onTick();

private:
    void createSkiaSurface();
    void drawBlockProgrammingContent(SkCanvas* canvas);

    sk_sp<GrDirectContext> skContext;
    sk_sp<SkSurface> skSurface;

    // 动画
    QTimer m_animationTimer;
    float m_animationTime;
};
