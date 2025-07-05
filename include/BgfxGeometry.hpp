#pragma once

#include <bgfx/bgfx.h>
#include <QDebug>
#include <vector>
#include <memory>

/**
 * @brief RAII封装的bgfx几何体资源管理类
 */
class BgfxGeometry
{
public:
    BgfxGeometry() = default;
    
    // 禁用拷贝构造和赋值
    BgfxGeometry(const BgfxGeometry&) = delete;
    BgfxGeometry& operator=(const BgfxGeometry&) = delete;
    
    // 支持移动构造和赋值
    BgfxGeometry(BgfxGeometry&& other) noexcept
        : m_vertexBuffer(other.m_vertexBuffer)
        , m_indexBuffer(other.m_indexBuffer)
        , m_vertexCount(other.m_vertexCount)
        , m_indexCount(other.m_indexCount)
    {
        other.m_vertexBuffer = BGFX_INVALID_HANDLE;
        other.m_indexBuffer = BGFX_INVALID_HANDLE;
        other.m_vertexCount = 0;
        other.m_indexCount = 0;
    }
    
    BgfxGeometry& operator=(BgfxGeometry&& other) noexcept
    {
        if (this != &other) {
            cleanup();
            
            m_vertexBuffer = other.m_vertexBuffer;
            m_indexBuffer = other.m_indexBuffer;
            m_vertexCount = other.m_vertexCount;
            m_indexCount = other.m_indexCount;
            
            other.m_vertexBuffer = BGFX_INVALID_HANDLE;
            other.m_indexBuffer = BGFX_INVALID_HANDLE;
            other.m_vertexCount = 0;
            other.m_indexCount = 0;
        }
        return *this;
    }
    
    ~BgfxGeometry()
    {
        cleanup();
    }
    
    /**
     * @brief 创建几何体资源
     */
    bool create(const void* vertexData, uint32_t vertexSize, uint32_t vertexCount,
                const void* indexData, uint32_t indexSize, uint32_t indexCount,
                const bgfx::VertexLayout& layout)
    {
        safeCleanup(); // 安全清理旧资源

        // 创建顶点缓冲区
        m_vertexBuffer = bgfx::createVertexBuffer(
            bgfx::makeRef(vertexData, vertexSize),
            layout
        );

        // 创建索引缓冲区
        m_indexBuffer = bgfx::createIndexBuffer(
            bgfx::makeRef(indexData, indexSize)
        );

        if (bgfx::isValid(m_vertexBuffer) && bgfx::isValid(m_indexBuffer)) {
            m_vertexCount = vertexCount;
            m_indexCount = indexCount;
            return true;
        } else {
            safeCleanup();
            return false;
        }
    }
    
    /**
     * @brief 检查资源是否有效
     */
    bool isValid() const
    {
        return bgfx::isValid(m_vertexBuffer) && bgfx::isValid(m_indexBuffer);
    }
    
    /**
     * @brief 绑定几何体到渲染管线
     */
    void bind() const
    {
        if (isValid()) {
            bgfx::setVertexBuffer(0, m_vertexBuffer);
            bgfx::setIndexBuffer(m_indexBuffer);
        }
    }
    
    /**
     * @brief 获取顶点数量
     */
    uint32_t getVertexCount() const { return m_vertexCount; }
    
    /**
     * @brief 获取索引数量
     */
    uint32_t getIndexCount() const { return m_indexCount; }

    /**
     * @brief 标记资源为无效（当bgfx重新初始化时调用）
     */
    void invalidateResources()
    {
        m_vertexBuffer = BGFX_INVALID_HANDLE;
        m_indexBuffer = BGFX_INVALID_HANDLE;
        m_vertexCount = 0;
        m_indexCount = 0;
    }

private:
    void cleanup()
    {
        if (bgfx::isValid(m_vertexBuffer)) {
            bgfx::destroy(m_vertexBuffer);
            m_vertexBuffer = BGFX_INVALID_HANDLE;
        }

        if (bgfx::isValid(m_indexBuffer)) {
            bgfx::destroy(m_indexBuffer);
            m_indexBuffer = BGFX_INVALID_HANDLE;
        }

        m_vertexCount = 0;
        m_indexCount = 0;
    }

    void safeCleanup()
    {
        // 检查bgfx是否仍然有效，避免在重新初始化时崩溃
        if (bgfx::getRendererType() != bgfx::RendererType::Noop) {
            cleanup();
        } else {
            // bgfx已经关闭，只需要重置句柄
            invalidateResources();
        }
    }

private:
    bgfx::VertexBufferHandle m_vertexBuffer = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_indexBuffer = BGFX_INVALID_HANDLE;
    uint32_t m_vertexCount = 0;
    uint32_t m_indexCount = 0;
};

/**
 * @brief 积木实例数据
 */
struct BlockInstance
{
    float x, y, z;          // 位置
    uint32_t color;         // 颜色
    int connectorType;      // 连接器类型 (0=无, 1=凸起, -1=凹陷)

    BlockInstance(float x = 0.0f, float y = 0.0f, float z = 0.0f,
                  uint32_t color = 0xffffffff, int connectorType = 0)
        : x(x), y(y), z(z), color(color), connectorType(connectorType)
    {}
};

/**
 * @brief bgfx实例数据结构（用于实例化渲染）
 */
struct InstanceData
{
    float mtx[16];      // 变换矩阵
    float color[4];     // 颜色 (RGBA)
    float config[4];    // 连接器配置
};

/**
 * @brief 积木几何体管理器
 */
class BlockGeometryManager
{
public:
    BlockGeometryManager() = default;
    ~BlockGeometryManager() = default;
    
    /**
     * @brief 初始化几何体资源
     */
    bool initialize(const bgfx::VertexLayout& layout);
    
    /**
     * @brief 添加积木实例
     */
    void addBlock(const BlockInstance& instance);
    
    /**
     * @brief 清除所有积木实例
     */
    void clearBlocks();
    
    /**
     * @brief 渲染所有积木
     */
    void render(bgfx::ViewId viewId, bgfx::ProgramHandle program,
                bgfx::UniformHandle roundedParamsUniform,
                bgfx::UniformHandle connectorConfigUniform,
                const float* baseTransform);

    /**
     * @brief 标记所有几何体资源为无效（当bgfx重新初始化时调用）
     */
    void invalidateResources();

private:
    BgfxGeometry m_connectorGeometry;   // 凸起积木几何体
    BgfxGeometry m_receptorGeometry;    // 凹陷积木几何体
    BgfxGeometry m_simpleGeometry;      // 简单积木几何体
    
    std::vector<BlockInstance> m_blocks; // 积木实例列表
    
    // 渲染状态缓存
    uint64_t m_renderState = BGFX_STATE_WRITE_RGB
                           | BGFX_STATE_WRITE_A
                           | BGFX_STATE_WRITE_Z
                           | BGFX_STATE_DEPTH_TEST_LESS
                           | BGFX_STATE_BLEND_ALPHA;
};
