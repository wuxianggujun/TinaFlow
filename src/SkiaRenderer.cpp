#include "SkiaRenderer.hpp"
#include <QDebug>
#include <QtMath>
#include <QOpenGLFunctions>

#include "include/core/SkCanvas.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLBackendSurface.h"
#include "include/gpu/ganesh/GrBackendSurface.h"  // 添加这个头文件
#include "include/effects/SkGradientShader.h"
#include "include/core/SkFont.h"
#include "include/core/SkPaint.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"

// ---------- 构造 / 析构 ----------
SkiaRenderer::SkiaRenderer(QWidget* p):QOpenGLWidget(p)
{
    connect(&timer,&QTimer::timeout,this,&SkiaRenderer::onTick);
}
SkiaRenderer::~SkiaRenderer(){ purge(); }

// ---------- 初始化 ----------
void SkiaRenderer::initializeGL()
{
    initializeOpenGLFunctions();

    // 1) 创建 Skia GL 接口 + 上下文
    auto iface = GrGLMakeNativeInterface();
    if(!iface) qFatal("GrGL interface nullptr");
    fCtx = GrDirectContexts::MakeGL(iface);
    if(!fCtx) qFatal("Create GrDirectContext failed");

    qDebug()<<"SkiaRenderer: GL ready:"
            << reinterpret_cast<const char*>(glGetString(GL_VERSION));

    // 当 Qt 销毁 context 时清理
    connect(context(),&QOpenGLContext::aboutToBeDestroyed,
            this,&SkiaRenderer::purge);
}

void SkiaRenderer::resizeGL(int,int){ /*Surface 在 paintGL 内按需重建*/ }

void SkiaRenderer::showEvent(QShowEvent* e)
{
    QOpenGLWidget::showEvent(e);
    if(!timer.isActive()) timer.start(33);
}
void SkiaRenderer::hideEvent(QHideEvent* e)
{
    QOpenGLWidget::hideEvent(e);
    timer.stop();
}

// ---------- 核心绘制 ----------
void SkiaRenderer::paintGL()
{
    if(!fCtx) return;

    // Qt 每帧可能换新的 FBO
    GLuint fbo = defaultFramebufferObject();
    if(!fSurf || fbo!=currFbo){
        currFbo = fbo;
        // 尝试从 LRU 缓存复用
        auto it = std::find_if(cache.begin(),cache.end(),
                               [fbo](const CacheEntry& c){return c.fbo==fbo;});
        if(it!=cache.end()){
            fSurf = it->surf;                // 命中缓存
            cache.erase(it); cache.prepend({fbo,fSurf});
        }else{
            recreateSurface(width()*devicePixelRatio(),
                            height()*devicePixelRatio(),fbo);
            cache.prepend({fbo,fSurf});
            while(cache.size()>3) cache.removeLast();
        }
    }

    if(!fSurf) return;

    // -------- 真正绘制 --------
    SkCanvas* cv = fSurf->getCanvas();
    cv->clear(SK_ColorWHITE);
    drawScene(cv);

    // -------- 提交 --------
    fCtx->flushAndSubmit();  // 修复API调用
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);   // 恢复给 Qt
}

// ---------- recreateSurface ----------
GrGLenum SkiaRenderer::detectColorFormat(GLuint fbo)
{
    // 简化实现，直接返回最兼容的格式，避免复杂的OpenGL查询
    Q_UNUSED(fbo)
    return GL_RGBA8;  // 使用最兼容的格式
}

void SkiaRenderer::recreateSurface(int w,int h,GLuint fbo)
{
    makeCurrent();               // 已经 current，但显式保证

    // 1) 关闭 sRGB 写（Qt 若为 SRGB FBO，保持线性写入更安全）
    glDisable(GL_FRAMEBUFFER_SRGB);

    // 2) 真实样本 / 模版
    GLint samples=0, stencil=0;
    glBindFramebuffer(GL_FRAMEBUFFER,fbo);
    glGetIntegerv(GL_SAMPLES,&samples);
    glGetIntegerv(GL_STENCIL_BITS,&stencil);

    // 3) BackendRenderTarget
    GrGLFramebufferInfo fbInfo{fbo, detectColorFormat(fbo)};
    GrBackendRenderTarget rt = GrBackendRenderTargets::MakeGL(
        w,h,samples,stencil,fbInfo);
    if(!rt.isValid()){
        qWarning()<<"BackendRenderTarget invalid, retry without MSAA";
        rt = GrBackendRenderTargets::MakeGL(w,h,0,stencil,fbInfo);
    }
    if(!rt.isValid()){ fSurf.reset(); return; }

    // 4) Wrap 成 SkSurface (TopLeft 与 Qt 对齐)
    SkSurfaceProps props;
    fSurf = SkSurfaces::WrapBackendRenderTarget(
        fCtx.get(),rt,kTopLeft_GrSurfaceOrigin,
        kRGBA_8888_SkColorType,nullptr,&props);
}

void SkiaRenderer::purge()
{
    cache.clear();
    fSurf.reset();
    if(fCtx){ fCtx->abandonContext(); fCtx.reset(); }
}

// ---------- 动画 ----------
void SkiaRenderer::onTick(){ t += 0.033f; update(); }

// ---------- 绘制内容 ----------
SkPath SkiaRenderer::makePuzzle(const QRectF& r,qreal w,qreal h) const
{
    SkPath p; p.moveTo(r.left(),r.top());
    p.lineTo(r.left()+w,r.top());
    p.cubicTo(r.left()+w,r.top(),
              r.left()+w/2,r.top()+h,
              r.left()+w,r.top()+2*h);
    p.lineTo(r.right()-w,r.top());
    p.lineTo(r.right(),r.top());
    p.lineTo(r.right(),r.bottom()-2*h);
    p.cubicTo(r.right(),r.bottom()-2*h,
              r.right()-w/2,r.bottom()-h,
              r.right()-w,r.bottom());
    p.lineTo(r.left()+w,r.bottom());
    p.close();
    return p;
}

void SkiaRenderer::drawScene(SkCanvas* c)
{
    // --- 网格背景 ---
    SkPaint g; g.setColor(SkColorSetARGB(40,120,120,120));
    for(int x=0;x<width();x+=20) c->drawLine(x,0,x,height(),g);
    for(int y=0;y<height();y+=20) c->drawLine(0,y,width(),y,g);

    // --- 三块积木 ---
    SkPaint p; p.setAntiAlias(true);
    p.setColor(SkColorSetARGB(255,33,150,243));
    c->drawPath(makePuzzle({40,40,120,40}),p);
    p.setColor(SkColorSetARGB(255,76,175,80));
    c->drawPath(makePuzzle({200,90,140,40}),p);
    p.setColor(SkColorSetARGB(255,255,152,0));
    float dx = 15.f*qSin(t*2.f);
    c->drawPath(makePuzzle({100+dx,190,160,40}),p);

    // --- 文本 ---
    SkPaint tp; tp.setAntiAlias(true); tp.setColor(SK_ColorBLACK);
    SkFont  f;  f.setSize(22);
    c->drawString("Block Programming - Skia/GL",50,30,f,tp);
}
