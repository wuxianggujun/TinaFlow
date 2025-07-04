#include "QSkiaOpenGLWidget.h"
#include <QElapsedTimer>
#include <QTimer>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>

#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"

struct QSkiaOpenGLWidgetPrivate {
    sk_sp<const GrGLInterface> glIf;
    sk_sp<GrDirectContext>     ctx;
    sk_sp<SkSurface>           surf;
    SkImageInfo                info;
    QElapsedTimer              clock;
    QTimer                     tick;
};

QSkiaOpenGLWidget::QSkiaOpenGLWidget(QWidget* parent)
    : QOpenGLWidget(parent), d(new QSkiaOpenGLWidgetPrivate)
{
    connect(&d->tick, &QTimer::timeout, this,
            QOverload<>::of(&QSkiaOpenGLWidget::update));

    // 修复语法错误：使用条件运算符
    qreal refreshRate = qApp->primaryScreen()->refreshRate();
    if (refreshRate <= 0) refreshRate = 60;
    d->tick.start(1000 / int(refreshRate));
}

QSkiaOpenGLWidget::~QSkiaOpenGLWidget()
{
    makeCurrent();
    d->surf.reset();
    if (d->ctx)  d->ctx->abandonContext();
    doneCurrent();
    delete d;
}

void QSkiaOpenGLWidget::initializeGL()
{
    d->glIf = GrGLMakeNativeInterface();
    if (!d->glIf) {
        qFatal("Cannot create GrGL interface");
    }

    d->ctx = GrDirectContexts::MakeGL(d->glIf);
    if (!d->ctx) {
        qFatal("Cannot create GrDirectContext");
    }

    recreateSurface(width(), height());
    d->clock.start();
}

void QSkiaOpenGLWidget::resizeGL(int w,int h){ recreateSurface(w,h); }

void QSkiaOpenGLWidget::recreateSurface(int w,int h)
{
    if (!d->ctx) return;
    d->info  = SkImageInfo::MakeN32Premul(w,h);
    d->surf  = SkSurfaces::RenderTarget(d->ctx.get(), skgpu::Budgeted::kYes, d->info);
    if (!d->surf) qFatal("SkSurfaces::RenderTarget failed");
    glViewport(0,0,w,h);
}

void QSkiaOpenGLWidget::paintGL()
{
    if (!d->surf) return;
    SkCanvas* cv = d->surf->getCanvas();
    cv->save();
    draw(cv, int(d->clock.restart()));
    cv->restore();

    // 修复flushAndSubmit调用 - 不传参数或传GrSyncCpu枚举
    d->ctx->flushAndSubmit();
}
