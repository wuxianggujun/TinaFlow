#include "BgfxManager.hpp"
#include "BgfxResourceManager.hpp"
#include <QDebug>

BgfxManager& BgfxManager::instance()
{
    static BgfxManager instance;
    return instance;
}

BgfxManager::~BgfxManager()
{
    shutdown();
}

bool BgfxManager::initialize(void* windowHandle, uint32_t width, uint32_t height)
{
    // 如果已经初始化，检查是否需要重新初始化
    if (m_initialized) {
        if (m_currentWindowHandle == windowHandle &&
            m_currentWidth == width &&
            m_currentHeight == height) {
            qDebug() << "BgfxManager: Already initialized with same parameters, reusing";
            return true;
        } else {
            qDebug() << "BgfxManager: Parameters changed, shutting down and reinitializing";
            qDebug() << "BgfxManager: Old window:" << m_currentWindowHandle << "New window:" << windowHandle;
            shutdown();
        }
    }

    qDebug() << "BgfxManager: Initializing bgfx with resolution:" << width << "x" << height;
    
    bgfx::Init init;
    init.type = bgfx::RendererType::Count; // 自动选择最佳渲染器
    init.vendorId = BGFX_PCI_ID_NONE;
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_NONE;
    init.platformData.nwh = windowHandle;
    
    if (!bgfx::init(init)) {
        qCritical() << "BgfxManager: Failed to initialize bgfx";
        return false;
    }
    
    // 启用调试文本
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    
    m_initialized = true;
    m_currentWindowHandle = windowHandle;
    m_currentWidth = width;
    m_currentHeight = height;
    m_nextViewId = 0;

    // 重置视图ID使用状态
    for (int i = 0; i < MAX_VIEWS; ++i) {
        m_viewIds[i] = false;
    }

    qDebug() << "BgfxManager: bgfx initialized successfully!";
    qDebug() << "BgfxManager: Renderer:" << bgfx::getRendererName(bgfx::getRendererType());

    return true;
}

void BgfxManager::shutdown()
{
    if (!m_initialized) {
        return;
    }

    qDebug() << "BgfxManager: Shutting down bgfx";

    // 在关闭bgfx之前清理资源管理器
    BgfxResourceManager::instance().cleanup();

    bgfx::shutdown();
    m_initialized = false;
    m_currentWindowHandle = nullptr;
    m_currentWidth = 0;
    m_currentHeight = 0;

    qDebug() << "BgfxManager: bgfx shutdown complete";
}

void BgfxManager::reset(uint32_t width, uint32_t height)
{
    if (!m_initialized) {
        qWarning() << "BgfxManager: Cannot reset - bgfx not initialized";
        return;
    }
    
    if (bgfx::getInternalData()->context != nullptr) {
        bgfx::reset(width, height, BGFX_RESET_NONE);
        m_currentWidth = width;
        m_currentHeight = height;
        qDebug() << "BgfxManager: Reset to resolution:" << width << "x" << height;
    }
}



bgfx::ViewId BgfxManager::getNextViewId()
{
    if (!m_initialized) {
        qWarning() << "BgfxManager: Cannot allocate view ID - bgfx not initialized";
        return UINT16_MAX;
    }
    
    // 查找下一个可用的视图ID
    for (bgfx::ViewId i = 0; i < MAX_VIEWS; ++i) {
        if (!m_viewIds[i]) {
            m_viewIds[i] = true;
            qDebug() << "BgfxManager: Allocated view ID:" << i;
            return i;
        }
    }
    
    qWarning() << "BgfxManager: No available view IDs";
    return UINT16_MAX;
}

void BgfxManager::releaseViewId(bgfx::ViewId viewId)
{
    if (viewId < MAX_VIEWS) {
        m_viewIds[viewId] = false;
        qDebug() << "BgfxManager: Released view ID:" << viewId;
    }
}
