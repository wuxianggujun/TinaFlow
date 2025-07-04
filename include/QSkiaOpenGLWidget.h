#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>

class SkCanvas;
class QSkiaOpenGLWidgetPrivate;

/**
 * 纯演示用基类：Skia 在自己创建的 RenderTarget 上绘制，
 * 不与 Qt 的默认 FBO 交互，可避免参数不一致。
 */
class QSkiaOpenGLWidget : public QOpenGLWidget
{
public:
    explicit QSkiaOpenGLWidget(QWidget* parent = nullptr);
    ~QSkiaOpenGLWidget() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    /** 子类实现此函数完成绘制 */
    virtual void draw(SkCanvas* canvas, int elapsedMs) = 0;

private:
    void recreateSurface(int w, int h);
    QSkiaOpenGLWidgetPrivate* d;
};
