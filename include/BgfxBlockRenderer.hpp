#pragma once

#include "BgfxWidget.hpp"
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

protected:
    // 重写基类的渲染方法
    void render() override;

    // 重写基类的资源管理方法
    void initializeResources() override;
    void cleanupResources() override;

private:
    // 渲染方法
    void renderDebugInfo();
    void renderTestGeometry();

    // bgfx资源
    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;

    // 凸起积木资源
    bgfx::VertexBufferHandle m_connectorVertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_connectorIndexBuffer = BGFX_INVALID_HANDLE;

    // 凹陷积木资源
    bgfx::VertexBufferHandle m_receptorVertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_receptorIndexBuffer = BGFX_INVALID_HANDLE;

    bgfx::VertexLayout m_vertexLayout;

    // 圆角着色器的uniform句柄
    bgfx::UniformHandle m_roundedParamsUniform = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_connectorConfigUniform = BGFX_INVALID_HANDLE;
};
