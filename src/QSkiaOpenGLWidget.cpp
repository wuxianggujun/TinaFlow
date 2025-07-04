#include "QSkiaOpenGLWidget.h"

#include "include/core/SkImageInfo.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLDirectContext.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

#include <QOpenGLFunctions>
#include <QElapsedTimer>
#include <QTimer>
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>

#include "include/core/SkCanvas.h"

class QSkiaOpenGLWidgetPrivate {
public:
    QOpenGLFunctions funcs;
    sk_sp<const GrGLInterface> glInterface = nullptr;
    sk_sp<GrDirectContext> context = nullptr;
    sk_sp<SkSurface> gpuSurface = nullptr;
    SkImageInfo info;
    QTimer timer;
    QElapsedTimer lastTime;
};

QSkiaOpenGLWidget::QSkiaOpenGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
    , m_dptr(new QSkiaOpenGLWidgetPrivate)
{
    connect(&m_dptr->timer, &QTimer::timeout, this, QOverload<>::of(&QSkiaOpenGLWidget::update));
    m_dptr->timer.start(1000 / qRound(qApp->primaryScreen()->refreshRate()));
}

QSkiaOpenGLWidget::~QSkiaOpenGLWidget()
{
    qDebug() << "QSkiaOpenGLWidget::~QSkiaOpenGLWidget() - 开始析构";

    try {
        makeCurrent();

        // 安全清理 Skia 资源
        if (m_dptr) {
            if (m_dptr->gpuSurface) {
                m_dptr->gpuSurface.reset();
                qDebug() << "QSkiaOpenGLWidget: Surface 已清理";
            }

            if (m_dptr->context) {
                m_dptr->context->abandonContext();
                m_dptr->context.reset();
                qDebug() << "QSkiaOpenGLWidget: Context 已清理";
            }

            delete m_dptr;
            m_dptr = nullptr;
        }

        doneCurrent();
        qDebug() << "QSkiaOpenGLWidget::~QSkiaOpenGLWidget() - 析构完成";
    } catch (...) {
        qCritical() << "QSkiaOpenGLWidget::~QSkiaOpenGLWidget() - 析构时发生异常";
    }
}

void QSkiaOpenGLWidget::initializeGL()
{
    m_dptr->funcs.initializeOpenGLFunctions();

    // 创建 Skia GPU 上下文 - 使用原来的简单方式
    m_dptr->context = GrDirectContexts::MakeGL(m_dptr->glInterface);
    if (!m_dptr->context) {
        qCritical() << "Failed to create GrDirectContext";
        return;
    }

    qDebug() << "QSkiaOpenGLWidget: Skia GPU context created successfully";

    init(this->width(), this->height());
    m_dptr->lastTime.start();
}

void QSkiaOpenGLWidget::resizeGL(int w, int h)
{
    if (this->width() == w && this->height() == h) {
        return;
    }
    init(w, h);
}

void QSkiaOpenGLWidget::init(int w, int h)
{
    if (!m_dptr->context) {
        return;
    }

    // 创建 ImageInfo
    m_dptr->info = SkImageInfo::MakeN32Premul(w, h);

    // 让 Skia 自己创建 RenderTarget，避开 Qt FBO 的问题
    m_dptr->gpuSurface = SkSurfaces::RenderTarget(
        m_dptr->context.get(),
        skgpu::Budgeted::kNo,
        m_dptr->info);

    if (!m_dptr->gpuSurface) {
        qCritical() << "SkSurfaces::RenderTarget return null";
        return;
    }

    qDebug() << "QSkiaOpenGLWidget: Surface created successfully, size:" << w << "x" << h;

    m_dptr->funcs.glViewport(0, 0, w, h);
}

void QSkiaOpenGLWidget::paintGL()
{
    if (!isVisible() || !m_dptr->gpuSurface) return;

    SkCanvas* canvas = m_dptr->gpuSurface->getCanvas();
    const qint64 elapsed = m_dptr->lastTime.restart();

    /* ---- 绘制 ---- */
    canvas->save();
    draw(canvas, elapsed);          // 你的自定义绘制
    canvas->restore();

    /* ---- 只 flush 当前 surface ---- */
    m_dptr->context->flushAndSubmit();

    // 暂时简化：只是让 Skia 渲染，不做 blit
    // 这样至少可以编译通过，我们可以看到是否有其他问题
    qDebug() << "Skia surface rendered, size:" << width() << "x" << height();
}
