//
// Created by wuxianggujun on 25-7-3.
//

#include "IconManager.hpp"
#include <QPixmap>
#include <QPainter>
#include <QDebug>

IconManager& IconManager::instance()
{
    static IconManager instance;
    return instance;
}

IconManager::IconManager()
{
    initializeIconMap();
}

void IconManager::initializeIconMap()
{
    // 操作类图标
    m_iconMap[IconType::Play] = ":/icons/actions/play";
    m_iconMap[IconType::Pause] = ":/icons/actions/pause";
    m_iconMap[IconType::Copy] = ":/icons/actions/copy";
    m_iconMap[IconType::Trash] = ":/icons/actions/trash";
    m_iconMap[IconType::Plus] = ":/icons/actions/plus";
    m_iconMap[IconType::Pencil] = ":/icons/actions/pencil";
    m_iconMap[IconType::PenOff] = ":/icons/actions/pen-off";
    m_iconMap[IconType::Undo] = ":/icons/actions/undo";
    m_iconMap[IconType::Redo] = ":/icons/actions/redo";
    
    // 文件操作图标
    m_iconMap[IconType::Save] = ":/icons/files/save";
    m_iconMap[IconType::SaveAll] = ":/icons/files/save-all";
    m_iconMap[IconType::SaveOff] = ":/icons/files/save-off";
    m_iconMap[IconType::Folder] = ":/icons/files/folder";
    m_iconMap[IconType::File] = ":/icons/files/file";
    m_iconMap[IconType::FilePlus] = ":/icons/files/file-plus";
    m_iconMap[IconType::Import] = ":/icons/files/import";
    m_iconMap[IconType::Upload] = ":/icons/files/upload";
    
    // 界面图标
    m_iconMap[IconType::Settings] = ":/icons/ui/settings";
    m_iconMap[IconType::Search] = ":/icons/ui/search";
    m_iconMap[IconType::Maximize] = ":/icons/ui/maximize";
    m_iconMap[IconType::ZoomIn] = ":/icons/ui/zoom-in";
    m_iconMap[IconType::ZoomOut] = ":/icons/ui/zoom-out";
    
    // 状态图标
    m_iconMap[IconType::Bug] = ":/icons/ui/bug";
}

QIcon IconManager::getIcon(IconType type, IconSize size) const
{
    QString path = getIconPath(type);
    if (path.isEmpty()) {
        qWarning() << "IconManager: Icon not found for type" << static_cast<int>(type);
        return QIcon();
    }
    
    return createIcon(path, size);
}

QIcon IconManager::getIcon(const QString& path, IconSize size) const
{
    if (path.isEmpty()) {
        qWarning() << "IconManager: Empty icon path provided";
        return QIcon();
    }
    
    return createIcon(path, size);
}

QString IconManager::getIconPath(IconType type) const
{
    return m_iconMap.value(type, QString());
}

bool IconManager::hasIcon(IconType type) const
{
    return m_iconMap.contains(type);
}

QIcon IconManager::createIcon(const QString& path, IconSize size) const
{
    // 创建缓存键
    QString cacheKey = QString("%1_%2").arg(path).arg(static_cast<int>(size));
    
    // 检查缓存
    if (m_iconCache.contains(cacheKey)) {
        return m_iconCache[cacheKey];
    }
    
    // 加载 SVG 并创建指定大小的图标
    QIcon icon;
    QPixmap pixmap(path);
    
    if (!pixmap.isNull()) {
        // 缩放到指定大小
        int iconSize = static_cast<int>(size);
        QPixmap scaledPixmap = pixmap.scaled(iconSize, iconSize, 
                                           Qt::KeepAspectRatio, 
                                           Qt::SmoothTransformation);
        icon = QIcon(scaledPixmap);
    } else {
        qWarning() << "IconManager: Failed to load icon from path:" << path;
        // 创建一个默认的空图标
        QPixmap defaultPixmap(static_cast<int>(size), static_cast<int>(size));
        defaultPixmap.fill(Qt::transparent);
        
        QPainter painter(&defaultPixmap);
        painter.setPen(QPen(Qt::gray, 1));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(defaultPixmap.rect().adjusted(1, 1, -1, -1));
        painter.drawLine(1, 1, defaultPixmap.width()-2, defaultPixmap.height()-2);
        painter.drawLine(1, defaultPixmap.height()-2, defaultPixmap.width()-2, 1);
        
        icon = QIcon(defaultPixmap);
    }
    
    // 缓存图标
    m_iconCache[cacheKey] = icon;
    
    return icon;
}
