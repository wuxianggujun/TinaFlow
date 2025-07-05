#include "BgfxResourceManager.hpp"
#include "shaders.h"
#include <QDebug>

bgfx::ProgramHandle BgfxResourceManager::getShaderProgram(const std::string& name)
{
    auto it = m_shaderPrograms.find(name);
    if (it != m_shaderPrograms.end() && bgfx::isValid(it->second)) {
        return it->second;
    }
    
    // 创建新的着色器程序
    bgfx::ProgramHandle program = loadShaderProgram(name);
    if (bgfx::isValid(program)) {
        m_shaderPrograms[name] = program;
        m_initialized = true;
        qDebug() << "BgfxResourceManager: Created shader program:" << name.c_str();
    } else {
        qWarning() << "BgfxResourceManager: Failed to create shader program:" << name.c_str();
    }
    
    return program;
}

bgfx::UniformHandle BgfxResourceManager::getUniform(const std::string& name, bgfx::UniformType::Enum type)
{
    auto it = m_uniforms.find(name);
    if (it != m_uniforms.end() && bgfx::isValid(it->second)) {
        return it->second;
    }
    
    // 创建新的uniform
    bgfx::UniformHandle uniform = bgfx::createUniform(name.c_str(), type);
    if (bgfx::isValid(uniform)) {
        m_uniforms[name] = uniform;
        qDebug() << "BgfxResourceManager: Created uniform:" << name.c_str();
    } else {
        qWarning() << "BgfxResourceManager: Failed to create uniform:" << name.c_str();
    }
    
    return uniform;
}

void BgfxResourceManager::cleanup()
{
    if (!m_initialized) {
        return; // 已经清理过了
    }

    // 销毁着色器程序
    for (auto& pair : m_shaderPrograms) {
        if (bgfx::isValid(pair.second)) {
            bgfx::destroy(pair.second);
        }
    }
    m_shaderPrograms.clear();

    // 销毁uniform
    for (auto& pair : m_uniforms) {
        if (bgfx::isValid(pair.second)) {
            bgfx::destroy(pair.second);
        }
    }
    m_uniforms.clear();

    m_initialized = false;
    qDebug() << "BgfxResourceManager: Cleaned up all resources";
}

void BgfxResourceManager::invalidateResources()
{
    // 标记所有资源为无效，不尝试销毁
    for (auto& pair : m_shaderPrograms) {
        pair.second = BGFX_INVALID_HANDLE;
    }
    
    for (auto& pair : m_uniforms) {
        pair.second = BGFX_INVALID_HANDLE;
    }
    
    m_initialized = false;
    qDebug() << "BgfxResourceManager: Invalidated all resources";
}

void BgfxResourceManager::safeCleanup()
{
    // 检查bgfx是否仍然有效，避免在程序退出时崩溃
    if (bgfx::getRendererType() != bgfx::RendererType::Noop) {
        cleanup();
    } else {
        // bgfx已经关闭，只需要清空容器
        m_shaderPrograms.clear();
        m_uniforms.clear();
        m_initialized = false;
        qDebug() << "BgfxResourceManager: Safe cleanup completed (bgfx already shutdown)";
    }
}

bgfx::ProgramHandle BgfxResourceManager::loadShaderProgram(const std::string& name)
{
    if (name == "rounded") {
        // 获取当前渲染器类型
        bgfx::RendererType::Enum renderer = bgfx::getRendererType();
        const char* shaderDir = nullptr;
        
        // 根据渲染器选择着色器
        switch (renderer) {
            case bgfx::RendererType::Direct3D11:
            case bgfx::RendererType::Direct3D12:
                shaderDir = "dx11";
                break;
            case bgfx::RendererType::OpenGL:
                shaderDir = "glsl";
                break;
            case bgfx::RendererType::Vulkan:
                shaderDir = "spirv";
                break;
            default:
                qWarning() << "BgfxResourceManager: Unsupported renderer type";
                return BGFX_INVALID_HANDLE;
        }
        
        // 加载顶点着色器
        bgfx::ShaderHandle vsh = BGFX_INVALID_HANDLE;
        bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
        
        if (strcmp(shaderDir, "dx11") == 0) {
            vsh = bgfx::createShader(bgfx::makeRef(vs_rounded_dx11, sizeof(vs_rounded_dx11)));
            fsh = bgfx::createShader(bgfx::makeRef(fs_rounded_dx11, sizeof(fs_rounded_dx11)));
        }
        // 可以添加其他渲染器的支持
        
        if (bgfx::isValid(vsh) && bgfx::isValid(fsh)) {
            qDebug() << "BgfxResourceManager: Loaded shaders for" << shaderDir;
            return bgfx::createProgram(vsh, fsh, true); // true表示自动销毁着色器
        } else {
            qWarning() << "BgfxResourceManager: Failed to load shaders for" << shaderDir;
            if (bgfx::isValid(vsh)) bgfx::destroy(vsh);
            if (bgfx::isValid(fsh)) bgfx::destroy(fsh);
        }
    }
    
    return BGFX_INVALID_HANDLE;
}
