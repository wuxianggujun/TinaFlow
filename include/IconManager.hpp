//
// Created by wuxianggujun on 25-7-3.
//

#pragma once

#include <QIcon>
#include <QString>
#include <QSize>
#include <QHash>

/**
 * @brief 图标管理器
 * 
 * 统一管理应用程序中的所有图标资源，提供便捷的图标访问接口
 */
class IconManager
{
public:
    /**
     * @brief 图标类型枚举
     */
    enum class IconType {
        // 操作类图标
        Play,           // 播放/运行
        Pause,          // 暂停
        Stop,           // 停止
        Copy,           // 复制
        Trash,          // 删除
        Plus,           // 添加
        Pencil,         // 编辑
        PenOff,         // 禁用编辑
        
        // 文件操作图标
        Save,           // 保存
        SaveAll,        // 全部保存
        SaveOff,        // 保存禁用
        Folder,         // 文件夹
        File,           // 文件
        Import,         // 导入
        Upload,         // 上传
        
        // 界面图标
        Settings,       // 设置
        Search,         // 搜索
        
        // 状态图标
        Bug,            // 错误/调试
        
        // 未知图标
        Unknown
    };
    
    /**
     * @brief 图标大小枚举
     */
    enum class IconSize {
        Small = 16,     // 小图标 (菜单项)
        Medium = 24,    // 中图标 (工具栏)
        Large = 32,     // 大图标 (节点)
        XLarge = 48     // 超大图标 (启动画面)
    };

public:
    /**
     * @brief 获取单例实例
     */
    static IconManager& instance();
    
    /**
     * @brief 获取图标
     * @param type 图标类型
     * @param size 图标大小 (可选)
     * @return QIcon 对象
     */
    QIcon getIcon(IconType type, IconSize size = IconSize::Medium) const;
    
    /**
     * @brief 通过路径获取图标
     * @param path 图标资源路径
     * @param size 图标大小 (可选)
     * @return QIcon 对象
     */
    QIcon getIcon(const QString& path, IconSize size = IconSize::Medium) const;
    
    /**
     * @brief 获取图标路径
     * @param type 图标类型
     * @return 资源路径字符串
     */
    QString getIconPath(IconType type) const;
    
    /**
     * @brief 检查图标是否存在
     * @param type 图标类型
     * @return 是否存在
     */
    bool hasIcon(IconType type) const;

private:
    IconManager();
    ~IconManager() = default;
    
    // 禁用拷贝和赋值
    IconManager(const IconManager&) = delete;
    IconManager& operator=(const IconManager&) = delete;
    
    /**
     * @brief 初始化图标映射
     */
    void initializeIconMap();
    
    /**
     * @brief 创建指定大小的图标
     * @param path 图标路径
     * @param size 图标大小
     * @return QIcon 对象
     */
    QIcon createIcon(const QString& path, IconSize size) const;

private:
    QHash<IconType, QString> m_iconMap;  // 图标类型到路径的映射
    mutable QHash<QString, QIcon> m_iconCache;  // 图标缓存
};

/**
 * @brief 便捷的图标获取函数
 */
namespace Icons {
    inline QIcon get(IconManager::IconType type, IconManager::IconSize size = IconManager::IconSize::Medium) {
        return IconManager::instance().getIcon(type, size);
    }
    
    inline QIcon get(const QString& path, IconManager::IconSize size = IconManager::IconSize::Medium) {
        return IconManager::instance().getIcon(path, size);
    }
}

// 便捷的图标类型别名
using IconType = IconManager::IconType;
using IconSize = IconManager::IconSize;
