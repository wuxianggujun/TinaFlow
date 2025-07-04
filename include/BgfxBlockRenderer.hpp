#pragma once

#include <QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QPointF>
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <memory>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>
#include <bx/allocator.h>

#include "Block.hpp"

/**
 * @brief 基于bgfx的积木渲染器
 * 
 * 使用bgfx跨平台渲染库实现积木编程的可视化渲染，
 * 提供高性能的GPU加速渲染和现代图形特性。
 */
class BgfxBlockRenderer : public QWidget
{
    Q_OBJECT

public:
    explicit BgfxBlockRenderer(QWidget* parent = nullptr);
    ~BgfxBlockRenderer() override;

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

    // 视图控制
    void setZoom(float zoom);
    float getZoom() const { return m_zoom; }
    void setPan(const QPointF& pan);
    QPointF getPan() const { return m_pan; }

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    
    // 鼠标事件
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private slots:
    void onRenderTimer();

private:
    // 初始化方法
    bool initializeBgfx();
    void shutdownBgfx();
    void createSampleBlocks();
    
    // 渲染方法
    void render();
    void renderGrid();
    void renderBlocks();
    void renderBlock(std::shared_ptr<Block> block);
    void renderConnections();
    void renderConnection(std::shared_ptr<BlockConnection> connection);
    void renderText();
    
    // 几何渲染方法
    void renderRect(const QRectF& rect, const QColor& color, bool filled = true);
    void renderRoundedRect(const QRectF& rect, float radius, const QColor& color, bool filled = true);
    void renderCircle(const QPointF& center, float radius, const QColor& color, bool filled = true);
    void renderLine(const QPointF& start, const QPointF& end, const QColor& color, float width = 1.0f);
    void renderBezierCurve(const QPointF& start, const QPointF& end, 
                          const QPointF& control1, const QPointF& control2, 
                          const QColor& color, float width = 1.0f);
    
    // 坐标变换
    QPointF screenToWorld(const QPointF& screenPos) const;
    QPointF worldToScreen(const QPointF& worldPos) const;
    
    // bgfx资源
    bgfx::ViewId m_viewId = 0;
    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    bgfx::VertexBufferHandle m_vertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_indexBuffer = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_colorUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_transformUniform = BGFX_INVALID_HANDLE;
    
    // 积木数据
    QVector<std::shared_ptr<Block>> m_blocks;
    QVector<std::shared_ptr<BlockConnection>> m_connections;
    
    // 视图状态
    float m_zoom = 1.0f;
    QPointF m_pan = QPointF(0, 0);
    float m_viewMatrix[16];
    float m_projMatrix[16];
    
    // 交互状态
    bool m_isDragging = false;
    bool m_isConnecting = false;
    bool m_isPanning = false;
    QPointF m_lastMousePos;
    QString m_draggedBlockId;
    QString m_connectionSourceBlockId;
    QString m_connectionSourcePointId;
    QPointF m_connectionEndPos;
    
    // 渲染设置
    QColor m_backgroundColor = QColor(245, 245, 245);
    QColor m_gridColor = QColor(200, 200, 200, 100);
    float m_gridSize = 20.0f;
    
    // 渲染定时器
    QTimer m_renderTimer;
    bool m_bgfxInitialized = false;
    
    // 文本渲染
    QFont m_font;
    QFontMetrics m_fontMetrics;
    
    // 窗口句柄
    void* m_nativeHandle = nullptr;
};
