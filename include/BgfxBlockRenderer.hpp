#pragma once

#include "BgfxWidget.hpp"
#include "BgfxGeometry.hpp"
#include "BgfxResourceManager.hpp"
#include <bgfx/bgfx.h>

/**
 * @brief 基于bgfx的块编程渲染器
 *
 * 继承自BgfxWidget，专注于块编程的可视化渲染。
 * 只需要实现具体的绘制逻辑，不需要管理bgfx的初始化等。
 */
class BgfxBlockRenderer : public BgfxWidget
{
    Q_OBJECT

public:
    explicit BgfxBlockRenderer(QWidget* parent = nullptr);
    ~BgfxBlockRenderer() override;

    // 积木管理接口
    void addBlock(float x, float y, int connectorType = 0, uint32_t color = 0xffffffff);
    void clearBlocks();
    void createTestBlocks(); // 创建测试用的积木
    void updateBlockPositions(); // 更新积木位置（缩放变化时调用）

    // 重写缩放方法以更新积木位置
    void setZoom(float zoom) override;

    // 积木选择和交互接口
    void selectBlock(int blockId, bool multiSelect = false);
    void clearSelection();
    std::vector<int> getSelectedBlocks() const;
    void moveSelectedBlocks(float deltaX, float deltaY);

    // 视图控制
    void resetView(); // 重置视图到积木可见的状态

protected:
    // 重写基类的渲染方法
    void render() override;

    // 重写基类的资源管理方法
    void initializeResources() override;
    void cleanupResources() override;

    // 处理bgfx重新初始化
    void onBgfxReset();

    // 重写鼠标事件处理
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    // 几何体管理器
    BlockGeometryManager m_geometryManager;

    // 顶点布局
    bgfx::VertexLayout m_vertexLayout;

    // 交互状态
    bool m_isBlockDragging = false;        // 是否正在拖动积木
    std::vector<int> m_draggingBlocks;     // 正在拖动的积木ID列表
    QPointF m_dragStartPos;                // 拖动开始位置（屏幕坐标）
    QPointF m_dragLastPos;                 // 上次拖动位置（屏幕坐标）
};
