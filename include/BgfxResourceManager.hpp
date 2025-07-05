#pragma once

#include <bgfx/bgfx.h>
#include <QDebug>
#include <unordered_map>
#include <string>

/**
 * @brief 统一的bgfx资源管理器 - 单例模式
 * 
 * 负责管理着色器、uniform等共享资源，避免重复创建
 */
class BgfxResourceManager
{
public:
    static BgfxResourceManager& instance()
    {
        static BgfxResourceManager instance;
        return instance;
    }
    
    // 禁用拷贝构造和赋值
    BgfxResourceManager(const BgfxResourceManager&) = delete;
    BgfxResourceManager& operator=(const BgfxResourceManager&) = delete;
    
    /**
     * @brief 获取或创建着色器程序
     */
    bgfx::ProgramHandle getShaderProgram(const std::string& name);
    
    /**
     * @brief 获取或创建uniform句柄
     */
    bgfx::UniformHandle getUniform(const std::string& name, bgfx::UniformType::Enum type);
    
    /**
     * @brief 清理所有资源（bgfx重新初始化时调用）
     */
    void cleanup();
    
    /**
     * @brief 标记资源为无效（bgfx重新初始化时调用）
     */
    void invalidateResources();
    
    /**
     * @brief 检查资源是否已初始化
     */
    bool isInitialized() const { return m_initialized; }

private:
    BgfxResourceManager() = default;
    ~BgfxResourceManager() { safeCleanup(); }
    
    // 加载着色器程序
    bgfx::ProgramHandle loadShaderProgram(const std::string& name);

    // 安全清理（检查bgfx是否仍然有效）
    void safeCleanup();
    
    // 资源缓存
    std::unordered_map<std::string, bgfx::ProgramHandle> m_shaderPrograms;
    std::unordered_map<std::string, bgfx::UniformHandle> m_uniforms;
    
    bool m_initialized = false;
};

/**
 * @brief 常用的着色器和uniform名称常量
 */
namespace BgfxResources
{
    // 着色器程序名称
    constexpr const char* ROUNDED_SHADER = "rounded";
    
    // Uniform名称
    constexpr const char* ROUNDED_PARAMS = "u_roundedParams";
    constexpr const char* CONNECTOR_CONFIG = "u_connectorConfig";
}
