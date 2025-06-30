#ifndef NODECATALOG_HPP
#define NODECATALOG_HPP

#include <QString>
#include <QStringList>
#include <QMap>
#include <QIcon>
#include <QPixmap>

/**
 * @brief 节点信息结构
 */
struct NodeInfo {
    QString id;                    // 节点ID（技术名称）
    QString displayName;           // 显示名称
    QString category;              // 分类
    QString description;           // 描述
    QString iconPath;              // 图标路径
    QStringList keywords;          // 搜索关键词
    bool isFrequentlyUsed;        // 是否为常用节点
    
    NodeInfo() : isFrequentlyUsed(false) {}
    
    NodeInfo(const QString& id, const QString& name, const QString& cat, 
             const QString& desc, const QString& icon = "", bool frequent = false)
        : id(id), displayName(name), category(cat), description(desc), 
          iconPath(icon), isFrequentlyUsed(frequent) 
    {
        // 自动生成搜索关键词
        keywords << name.toLower() << cat.toLower();
        QStringList words = name.split(' ', Qt::SkipEmptyParts);
        for (const QString& word : words) {
            keywords << word.toLower();
        }
        keywords.removeDuplicates();
    }
};

/**
 * @brief 节点目录管理器
 */
class NodeCatalog
{
public:
    // 节点分类
    enum Category {
        DataSource,     // 数据源
        Processing,     // 数据处理
        Display,        // 显示
        Control,        // 控制流
        Math,           // 数学运算
        Utility         // 实用工具
    };
    
    static QString categoryToString(Category category);
    static QString categoryToDisplayName(Category category);
    static QString categoryToIcon(Category category);
    
    // 获取所有节点信息
    static QList<NodeInfo> getAllNodes();
    
    // 按分类获取节点
    static QList<NodeInfo> getNodesByCategory(Category category);
    static QList<NodeInfo> getNodesByCategory(const QString& categoryName);
    
    // 搜索节点
    static QList<NodeInfo> searchNodes(const QString& query);
    
    // 获取常用节点
    static QList<NodeInfo> getFrequentlyUsedNodes();
    
    // 获取特定节点信息
    static NodeInfo getNodeInfo(const QString& nodeId);
    
    // 获取所有分类
    static QStringList getAllCategories();
    
private:
    static void initializeNodeCatalog();
    static QMap<QString, NodeInfo> s_nodeMap;
    static bool s_initialized;
};

#endif // NODECATALOG_HPP 