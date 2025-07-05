#include "BgfxGeometry.hpp"
#include "BgfxVertexTypes.hpp"
#include <bx/math.h>
#include <cstring> // for memcpy

bool BlockGeometryManager::initialize(const bgfx::VertexLayout& layout)
{
    // 确保顶点布局已初始化
    PosColorTexVertex::init();

    // 积木尺寸定义
    const float blockWidth = 120.0f;
    const float blockHeight = 40.0f;
    
    // 创建凸起积木几何体
    {
        const uint32_t mainColor = 0xffe2904a; // 蓝色主体
        
        static PosColorTexVertex vertices[] = {
            // 主体矩形
            {-blockWidth/2, -blockHeight/2, 0.0f, mainColor, -1.0f, -1.0f},
            { blockWidth/2, -blockHeight/2, 0.0f, mainColor,  1.0f, -1.0f},
            { blockWidth/2,  blockHeight/2, 0.0f, mainColor,  1.0f,  1.0f},
            {-blockWidth/2,  blockHeight/2, 0.0f, mainColor, -1.0f,  1.0f},
            
            // 左连接器
            {-blockWidth/4 - 6.0f,  blockHeight/2, 0.0f, mainColor, -0.2f,  0.95f},
            {-blockWidth/4 + 6.0f,  blockHeight/2, 0.0f, mainColor,  0.2f,  0.95f},
            {-blockWidth/4 + 6.0f,  blockHeight/2 + 4.0f, 0.0f, mainColor,  0.2f,  1.5f},
            {-blockWidth/4 - 6.0f,  blockHeight/2 + 4.0f, 0.0f, mainColor, -0.2f,  1.5f},
            
            // 右连接器
            { blockWidth/4 - 6.0f,  blockHeight/2, 0.0f, mainColor, -0.2f,  0.95f},
            { blockWidth/4 + 6.0f,  blockHeight/2, 0.0f, mainColor,  0.2f,  0.95f},
            { blockWidth/4 + 6.0f,  blockHeight/2 + 4.0f, 0.0f, mainColor,  0.2f,  1.5f},
            { blockWidth/4 - 6.0f,  blockHeight/2 + 4.0f, 0.0f, mainColor, -0.2f,  1.5f},
        };
        
        static uint16_t indices[] = {
            0, 1, 2,  2, 3, 0,    // 主体
            4, 5, 6,  6, 7, 4,    // 左连接器
            8, 9, 10, 10, 11, 8,  // 右连接器
        };
        
        if (!m_connectorGeometry.create(vertices, sizeof(vertices), 12,
                                       indices, sizeof(indices), 18, PosColorTexVertex::ms_layout)) {
            qWarning() << "Failed to create connector geometry";
            return false;
        }
    }
    
    // 创建凹陷积木几何体 (简单矩形)
    {
        const uint32_t mainColor = 0xff4ae290; // 绿色主体
        
        static PosColorTexVertex vertices[] = {
            {-blockWidth/2, -blockHeight/2, 0.0f, mainColor, -1.0f, -1.0f},
            { blockWidth/2, -blockHeight/2, 0.0f, mainColor,  1.0f, -1.0f},
            { blockWidth/2,  blockHeight/2, 0.0f, mainColor,  1.0f,  1.0f},
            {-blockWidth/2,  blockHeight/2, 0.0f, mainColor, -1.0f,  1.0f},
        };
        
        static uint16_t indices[] = {
            0, 1, 2,  2, 3, 0,
        };
        
        if (!m_receptorGeometry.create(vertices, sizeof(vertices), 4,
                                      indices, sizeof(indices), 6, PosColorTexVertex::ms_layout)) {
            qWarning() << "Failed to create receptor geometry";
            return false;
        }
    }
    
    // 创建简单积木几何体
    {
        const uint32_t mainColor = 0xffffff90; // 黄色主体
        
        static PosColorTexVertex vertices[] = {
            {-blockWidth/2, -blockHeight/2, 0.0f, mainColor, -1.0f, -1.0f},
            { blockWidth/2, -blockHeight/2, 0.0f, mainColor,  1.0f, -1.0f},
            { blockWidth/2,  blockHeight/2, 0.0f, mainColor,  1.0f,  1.0f},
            {-blockWidth/2,  blockHeight/2, 0.0f, mainColor, -1.0f,  1.0f},
        };
        
        static uint16_t indices[] = {
            0, 1, 2,  2, 3, 0,
        };
        
        if (!m_simpleGeometry.create(vertices, sizeof(vertices), 4,
                                    indices, sizeof(indices), 6, PosColorTexVertex::ms_layout)) {
            qWarning() << "Failed to create simple geometry";
            return false;
        }
    }

    // 创建选择边框几何体（填充边框）
    {
        const uint32_t borderColor = 0xffffff00; // 黄色边框
        const float borderWidth = 3.0f; // 增加边框宽度
        const float outerHalfWidth = blockWidth * 0.5f + borderWidth;
        const float outerHalfHeight = blockHeight * 0.5f + borderWidth;
        const float innerHalfWidth = blockWidth * 0.5f;
        const float innerHalfHeight = blockHeight * 0.5f;

        // 创建边框顶点（8个顶点，形成边框矩形）
        static PosColorTexVertex vertices[] = {
            // 外边框的4个顶点
            {-outerHalfWidth, -outerHalfHeight, 0.0f, borderColor, 0.0f, 0.0f},
            { outerHalfWidth, -outerHalfHeight, 0.0f, borderColor, 1.0f, 0.0f},
            { outerHalfWidth,  outerHalfHeight, 0.0f, borderColor, 1.0f, 1.0f},
            {-outerHalfWidth,  outerHalfHeight, 0.0f, borderColor, 0.0f, 1.0f},
            // 内边框的4个顶点
            {-innerHalfWidth, -innerHalfHeight, 0.0f, borderColor, 0.25f, 0.25f},
            { innerHalfWidth, -innerHalfHeight, 0.0f, borderColor, 0.75f, 0.25f},
            { innerHalfWidth,  innerHalfHeight, 0.0f, borderColor, 0.75f, 0.75f},
            {-innerHalfWidth,  innerHalfHeight, 0.0f, borderColor, 0.25f, 0.75f},
        };

        // 边框索引（8个三角形，形成边框）
        static uint16_t indices[] = {
            // 上边框
            0, 1, 5,  0, 5, 4,
            // 右边框
            1, 2, 6,  1, 6, 5,
            // 下边框
            2, 3, 7,  2, 7, 6,
            // 左边框
            3, 0, 4,  3, 4, 7
        };

        if (!m_selectionBorder.create(vertices, sizeof(vertices), 8,
                                     indices, sizeof(indices), 24, PosColorTexVertex::ms_layout)) {
            qWarning() << "Failed to create selection border geometry";
            return false;
        }
    }

    // 减少日志输出
    // qDebug() << "BlockGeometryManager: All geometries created successfully";
    return true;
}

void BlockGeometryManager::addBlock(const BlockInstance& instance)
{
    BlockInstance newInstance = instance;
    // 如果没有指定ID，自动分配一个
    if (newInstance.blockId == -1) {
        newInstance.blockId = m_nextBlockId++;
    } else {
        // 更新下一个ID，确保不重复
        m_nextBlockId = std::max(m_nextBlockId, newInstance.blockId + 1);
    }

    m_blocks.push_back(newInstance);
    // 减少日志输出 - 只在调试时启用
    // qDebug() << "BlockGeometryManager: Added block at (" << instance.x << "," << instance.y
    //          << ") type:" << instance.connectorType << "total blocks:" << m_blocks.size();
}

void BlockGeometryManager::clearBlocks()
{
    m_blocks.clear();
    m_nextBlockId = 0; // 重置ID计数器
}

void BlockGeometryManager::render(bgfx::ViewId viewId, bgfx::ProgramHandle program,
                                 bgfx::UniformHandle roundedParamsUniform,
                                 bgfx::UniformHandle connectorConfigUniform,
                                 const float* baseTransform)
{
    if (!bgfx::isValid(program)) {
        return;
    }
    
    // 渲染所有积木实例
    // 减少日志输出
    // static bool logged = false;
    // if (!logged) {
    //     qDebug() << "BlockGeometryManager::render: Rendering" << m_blocks.size() << "blocks";
    //     logged = true;
    // }

    for (const auto& block : m_blocks) {
        // 创建积木自身的平移矩阵
        float blockTransform[16];
        bx::mtxTranslate(blockTransform, block.x, block.y, block.z);
    
        // 正确的组合方式：将视图变换(baseTransform)应用到积木的变换(blockTransform)上
        float finalTransform[16];
        bx::mtxMul(finalTransform, blockTransform, baseTransform); // 注意这里的顺序

        // 将最终的组合矩阵设置给bgfx
        bgfx::setTransform(finalTransform);
        
        // 根据连接器类型选择几何体和配置
        BgfxGeometry* geometry = nullptr;
        float connectorConfig[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        
        switch (block.connectorType) {
            case 1: // 凸起
                geometry = &m_connectorGeometry;
                connectorConfig[0] = 1.0f; // top=1
                break;
            case -1: // 凹陷
                geometry = &m_receptorGeometry;
                connectorConfig[1] = -1.0f; // bottom=-1
                break;
            default: // 简单
                geometry = &m_simpleGeometry;
                break;
        }
        
        if (geometry && geometry->isValid()) {
            // 设置渲染状态 (每个积木都需要重新设置)
            bgfx::setState(m_renderState);

            // 绑定几何体
            geometry->bind();

            // 设置圆角参数
            if (bgfx::isValid(roundedParamsUniform)) {
                float roundedParams[4] = {120.0f, 40.0f, 8.0f, 0.0f};
                bgfx::setUniform(roundedParamsUniform, roundedParams);
            }

            // 设置连接器配置
            if (bgfx::isValid(connectorConfigUniform)) {
                bgfx::setUniform(connectorConfigUniform, connectorConfig);
            }

            // 提交渲染
            bgfx::submit(viewId, program);

            // 减少日志输出
            // static int submitCount = 0;
            // submitCount++;
            // if (submitCount <= 10) { // 只打印前10次
            //     qDebug() << "BlockGeometryManager: Submitted block" << submitCount
            //              << "at (" << block.x << "," << block.y << ") type:" << block.connectorType;
            // }
        }
    }

    // 渲染选中积木的边框
    if (m_selectionBorder.isValid()) {
        for (const auto& block : m_blocks) {
            if (block.isSelected) {
                // 创建积木自身的平移矩阵
                float blockTransform[16];
                bx::mtxTranslate(blockTransform, block.x, block.y, block.z + 0.01f); // 稍微提高Z值避免Z-fighting

                // 组合变换矩阵
                float finalTransform[16];
                bx::mtxMul(finalTransform, blockTransform, baseTransform);

                // 设置变换矩阵
                bgfx::setTransform(finalTransform);

                // 设置边框渲染状态（填充模式）
                uint64_t borderState = BGFX_STATE_WRITE_RGB
                                     | BGFX_STATE_WRITE_A
                                     | BGFX_STATE_WRITE_Z
                                     | BGFX_STATE_DEPTH_TEST_LESS
                                     | BGFX_STATE_BLEND_ALPHA; // 移除线框模式
                bgfx::setState(borderState);

                // 绑定边框几何体
                m_selectionBorder.bind();

                // 设置圆角参数（边框不需要圆角）
                if (bgfx::isValid(roundedParamsUniform)) {
                    float roundedParams[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                    bgfx::setUniform(roundedParamsUniform, roundedParams);
                }

                // 设置连接器配置（边框不需要连接器）
                if (bgfx::isValid(connectorConfigUniform)) {
                    float connectorConfig[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                    bgfx::setUniform(connectorConfigUniform, connectorConfig);
                }

                // 提交边框渲染
                bgfx::submit(viewId, program);
            }
        }
    }
}

void BlockGeometryManager::invalidateResources()
{
    m_connectorGeometry.invalidateResources();
    m_receptorGeometry.invalidateResources();
    m_simpleGeometry.invalidateResources();
    m_selectionBorder.invalidateResources();
}

BlockInstance* BlockGeometryManager::findBlockAt(float worldX, float worldY)
{
    // 从后往前遍历，优先选择最后绘制的（最上层的）积木
    for (auto it = m_blocks.rbegin(); it != m_blocks.rend(); ++it) {
        if (it->containsPoint(worldX, worldY)) {
            qDebug() << "Found block" << it->blockId << "at (" << it->x << "," << it->y << ")";
            return &(*it);
        }
    }
    return nullptr;
}

BlockInstance* BlockGeometryManager::getBlockById(int blockId)
{
    auto it = std::find_if(m_blocks.begin(), m_blocks.end(),
                          [blockId](const BlockInstance& block) {
                              return block.blockId == blockId;
                          });
    return (it != m_blocks.end()) ? &(*it) : nullptr;
}

void BlockGeometryManager::setBlockSelected(int blockId, bool selected)
{
    BlockInstance* block = getBlockById(blockId);
    if (block) {
        qDebug() << "BlockGeometryManager: Setting block" << blockId << "selected =" << selected;
        block->setSelected(selected);
    } else {
        qDebug() << "BlockGeometryManager: Block" << blockId << "not found for selection";
    }
}

void BlockGeometryManager::clearSelection()
{
    for (auto& block : m_blocks) {
        block.setSelected(false);
    }
}

std::vector<int> BlockGeometryManager::getSelectedBlocks() const
{
    std::vector<int> selectedIds;
    for (const auto& block : m_blocks) {
        if (block.isSelected) {
            selectedIds.push_back(block.blockId);
        }
    }
    return selectedIds;
}

void BlockGeometryManager::moveBlock(int blockId, float newX, float newY)
{
    BlockInstance* block = getBlockById(blockId);
    if (block) {
        block->x = newX;
        block->y = newY;
    }
}
