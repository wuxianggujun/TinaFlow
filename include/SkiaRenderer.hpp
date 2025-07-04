#pragma once

#include <QWidget>
#include <QTimer>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QHideEvent>

// Skia CPU 渲染 API
#include "include/core/SkSurface.h"
#include "include/core/SkPath.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"

/**
 * @brief Skia CPU渲染器，避免OpenGL兼容性问题
 */
class SkiaRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit SkiaRenderer(QWidget* parent = nullptr);
    ~SkiaRenderer() override;

protected:
    // Qt Widget lifecycle
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private slots:
    void updateAnimation();

private:
    // helper methods
    SkPath makePuzzlePath(const QRectF& r, qreal notchW = 8, qreal notchH = 4);
    void drawBlockProgrammingContent(SkCanvas* canvas);
    void initializeSkia(); // 初始化Skia CPU渲染

    // Skia objects - CPU渲染
    sk_sp<SkSurface> m_surface;
    uint8_t* m_skiaImage; // 图像缓冲区

    // Animation
    QTimer* m_animationTimer;
    float m_animationTime;
};
