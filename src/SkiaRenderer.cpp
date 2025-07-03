#include "SkiaRenderer.hpp"
#include <QPainter>
#include <QDebug>

// 额外的 Skia 头文件
#include "include/effects/SkGradientShader.h"
#include "include/core/SkFont.h"

SkiaRenderer::SkiaRenderer(QWidget* parent)
    : QWidget(parent)
    , m_skiaInitialized(false)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(400, 300);
}

SkiaRenderer::~SkiaRenderer() = default;

void SkiaRenderer::initializeSkia()
{
    if (m_skiaInitialized) {
        return;
    }

    const int width = this->width();
    const int height = this->height();
    
    if (width <= 0 || height <= 0) {
        return;
    }

    // 创建位图
    SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(width, height);
    if (!m_bitmap.tryAllocPixels(imageInfo)) {
        qWarning() << "Failed to allocate Skia bitmap";
        return;
    }

    // 创建Surface
    m_surface = SkSurfaces::WrapPixels(imageInfo, m_bitmap.getPixels(), m_bitmap.rowBytes());
    if (!m_surface) {
        qWarning() << "Failed to create Skia surface";
        return;
    }

    m_skiaInitialized = true;
    qDebug() << "Skia initialized successfully with size:" << width << "x" << height;
}

void SkiaRenderer::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    initializeSkia();
    
    if (!m_skiaInitialized || !m_surface) {
        // 如果Skia初始化失败，使用Qt绘制一个错误提示
        QPainter painter(this);
        painter.fillRect(rect(), Qt::red);
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Skia initialization failed");
        return;
    }

    // 使用Skia绘制
    SkCanvas* canvas = m_surface->getCanvas();
    if (canvas) {
        drawSkiaContent(canvas);
        // 对于CPU渲染的Surface，不需要调用flush
    }

    // 将Skia绘制的内容转换为QImage并用QPainter绘制到Widget上
    QImage qImage(
        reinterpret_cast<const uchar*>(m_bitmap.getPixels()),
        m_bitmap.width(),
        m_bitmap.height(),
        m_bitmap.rowBytes(),
        QImage::Format_ARGB32_Premultiplied
    );

    QPainter painter(this);
    painter.drawImage(0, 0, qImage);
}

void SkiaRenderer::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // 重置Skia初始化状态，强制重新初始化
    m_skiaInitialized = false;
    m_surface.reset();
    
    update();
}

void SkiaRenderer::drawSkiaContent(SkCanvas* canvas)
{
    // 清除画布
    canvas->clear(SK_ColorWHITE);

    // 创建画笔
    SkPaint paint;
    paint.setAntiAlias(true);

    // 绘制背景渐变
    SkPoint points[2] = {
        SkPoint::Make(0, 0),
        SkPoint::Make(width(), height())
    };
    SkColor colors[2] = { SK_ColorBLUE, SK_ColorCYAN };
    auto gradient = SkGradientShader::MakeLinear(points, colors, nullptr, 2, SkTileMode::kClamp);
    paint.setShader(gradient);
    canvas->drawRect(SkRect::MakeWH(width(), height()), paint);

    // 重置画笔
    paint.setShader(nullptr);
    paint.setColor(SK_ColorRED);
    paint.setStrokeWidth(3);
    paint.setStyle(SkPaint::kStroke_Style);

    // 绘制圆形
    canvas->drawCircle(width() / 4.0f, height() / 4.0f, 50, paint);

    // 绘制矩形
    paint.setColor(SK_ColorGREEN);
    paint.setStyle(SkPaint::kFill_Style);
    SkRect rect = SkRect::MakeXYWH(width() / 2.0f, height() / 2.0f, 100, 80);
    canvas->drawRect(rect, paint);

    // 绘制路径
    paint.setColor(SK_ColorMAGENTA);
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(5);
    
    SkPath path;
    path.moveTo(width() * 0.1f, height() * 0.8f);
    path.quadTo(width() * 0.5f, height() * 0.6f, width() * 0.9f, height() * 0.8f);
    canvas->drawPath(path, paint);

    // 绘制文本
    paint.setColor(SK_ColorBLACK);
    paint.setStyle(SkPaint::kFill_Style);
    
    SkFont font;
    font.setSize(24);
    
    const char* text = "Skia Integration Test";
    canvas->drawString(text, width() / 2.0f - 100, height() - 50, font, paint);
}
