#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QHash>
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLTypes.h"  // 添加GrGLenum类型
#include "include/core/SkSurface.h"
#include "include/core/SkPath.h"

/** 真正集成在 TinaFlow 中的渲染器 */
class SkiaRenderer final : public QOpenGLWidget,
                           protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit SkiaRenderer(QWidget* parent=nullptr);
    ~SkiaRenderer() override;

protected:
    // Qt‑OpenGL 生命周期
    void initializeGL() override;
    void resizeGL(int w,int h) override;
    void paintGL()   override;

    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

private slots:
    void onTick();

private:
    // -- 绘制辅助 --
    SkPath makePuzzle(const QRectF& r,qreal nw=8,qreal nh=4) const;
    void   drawScene(SkCanvas*);

    // -- GPU / 资源管理 --
    void   recreateSurface(int w,int h,GLuint fbo);
    void   purge();
    GrGLenum detectColorFormat(GLuint fbo);  // 添加函数声明

    // Skia 对象
    sk_sp<GrDirectContext>   fCtx;
    sk_sp<SkSurface>         fSurf;

    // FBO→Surface 缓存 (LRU≤3)
    struct CacheEntry { GLuint fbo; sk_sp<SkSurface> surf; };
    QList<CacheEntry>        cache;
    GLuint                   currFbo = 0;

    // 动画
    QTimer  timer;
    float   t = 0.f;
};
