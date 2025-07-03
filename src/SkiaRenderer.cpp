#include "SkiaRenderer.hpp"
#include <QDebug>
#include <QRectF>

// 额外的 Skia 头文件
#include "include/effects/SkGradientShader.h"
#include "include/core/SkFont.h"
#include "include/gpu/ganesh/GrBackendSurface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"

SkiaRenderer::SkiaRenderer(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_animationTime(0.0f)
    , m_cachedFbo(0)
{
    // 设置动画定时器 - 但不立即启动，等窗口显示时再启动
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &SkiaRenderer::updateAnimation);
    // 不在构造函数中启动定时器，避免后台运行
}

SkiaRenderer::~SkiaRenderer()
{
    // 停止动画定时器
    if (m_animationTimer) {
        m_animationTimer->stop();
    }

    // 安全清理GPU资源
    if (context()) {
        makeCurrent();
        cleanupGPUResources();
        doneCurrent();
    }
}

void SkiaRenderer::initializeGL()
{
    initializeOpenGLFunctions();

    // 步骤1：检查OpenGL版本和信息
    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* vendor = reinterpret_cast<const char*>(glGetString(GL_VENDOR));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    qDebug() << "=== OpenGL Info ===";
    qDebug() << "Version:" << (version ? version : "Unknown");
    qDebug() << "Vendor:" << (vendor ? vendor : "Unknown");
    qDebug() << "Renderer:" << (renderer ? renderer : "Unknown");

    // 步骤2：创建 Skia GL 接口 - 关键调试点①
    auto interface = GrGLMakeNativeInterface();
    qDebug() << "① GrGL interface =" << (bool)interface;

    if (!interface) {
        qCritical() << "Failed to create GrGL interface - 您的Skia包可能是CPU-only版本!";
        return;
    }

    // 步骤3：创建GPU上下文 - 关键调试点②
    GrContextOptions opts;
    fContext = GrDirectContexts::MakeGL(interface, opts);
    qDebug() << "② DirectContext =" << (bool)fContext;

    if (!fContext) {
        qCritical() << "Failed to create DirectContext - GPU后端不支持或驱动问题!";
        return;
    }

    qDebug() << "Skia GPU context created successfully";

    // 连接Qt信号以处理上下文销毁
    connect(context(), &QOpenGLContext::aboutToBeDestroyed,
            this, &SkiaRenderer::cleanupGPUResources);
}

void SkiaRenderer::resizeGL(int w, int h)
{
    if (!fContext) {
        qWarning() << "resizeGL called but no valid DirectContext!";
        return;
    }

    qDebug() << "=== resizeGL Debug ===";
    qDebug() << "Size:" << w << "x" << h;

    // 重置缓存的FBO，强制在下一次paintGL时重建Surface
    m_cachedFbo = 0;
    fSurface.reset();

    qDebug() << "Surface reset, will be recreated in paintGL";
}

void SkiaRenderer::paintGL()
{
    if (!fContext) {
        qWarning() << "paintGL called but no valid DirectContext!";
        return;
    }

    // 检查上下文是否被放弃
    if (fContext->abandoned()) {
        qWarning() << "DirectContext was abandoned! Recreating...";
        fContext.reset();
        fSurface.reset();
        return;
    }

    // 调试输出：验证参数不匹配问题
    static bool firstFrame = true;
    if (firstFrame) {
        qDebug() << "=== First Frame Debug Info ===";
        qDebug() << "Qt format samples/stencil:" << context()->format().samples() << context()->format().stencilBufferSize();

        GLint realSamples = 0, realStencil = 0;
        glGetIntegerv(GL_SAMPLES, &realSamples);
        glGetIntegerv(GL_STENCIL_BITS, &realStencil);
        qDebug() << "Real GL samples/stencil:" << realSamples << realStencil;

        if (realSamples != context()->format().samples() || realStencil != context()->format().stencilBufferSize()) {
            qWarning() << "*** PARAMETER MISMATCH DETECTED! This was causing the crash. ***";
        }
        firstFrame = false;
    }

    // 关键修复：每帧检测FBO是否变化，变就重建SkSurface
    GLuint currFbo = defaultFramebufferObject();
    if (!fSurface || currFbo != m_cachedFbo) {
        qDebug() << "FBO changed from" << m_cachedFbo << "to" << currFbo << "- rebuilding Surface";

        m_cachedFbo = currFbo; // 记录最新FBO

        // 获取当前窗口信息
        const int w = width();
        const int h = height();
        const qreal dpr = devicePixelRatio();

        // 调用新的重建Surface方法
        rebuildSurface(w * dpr, h * dpr, currFbo);

        if (!fSurface) {
            qCritical() << "Failed to create GPU surface - cannot continue rendering";
            return;
        }

        qDebug() << "Surface recreated successfully for FBO" << currFbo;
    }

    try {
        SkCanvas* canvas = fSurface->getCanvas();
        if (!canvas) {
            qCritical() << "Failed to get canvas from GPU surface!";
            return;
        }

        canvas->clear(SK_ColorWHITE);

        // 绘制积木编程内容
        drawBlockProgrammingContent(canvas);

        // 安全地flush GPU命令 - 现在使用正确的FBO参数，应该不会崩溃了
        if (fContext && !fContext->abandoned()) {
            fContext->flushAndSubmit();
        } else {
            qWarning() << "Cannot flush - context is abandoned or null";
        }

    } catch (const std::exception& e) {
        qCritical() << "Exception in paintGL:" << e.what();
        // 重置surface，下一帧会重新创建
        fSurface.reset();
        m_cachedFbo = 0;
    } catch (...) {
        qCritical() << "Unknown exception in paintGL - resetting surface";
        // 重置surface，下一帧会重新创建
        fSurface.reset();
        m_cachedFbo = 0;
    }
}

void SkiaRenderer::updateAnimation()
{
    // 只有在窗口可见且有有效上下文时才更新动画
    if (!isVisible() || !fContext || fContext->abandoned()) {
        return;
    }

    m_animationTime += 0.033f; // 增加动画时间（对应30FPS）
    update(); // 触发重绘
}

void SkiaRenderer::showEvent(QShowEvent* event)
{
    QOpenGLWidget::showEvent(event);
    // 窗口显示时启动动画
    if (m_animationTimer && !m_animationTimer->isActive()) {
        m_animationTimer->start(33);
        qDebug() << "Animation timer started";
    }
}

void SkiaRenderer::hideEvent(QHideEvent* event)
{
    QOpenGLWidget::hideEvent(event);
    // 窗口隐藏时停止动画，避免后台误触重绘
    if (m_animationTimer && m_animationTimer->isActive()) {
        m_animationTimer->stop();
        qDebug() << "Animation timer stopped";
    }
}

void SkiaRenderer::rebuildSurface(int w, int h, GLuint fbo)
{
    makeCurrent();

    // 1. 查询实际的 FBO 参数 - 关键修复！
    GLint samples = 0, stencil = 0, fmt = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGetIntegerv(GL_SAMPLES, &samples);
    glGetIntegerv(GL_STENCIL_BITS, &stencil);
    glGetFramebufferAttachmentParameteriv(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE,
        &fmt);

    // 如果查询失败，使用常见默认值
    if (!fmt) fmt = GL_SRGB8_ALPHA8;

    // 调试输出：对比 Qt 认为的 vs 实际的参数
    qDebug() << "=== FBO Parameter Comparison ===";
    qDebug() << "Qt format samples/stencil:" << context()->format().samples() << context()->format().stencilBufferSize();
    qDebug() << "Real GL samples/stencil:" << samples << stencil;
    qDebug() << "Real GL format:" << QString("0x%1").arg(fmt, 0, 16);

    // 2. 格式转换：GL_SRGB8_ALPHA8 -> GL_RGBA8 (Skia 兼容)
    GrGLenum skiaFormat = fmt;
    if (fmt == GL_SRGB8_ALPHA8) {
        skiaFormat = GL_RGBA8;  // Skia 更好地支持 RGBA8
        qDebug() << "Converting GL_SRGB8_ALPHA8 to GL_RGBA8 for Skia compatibility";
    }

    // 3. 使用转换后的格式重新封装
    GrGLFramebufferInfo fbInfo{fbo, skiaFormat};
    GrBackendRenderTarget rt = GrBackendRenderTargets::MakeGL(
        w, h, samples, stencil, fbInfo);

    if (!rt.isValid()) {
        qCritical() << "Invalid BackendRenderTarget with real parameters";
        qCritical() << "FBO:" << fbo << "Size:" << w << "x" << h << "Samples:" << samples << "Stencil:" << stencil;
        return;
    }

    SkSurfaceProps props;
    fSurface = SkSurfaces::WrapBackendRenderTarget(
        fContext.get(), rt,
        kBottomLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,
        nullptr, &props);

    if (!fSurface) {
        qCritical() << "WrapBackendRenderTarget failed even with correct parameters";
        return;
    }

    qDebug() << "GPU Surface created successfully with real FBO parameters";
    qDebug() << "FBO:" << fbo << "Size:" << w << "x" << h << "Samples:" << samples << "Stencil:" << stencil;
}



void SkiaRenderer::cleanupGPUResources()
{
    if (fSurface) {
        fSurface.reset();
        qDebug() << "Skia surface released";
    }

    if (fContext) {
        fContext->abandonContext();
        fContext.reset();
        qDebug() << "Skia GPU context abandoned and released";
    }
}



// 画一个顶部凸舌、底部凹槽的"积木"路径
SkPath SkiaRenderer::makePuzzlePath(const QRectF& r, qreal w, qreal h)
{
    SkPath p;
    p.moveTo(r.left(), r.top());
    p.lineTo(r.left() + w, r.top());                  // notch 左边
    p.cubicTo(r.left() + w, r.top(),
              r.left() + w / 2, r.top() + h,
              r.left() + w, r.top() + 2 * h);         // 凸舌
    p.lineTo(r.right() - w, r.top());
    p.lineTo(r.right(), r.top());
    p.lineTo(r.right(), r.bottom() - 2 * h);
    p.cubicTo(r.right(), r.bottom() - 2 * h,
              r.right() - w / 2, r.bottom() - h,
              r.right() - w, r.bottom());             // 底部凹槽
    p.lineTo(r.left() + w, r.bottom());
    p.close();
    return p;
}

void SkiaRenderer::drawBlockProgrammingContent(SkCanvas* canvas)
{
    // 绘制背景网格
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
    
    // 积木块1 - 蓝色
    blockPaint.setColor(SkColorSetARGB(255, 30, 136, 229));
    canvas->drawPath(makePuzzlePath({50, 50, 120, 40}), blockPaint);
    
    // 积木块2 - 绿色
    blockPaint.setColor(SkColorSetARGB(255, 76, 175, 80));
    canvas->drawPath(makePuzzlePath({200, 100, 140, 40}), blockPaint);
    
    // 积木块3 - 橙色（带动画）
    float animOffset = sin(m_animationTime * 2.0f) * 10.0f;
    blockPaint.setColor(SkColorSetARGB(255, 255, 152, 0));
    canvas->drawPath(makePuzzlePath({100 + animOffset, 200, 160, 40}), blockPaint);

    // 绘制文本
    SkPaint textPaint;
    textPaint.setAntiAlias(true);
    textPaint.setColor(SK_ColorBLACK);
    
    SkFont font;
    font.setSize(24);
    
    const char* title = "Block Programming View - Skia GPU Rendering";
    canvas->drawString(title, 50, 30, font, textPaint);
    
    // 绘制状态信息
    font.setSize(16);
    textPaint.setColor(SkColorSetARGB(255, 100, 100, 100));
    
    QString statusText = QString("Animation Time: %1s | Size: %2x%3")
                        .arg(m_animationTime, 0, 'f', 2)
                        .arg(width())
                        .arg(height());
    
    canvas->drawString(statusText.toUtf8().constData(), 50, height() - 20, font, textPaint);
}
