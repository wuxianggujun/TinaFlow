#include "SkiaRenderer.hpp"
#include <QDebug>
#include <QRectF>
#include <QPainter>

// Skia CPU 渲染头文件
#include "include/effects/SkGradientShader.h"
#include "include/core/SkFont.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"

SkiaRenderer::SkiaRenderer(QWidget* parent)
    : QWidget(parent)  // 改为普通QWidget，避免OpenGL冲突
    , m_animationTime(0.0f)
    , m_skiaImage(nullptr)
{
    // 设置动画定时器
    m_animationTimer = new QTimer(this);
    connect(m_animationTimer, &QTimer::timeout, this, &SkiaRenderer::updateAnimation);

    // 设置最小尺寸
    setMinimumSize(800, 600);

    qDebug() << "SkiaRenderer: Using CPU rendering to avoid OpenGL conflicts";
}

SkiaRenderer::~SkiaRenderer()
{
    // 停止动画定时器
    if (m_animationTimer) {
        m_animationTimer->stop();
    }

    // 清理Skia资源
    if (m_skiaImage) {
        delete[] m_skiaImage;
        m_skiaImage = nullptr;
    }
}

void SkiaRenderer::initializeSkia()
{
    // 初始化CPU渲染的Skia surface
    const int w = width();
    const int h = height();

    if (w <= 0 || h <= 0) {
        qWarning() << "Invalid widget size for Skia initialization:" << w << "x" << h;
        return;
    }

    // 创建图像缓冲区
    if (m_skiaImage) {
        delete[] m_skiaImage;
    }

    const size_t imageSize = w * h * 4; // RGBA
    m_skiaImage = new uint8_t[imageSize];

    // 创建Skia CPU surface
    SkImageInfo info = SkImageInfo::MakeN32Premul(w, h);
    m_surface = SkSurfaces::WrapPixels(info, m_skiaImage, w * 4);

    if (!m_surface) {
        qCritical() << "Failed to create Skia CPU surface";
        return;
    }

    qDebug() << "Skia CPU surface created successfully, size:" << w << "x" << h;
}

void SkiaRenderer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    qDebug() << "SkiaRenderer resized to:" << width() << "x" << height();

    // 重新初始化Skia surface
    initializeSkia();

    // 触发重绘
    update();
}

void SkiaRenderer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    // 确保Skia surface已初始化
    if (!m_surface) {
        initializeSkia();
        if (!m_surface) {
            qWarning() << "Cannot paint - Skia surface not available";
            return;
        }
    }

    // 使用Skia绘制到CPU缓冲区
    SkCanvas* canvas = m_surface->getCanvas();
    if (!canvas) {
        qWarning() << "Cannot get canvas from Skia surface";
        return;
    }

    // 清除画布
    canvas->clear(SK_ColorWHITE);

    // 绘制积木编程内容
    drawBlockProgrammingContent(canvas);

    // 将Skia渲染结果转换为QImage并用QPainter绘制到Qt widget
    const int w = width();
    const int h = height();

    if (w > 0 && h > 0 && m_skiaImage) {
        QImage qimg(m_skiaImage, w, h, w * 4, QImage::Format_RGBA8888);

        QPainter painter(this);
        painter.drawImage(0, 0, qimg);
    }
}

void SkiaRenderer::updateAnimation()
{
    // 只有在窗口可见时才更新动画
    if (!isVisible()) {
        return;
    }

    m_animationTime += 0.033f; // 增加动画时间（对应30FPS）
    update(); // 触发重绘
}

void SkiaRenderer::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // 初始化Skia（如果还没有初始化）
    if (!m_surface) {
        initializeSkia();
    }

    // 窗口显示时启动动画
    if (m_animationTimer && !m_animationTimer->isActive()) {
        m_animationTimer->start(33);
        qDebug() << "Animation timer started";
    }
}

void SkiaRenderer::hideEvent(QHideEvent* event)
{
    QWidget::hideEvent(event);
    // 窗口隐藏时停止动画，避免后台误触重绘
    if (m_animationTimer && m_animationTimer->isActive()) {
        m_animationTimer->stop();
        qDebug() << "Animation timer stopped";
    }
}

// GPU相关方法已删除，现在使用CPU渲染



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
