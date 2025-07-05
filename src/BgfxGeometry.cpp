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
    
    // 减少日志输出
    // qDebug() << "BlockGeometryManager: All geometries created successfully";
    return true;
}

void BlockGeometryManager::addBlock(const BlockInstance& instance)
{
    m_blocks.push_back(instance);
    // 减少日志输出 - 只在调试时启用
    // qDebug() << "BlockGeometryManager: Added block at (" << instance.x << "," << instance.y
    //          << ") type:" << instance.connectorType << "total blocks:" << m_blocks.size();
}

void BlockGeometryManager::clearBlocks()
{
    m_blocks.clear();
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
}

void BlockGeometryManager::invalidateResources()
{
    m_connectorGeometry.invalidateResources();
    m_receptorGeometry.invalidateResources();
    m_simpleGeometry.invalidateResources();
}
