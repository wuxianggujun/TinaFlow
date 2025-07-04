#include "SkiaRenderer.hpp"
#include <QDebug>
#include <QtMath>
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkFont.h"
#include "include/core/SkPath.h"
#include "include/core/SkColorSpace.h"
#include "include/effects/SkGradientShader.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"

SkiaRenderer::SkiaRenderer(QWidget* parent)
    : QOpenGLWidget(parent), fContext(nullptr), fSurface(nullptr), m_animationTime(0.0f)
{
    // 设置OpenGL格式
    QSurfaceFormat fmt;
    fmt.setRenderableType(QSurfaceFormat::OpenGL);
    fmt.setVersion(3, 3);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(fmt);

    // ❶ 关键修复：关闭Qt的partial-update blit，避免Intel GPU驱动兼容性问题
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);

    // 设置动画定时器
    connect(&m_animationTimer, &QTimer::timeout, this, &SkiaRenderer::onTick);
}

SkiaRenderer::~SkiaRenderer()
{
    makeCurrent();
    fSurface.reset();
    fContext.reset();
    doneCurrent();
}

void SkiaRenderer::initializeGL()
{
    initializeOpenGLFunctions();

    // ❶ 关键修复：彻底"封印" scissor，防止Qt在blit时重新开启
    glDisable(GL_SCISSOR_TEST);

    auto interface = GrGLMakeNativeInterface();
    fContext = GrDirectContexts::MakeGL(interface);  // 使用正确的API

    if (!fContext) {
        qCritical() << "Failed to create Skia GrDirectContext";
        return;
    }

    qDebug() << "SkiaRenderer: GL ready:"
             << reinterpret_cast<const char*>(glGetString(GL_VERSION));

    resizeSkiaSurface(width(), height());
}

void SkiaRenderer::resizeGL(int w, int h)
{
    resizeSkiaSurface(w, h);
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

void SkiaRenderer::paintGL()
{
    /* ★ 1. 让整张 FBO 可写 — 在 *Qt 已经* 改完脏矩形之后 */
    const qreal dpr = devicePixelRatioF();
    glViewport(0, 0, int(width()*dpr), int(height()*dpr));
    glDisable(GL_SCISSOR_TEST);

    if (!fSurface || defaultFramebufferObject() != fBoundFbo)
        resizeSkiaSurface(width(), height());

    if (!fSurface) return;

    SkCanvas* cv = fSurface->getCanvas();
    cv->resetMatrix();                    // 保证坐标系总是正确
    cv->scale(dpr, dpr);

    // ❷ 关键修复：只画渐变，不再先清黑
    SkColor cols[2] = { SK_ColorYELLOW, SK_ColorGREEN };
    SkPoint pts[2] = { {0,0}, {float(width()),float(height())} };
    SkPaint g;
    g.setShader(SkGradientShader::MakeLinear(pts, cols, nullptr, 2, SkTileMode::kClamp));
    g.setBlendMode(SkBlendMode::kSrc);              // 保证一定覆盖
    cv->drawRect(SkRect::MakeWH(width(), height()), g);

    fContext->flushAndSubmit();

    /* ★ 2. 再次覆盖  —— Qt 在返回前还会再开一次 scissor！ */
    glViewport(0, 0, int(width()*dpr), int(height()*dpr));
    glDisable(GL_SCISSOR_TEST);
}

void SkiaRenderer::resizeSkiaSurface(int w, int h)
{
    fSurface.reset();
    if (!fContext || w <= 0 || h <= 0) return;

    // 关键修复：使用像素尺寸而不是逻辑尺寸
    const qreal dpr = devicePixelRatioF();          // Hi-DPI 缩放系数
    const int   fbW = int(w * dpr + 0.5);
    const int   fbH = int(h * dpr + 0.5);

    GLuint fbo = defaultFramebufferObject();
    fBoundFbo  = fbo;

    qDebug() << "=== FBO Info ===";
    qDebug() << "FBO ID:" << fbo;
    qDebug() << "Logical size:" << w << "x" << h;
    qDebug() << "DPR:" << dpr;
    qDebug() << "Pixel size:" << fbW << "x" << fbH;

    GrGLFramebufferInfo fbInfo{ fbo, GL_RGBA8 };
    auto backendRT = GrBackendRenderTargets::MakeGL(
        fbW, fbH, format().samples(), format().stencilBufferSize(), fbInfo);

    if (!backendRT.isValid()) {
        qWarning() << "[SKIA] backendRT invalid, fallback no-MSAA";
        backendRT = GrBackendRenderTargets::MakeGL(fbW, fbH, 0, 0, fbInfo);
    }

    fSurface = SkSurfaces::WrapBackendRenderTarget(
        fContext.get(), backendRT,
        kBottomLeft_GrSurfaceOrigin, kRGBA_8888_SkColorType, nullptr, nullptr);

    if (!fSurface) {
        qCritical() << "[SKIA] WrapBackendRenderTarget FAILED";
    } else {
        qDebug() << "[SKIA] Surface created successfully";
        qDebug() << "Pixel size:" << fbW << "x" << fbH << "DPR:" << dpr;
    }
}

void SkiaRenderer::onTick()
{
    m_animationTime += 0.033f;
    update();
}

void SkiaRenderer::drawBlockProgrammingContent(SkCanvas* canvas)
{
    if (!canvas) {
        qDebug() << "drawBlockProgrammingContent: No canvas!";
        return;
    }

    static int drawCount = 0;
    drawCount++;
    if (drawCount % 60 == 0) {  // 每60帧打印一次
        qDebug() << "drawBlockProgrammingContent: Draw call" << drawCount
                 << "Size:" << width() << "x" << height();
    }

    // 首先测试最基础的绘制 - 填充整个画布
    SkPaint bgPaint;
    bgPaint.setColor(SkColorSetARGB(100, 255, 0, 0));  // 半透明红色背景
    canvas->drawRect(SkRect::MakeXYWH(0, 0, width(), height()), bgPaint);

    // 绘制一些明显的测试图形
    SkPaint testPaint;
    testPaint.setAntiAlias(true);

    // 绘制一个大红色矩形 - 使用更大的尺寸
    testPaint.setColor(SK_ColorRED);
    testPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(SkRect::MakeXYWH(100, 100, 300, 200), testPaint);

    // 绘制一个蓝色圆形 - 使用更大的半径
    testPaint.setColor(SK_ColorBLUE);
    canvas->drawCircle(500, 200, 80, testPaint);

    // 绘制一个绿色边框矩形 - 使用更粗的线条
    testPaint.setColor(SK_ColorGREEN);
    testPaint.setStyle(SkPaint::kStroke_Style);
    testPaint.setStrokeWidth(10);
    canvas->drawRect(SkRect::MakeXYWH(200, 350, 200, 100), testPaint);

    // 绘制一些对角线
    testPaint.setColor(SK_ColorBLACK);
    testPaint.setStrokeWidth(5);
    canvas->drawLine(0, 0, width(), height(), testPaint);
    canvas->drawLine(width(), 0, 0, height(), testPaint);

    // 绘制网格背景
    SkPaint gridPaint;
    gridPaint.setColor(SkColorSetARGB(255, 100, 100, 100));  // 完全不透明的灰色
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
