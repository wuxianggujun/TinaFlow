#pragma once

#include <bgfx/bgfx.h>
#include <QDebug>
#include <vector>
#include <memory>
#include <algorithm>

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

    // 选择和交互状态
    bool isSelected = false;        // 是否被选中
    bool isDragging = false;        // 是否正在拖动
    int blockId = -1;              // 积木唯一ID

    // 边界框信息 (相对于积木中心的偏移)
    float width = 120.0f;          // 积木宽度
    float height = 40.0f;          // 积木高度

    // 原始颜色（用于恢复选择状态）
    uint32_t originalColor;

    BlockInstance(float x = 0.0f, float y = 0.0f, float z = 0.0f,
                  uint32_t color = 0xffffffff, int connectorType = 0, int id = -1)
        : x(x), y(y), z(z), color(color), connectorType(connectorType),
          blockId(id), originalColor(color)
    {}

    /**
     * @brief 检查点是否在积木边界内
     * @param px 点的x坐标（世界坐标）
     * @param py 点的y坐标（世界坐标）
     * @return 如果点在积木内返回true
     */
    bool containsPoint(float px, float py) const {
        float halfWidth = width * 0.5f;
        float halfHeight = height * 0.5f;
        return (px >= x - halfWidth && px <= x + halfWidth &&
                py >= y - halfHeight && py <= y + halfHeight);
    }

    /**
     * @brief 设置选择状态
     * @param selected 是否选中
     */
    void setSelected(bool selected) {
        isSelected = selected;
        if (selected) {
            // 选中时使用高亮颜色（增加亮度）
            color = brightenColor(originalColor);
        } else {
            // 取消选中时恢复原始颜色
            color = originalColor;
        }
    }

private:
    /**
     * @brief 增加颜色亮度
     */
    uint32_t brightenColor(uint32_t originalColor) const {
        uint8_t r = (originalColor >> 24) & 0xFF;
        uint8_t g = (originalColor >> 16) & 0xFF;
        uint8_t b = (originalColor >> 8) & 0xFF;
        uint8_t a = originalColor & 0xFF;

        // 增加亮度（限制在255以内）
        r = std::min(255, static_cast<int>(r * 1.3f));
        g = std::min(255, static_cast<int>(g * 1.3f));
        b = std::min(255, static_cast<int>(b * 1.3f));

        return (r << 24) | (g << 16) | (b << 8) | a;
    }
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

    /**
     * @brief 根据世界坐标查找积木
     * @param worldX 世界坐标X
     * @param worldY 世界坐标Y
     * @return 找到的积木指针，如果没找到返回nullptr
     */
    BlockInstance* findBlockAt(float worldX, float worldY);

    /**
     * @brief 获取指定ID的积木
     * @param blockId 积木ID
     * @return 积木指针，如果没找到返回nullptr
     */
    BlockInstance* getBlockById(int blockId);

    /**
     * @brief 设置积木选择状态
     * @param blockId 积木ID
     * @param selected 是否选中
     */
    void setBlockSelected(int blockId, bool selected);

    /**
     * @brief 清除所有积木的选择状态
     */
    void clearSelection();

    /**
     * @brief 获取所有选中的积木
     * @return 选中积木的ID列表
     */
    std::vector<int> getSelectedBlocks() const;

    /**
     * @brief 移动积木位置
     * @param blockId 积木ID
     * @param newX 新的X坐标
     * @param newY 新的Y坐标
     */
    void moveBlock(int blockId, float newX, float newY);

    /**
     * @brief 获取积木数量
     */
    size_t getBlockCount() const { return m_blocks.size(); }

    /**
     * @brief 获取积木列表（只读）
     */
    const std::vector<BlockInstance>& getBlocks() const { return m_blocks; }

private:
    BgfxGeometry m_connectorGeometry;   // 凸起积木几何体
    BgfxGeometry m_receptorGeometry;    // 凹陷积木几何体
    BgfxGeometry m_simpleGeometry;      // 简单积木几何体
    BgfxGeometry m_selectionBorder;     // 选择边框几何体

    std::vector<BlockInstance> m_blocks; // 积木实例列表
    int m_nextBlockId = 0;              // 下一个积木ID

    // 渲染状态缓存
    uint64_t m_renderState = BGFX_STATE_WRITE_RGB
                           | BGFX_STATE_WRITE_A
                           | BGFX_STATE_WRITE_Z
                           | BGFX_STATE_DEPTH_TEST_LESS
                           | BGFX_STATE_BLEND_ALPHA;
};
