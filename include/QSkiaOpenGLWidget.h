#pragma once

#include <QtOpenGLWidgets/QOpenGLWidget>

class SkCanvas;
class QSkiaOpenGLWidgetPrivate;

/**
 * @brief 基于 Skia 的 OpenGL Widget，避开 Qt 默认 FBO 的问题
 *
 * 这个类让 Skia 自己创建和管理 RenderTarget，而不是包装 Qt 的 FBO，
 * 从而避免了 FBO 参数不匹配导致的崩溃和黑屏问题。
 */
class QSkiaOpenGLWidget : public QOpenGLWidget
{
    // 不使用 Q_OBJECT，因为这个基类不需要信号槽功能

public:
    explicit QSkiaOpenGLWidget(QWidget* parent = nullptr);
    ~QSkiaOpenGLWidget() override;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

    /**
     * @brief 子类需要实现这个方法来进行实际的绘制
     * @param canvas Skia 画布
     * @param elapsed 距离上一帧的时间（毫秒）
     */
    virtual void draw(SkCanvas* canvas, int elapsed) = 0;

private:
    void init(int w, int h);

    QSkiaOpenGLWidgetPrivate* m_dptr;
};
