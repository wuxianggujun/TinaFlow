#pragma once

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <QDebug>

/**
 * @brief bgfx全局管理器
 * 
 * 确保bgfx只初始化一次，并提供全局的bgfx状态管理
 */
class BgfxManager
{
public:
    static BgfxManager& instance();
    
    // 初始化bgfx（如果还没有初始化）
    bool initialize(void* windowHandle, uint32_t width, uint32_t height);
    
    // 关闭bgfx
    void shutdown();
    
    // 检查是否已初始化
    bool isInitialized() const { return m_initialized; }
    
    // 重置分辨率
    void reset(uint32_t width, uint32_t height);

    // 获取当前窗口句柄
    void* getCurrentWindowHandle() const { return m_currentWindowHandle; }

    // 获取下一个可用的视图ID
    bgfx::ViewId getNextViewId();
    
    // 释放视图ID
    void releaseViewId(bgfx::ViewId viewId);

private:
    BgfxManager() = default;
    ~BgfxManager();
    
    // 禁止拷贝
    BgfxManager(const BgfxManager&) = delete;
    BgfxManager& operator=(const BgfxManager&) = delete;
    
    bool m_initialized = false;
    void* m_currentWindowHandle = nullptr;
    uint32_t m_currentWidth = 0;
    uint32_t m_currentHeight = 0;
    bgfx::ViewId m_nextViewId = 0;
    static constexpr bgfx::ViewId MAX_VIEWS = 255;
    bool m_viewIds[MAX_VIEWS] = {false};
};
