#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <memory>

// Skia includes
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkBitmap.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPath.h"
#include "include/core/SkColor.h"

/**
 * @brief Skia渲染器类，用于在Qt Widget中使用Skia进行绘制
 */
class SkiaRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit SkiaRenderer(QWidget* parent = nullptr);
    ~SkiaRenderer() override;

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void initializeSkia();
    void drawSkiaContent(SkCanvas* canvas);
    
    sk_sp<SkSurface> m_surface;
    SkBitmap m_bitmap;
    bool m_skiaInitialized;
};
