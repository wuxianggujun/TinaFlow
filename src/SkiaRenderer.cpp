#include "SkiaRenderer.hpp"
#include <QDebug>
#include <QtMath>
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkFont.h"
#include "include/core/SkPath.h"
#include "include/core/SkColorSpace.h"
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
    if (!fSurface) {
        qDebug() << "paintGL: No surface available";
        return;
    }

    SkCanvas* canvas = fSurface->getCanvas();
    if (!canvas) {
        qDebug() << "paintGL: No canvas available";
        return;
    }

    // 清除为白色背景
    canvas->clear(SK_ColorWHITE);

    // 添加调试信息
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0) {  // 每60帧打印一次
        qDebug() << "paintGL: Frame" << frameCount << "Animation time:" << m_animationTime;
    }

    // 绘制内容
    drawBlockProgrammingContent(canvas);

    // 确保所有绘制命令都被提交到GPU
    fContext->flushAndSubmit();
}

void SkiaRenderer::resizeSkiaSurface(int w, int h)
{
    if (!fContext) return;

    GrGLFramebufferInfo fbInfo;
    fbInfo.fFBOID = defaultFramebufferObject();
    fbInfo.fFormat = GL_RGBA8;

    GrBackendRenderTarget backendRenderTarget = GrBackendRenderTargets::MakeGL(
        w, h, /*sampleCnt*/ 0, /*stencilBits*/ 8, fbInfo);

    SkColorType colorType = kRGBA_8888_SkColorType;
    SkSurfaceProps props;

    fSurface = SkSurfaces::WrapBackendRenderTarget(
        fContext.get(),
        backendRenderTarget,
        kTopLeft_GrSurfaceOrigin,  // 改为TopLeft，与Qt对齐
        colorType,
        nullptr,
        &props
    );

    if (!fSurface) {
        qCritical() << "Failed to create Skia surface";
    } else {
        qDebug() << "Skia surface created successfully, size:" << w << "x" << h;
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

    // 首先绘制一些明显的测试图形
    SkPaint testPaint;
    testPaint.setAntiAlias(true);

    // 绘制一个大红色矩形
    testPaint.setColor(SK_ColorRED);
    testPaint.setStyle(SkPaint::kFill_Style);
    canvas->drawRect(SkRect::MakeXYWH(50, 50, 200, 100), testPaint);

    // 绘制一个蓝色圆形
    testPaint.setColor(SK_ColorBLUE);
    canvas->drawCircle(400, 100, 50, testPaint);

    // 绘制一个绿色边框矩形
    testPaint.setColor(SK_ColorGREEN);
    testPaint.setStyle(SkPaint::kStroke_Style);
    testPaint.setStrokeWidth(5);
    canvas->drawRect(SkRect::MakeXYWH(100, 200, 150, 80), testPaint);

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
