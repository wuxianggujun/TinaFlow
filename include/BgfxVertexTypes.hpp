#pragma once

#include <bgfx/bgfx.h>
#include <cstdint>

/**
 * @brief 包含位置、颜色和纹理坐标的顶点结构
 */
struct PosColorTexVertex
{
    float x, y, z;
    uint32_t abgr;
    float u, v;

    static void init()
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
    }

    static bgfx::VertexLayout ms_layout;
};

/**
 * @brief 包含位置和颜色的简单顶点结构
 */
struct PosColorVertex 
{
    float x, y, z;
    uint32_t abgr;

    static void init() 
    {
        ms_layout
            .begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();
    }

    static bgfx::VertexLayout ms_layout;
};
