#pragma once

#include "BgfxWidget.hpp"
#include "BgfxGeometry.hpp"
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

protected:
    // 重写基类的渲染方法
    void render() override;

    // 重写基类的资源管理方法
    void initializeResources() override;
    void cleanupResources() override;

private:

    // bgfx资源
    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout m_vertexLayout;

    // 圆角着色器的uniform句柄
    bgfx::UniformHandle m_roundedParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_connectorConfigUniform = BGFX_INVALID_HANDLE;

    // 几何体管理器
    BlockGeometryManager m_geometryManager;
};
