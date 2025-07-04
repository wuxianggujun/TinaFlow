#include "SkiaRenderer.hpp"
#include <QtMath>
#include "include/core/SkPaint.h"
#include "include/core/SkFont.h"
#include "include/core/SkPath.h"
#include "include/effects/SkGradientShader.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/GrBackendSurface.h"

SkiaRenderer::SkiaRenderer(QWidget* parent)
    : QOpenGLWidget(parent), skContext(nullptr), skSurface(nullptr), m_animationTime(0.0f)
{
    setMinimumSize(800, 600);

    // 设置动画定时器
    connect(&m_animationTimer, &QTimer::timeout, this, &SkiaRenderer::onTick);
}

SkiaRenderer::~SkiaRenderer()
{
    makeCurrent();
    skSurface.reset();
    skContext.reset();
    doneCurrent();
}

void SkiaRenderer::initializeGL()
{
    initializeOpenGLFunctions();
    sk_sp<const GrGLInterface> interface(GrGLMakeNativeInterface());
    skContext = GrDirectContexts::MakeGL(interface);
    createSkiaSurface();
}

void SkiaRenderer::resizeGL(int w, int h)
{
    createSkiaSurface();
}

void SkiaRenderer::paintGL()
{
    if (!skSurface) return;
    SkCanvas* canvas = skSurface->getCanvas();
    canvas->clear(SK_ColorWHITE);

    // Draw something (replace with your drawing code)
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    canvas->drawRect(SkRect::MakeXYWH(50, 50, 200, 100), paint);

    paint.setColor(SK_ColorBLUE);
    canvas->drawCircle(300, 100, 40, paint);

    skContext->flush();
    // Qt will swap buffers automatically
}

void SkiaRenderer::resizeEvent(QResizeEvent* event)
{
    QOpenGLWidget::resizeEvent(event);
    // Skia surface is recreated in resizeGL
}

void SkiaRenderer::showEvent(QShowEvent* e)
{
    QOpenGLWidget::showEvent(e);
    if (!m_animationTimer.isActive()) {
        m_animationTimer.start(33);  // ~30 FPS
        qDebug() << "Animation timer started";
    }
}

void SkiaRenderer::hideEvent(QHideEvent* e)
{
    QOpenGLWidget::hideEvent(e);
    m_animationTimer.stop();
    qDebug() << "Animation timer stopped";
}

void SkiaRenderer::onTick()
{
    m_animationTime += 0.033f;
    update();
}

void SkiaRenderer::createSkiaSurface()
{
    if (!skContext) return;
    skContext->flush();
    skSurface.reset();

    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = defaultFramebufferObject();
    fbInfo.fFormat = GL_RGBA8;

    SkColorType colorType = kRGBA_8888_SkColorType;

    GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(
        width(), height(),
        this->format().samples(),  // sample count
        this->format().stencilBufferSize(),  // stencil bits
        fbInfo
    );

    skSurface = SkSurfaces::WrapBackendRenderTarget(
        skContext.get(),
        backendRenderTarget,
        kBottomLeft_GrSurfaceOrigin,
        colorType,
        nullptr,
        nullptr
    );

    if (!skSurface) {
        qCritical() << "Failed to create Skia surface";
    } else {
        qDebug() << "Skia surface created successfully, size:" << width() << "x" << height();
    }
}

void SkiaRenderer::drawBlockProgrammingContent(SkCanvas* canvas)
{
    if (!canvas) return;

    // 绘制网格背景
    SkPaint gridPaint;
    gridPaint.setColor(SkColorSetARGB(50, 200, 200, 200));
    gridPaint.setStrokeWidth(1);

    int gridSize = 20;
    for (int x = 0; x < width(); x += gridSize) {
        canvas->drawLine(x, 0, x, height(), gridPaint);
    }
    for (int y = 0; y < height(); y += gridSize) {
        canvas->drawLine(0, y, width(), y, gridPaint);
    }

    // 绘制示例积木块
    SkPaint blockPaint;
    blockPaint.setAntiAlias(true);

    // 积木块1 - 蓝色矩形
    blockPaint.setColor(SkColorSetARGB(255, 30, 136, 229));
    canvas->drawRoundRect(SkRect::MakeXYWH(50, 50, 120, 40), 8, 8, blockPaint);

    // 积木块2 - 绿色矩形
    blockPaint.setColor(SkColorSetARGB(255, 76, 175, 80));
    canvas->drawRoundRect(SkRect::MakeXYWH(200, 100, 140, 40), 8, 8, blockPaint);

    // 积木块3 - 橙色（带动画）
    float animOffset = sin(m_animationTime * 2.0f) * 10.0f;
    blockPaint.setColor(SkColorSetARGB(255, 255, 152, 0));
    canvas->drawRoundRect(SkRect::MakeXYWH(100 + animOffset, 200, 160, 40), 8, 8, blockPaint);

    // 绘制文本
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorBLACK);

    SkFont font;
    font.setSize(24);

    canvas->drawString("Block Programming View - Skia Rendering", 50, 30, font, textPaint);

    // 绘制状态信息
    font.setSize(16);
    textPaint.setColor(SkColorSetARGB(255, 100, 100, 100));

    QString statusText = QString("Animation Time: %1s | Size: %2x%3")
                        .arg(m_animationTime, 0, 'f', 2)
                        .arg(width())
                        .arg(height());

    canvas->drawString(statusText.toUtf8().constData(), 50, height() - 20, font, textPaint);
}
