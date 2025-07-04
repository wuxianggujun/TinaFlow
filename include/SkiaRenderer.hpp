#pragma once
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QSurfaceFormat>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QDebug>
#include <QPointF>
#include <QRectF>
#include <QColor>
#include <QString>
#include <QVector>
#include <memory>

// Skia
#include "include/core/SkCanvas.h"
#include "include/core/SkSurface.h"
#include "include/gpu/ganesh/GrDirectContext.h"
#include "include/gpu/ganesh/gl/GrGLInterface.h"
#include "include/gpu/ganesh/gl/GrGLTypes.h"
#include "include/gpu/ganesh/SkSurfaceGanesh.h"

// 包含积木相关头文件
#include "Block.hpp"

/**
 * @brief 基于工作示例的Skia渲染器，支持积木编程
 */
class SkiaRenderer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit SkiaRenderer(QWidget* parent = nullptr);
    ~SkiaRenderer() override;

    // 积木管理
    void addBlock(std::shared_ptr<Block> block);
    void removeBlock(const QString& blockId);
    std::shared_ptr<Block> getBlock(const QString& blockId);
    const QVector<std::shared_ptr<Block>>& getBlocks() const { return m_blocks; }

    // 连接管理
    void addConnection(std::shared_ptr<BlockConnection> connection);
    void removeConnection(const QString& connectionId);
    std::shared_ptr<BlockConnection> getConnection(const QString& connectionId);
    const QVector<std::shared_ptr<BlockConnection>>& getConnections() const { return m_connections; }

    // 选择管理
    void selectBlock(const QString& blockId);
    void deselectAll();
    QVector<QString> getSelectedBlockIds() const;

    // 工具方法
    std::shared_ptr<Block> getBlockAt(const QPointF& position);
    ConnectionPoint* getConnectionPointAt(const QPointF& position);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void resizeEvent(QResizeEvent* event) override;

    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;

    // 鼠标事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void onTick();

private:
    void createSkiaSurface();
    void drawBlockProgrammingContent(SkCanvas* canvas);

    // 积木绘制方法
    void drawBlock(SkCanvas* canvas, std::shared_ptr<Block> block);
    void drawConnectionPoint(SkCanvas* canvas, const ConnectionPoint& point, const QPointF& blockPos);
    void drawConnection(SkCanvas* canvas, std::shared_ptr<BlockConnection> connection);
    void drawGrid(SkCanvas* canvas);
    void drawSelectionBox(SkCanvas* canvas);

    // 示例积木创建
    void createSampleBlocks();

    sk_sp<GrDirectContext> skContext;
    sk_sp<SkSurface> skSurface;

    // 动画
    QTimer m_animationTimer;
    float m_animationTime;

    // 积木数据
    QVector<std::shared_ptr<Block>> m_blocks;
    QVector<std::shared_ptr<BlockConnection>> m_connections;

    // 交互状态
    bool m_isDragging = false;
    bool m_isConnecting = false;
    QPointF m_lastMousePos;
    QString m_draggedBlockId;
    QString m_connectionSourceBlockId;
    QString m_connectionSourcePointId;
    QPointF m_connectionEndPos;
};
