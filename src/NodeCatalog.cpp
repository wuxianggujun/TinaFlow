#include "NodeCatalog.hpp"
#include <QDebug>

// 静态成员变量定义
QMap<QString, NodeInfo> NodeCatalog::s_nodeMap;
bool NodeCatalog::s_initialized = false;

QString NodeCatalog::categoryToString(Category category)
{
    switch (category) {
        case DataSource: return "DataSource";
        case Processing: return "Processing";
        case Display: return "Display";
        case Control: return "Control";
        case Math: return "Math";
        case Utility: return "Utility";
        default: return "Unknown";
    }
}

QString NodeCatalog::categoryToDisplayName(Category category)
{
    switch (category) {
        case DataSource: return "数据源";
        case Processing: return "数据处理";
        case Display: return "显示";
        case Control: return "控制流";
        case Math: return "数学运算";
        case Utility: return "实用工具";
        default: return "未知";
    }
}

QString NodeCatalog::categoryToIcon(Category category)
{
    switch (category) {
        case DataSource: return ":/icons/datasource.png";
        case Processing: return ":/icons/processing.png";
        case Display: return ":/icons/display.png";
        case Control: return ":/icons/control.png";
        case Math: return ":/icons/math.png";
        case Utility: return ":/icons/utility.png";
        default: return ":/icons/unknown.png";
    }
}

void NodeCatalog::initializeNodeCatalog()
{
    if (s_initialized) return;
    
    s_nodeMap.clear();
    
    // 数据源节点
    s_nodeMap["OpenExcel"] = NodeInfo(
        "OpenExcel", 
        "Excel文件", 
        categoryToDisplayName(DataSource),
        "打开和读取Excel工作簿文件",
        categoryToIcon(DataSource),
        true  // 常用节点
    );
    
    s_nodeMap["SelectSheet"] = NodeInfo(
        "SelectSheet", 
        "选择工作表", 
        categoryToDisplayName(DataSource),
        "从Excel工作簿中选择特定的工作表",
        categoryToIcon(DataSource),
        true  // 常用节点
    );
    
    s_nodeMap["ReadCell"] = NodeInfo(
        "ReadCell", 
        "读取单元格", 
        categoryToDisplayName(DataSource),
        "从工作表中读取单个单元格的数据",
        categoryToIcon(DataSource),
        true  // 常用节点
    );
    
    s_nodeMap["ReadRange"] = NodeInfo(
        "ReadRange",
        "读取范围",
        categoryToDisplayName(DataSource),
        "从工作表中读取指定范围的数据",
        categoryToIcon(DataSource),
        true  // 常用节点
    );
    
    s_nodeMap["SaveExcel"] = NodeInfo(
        "SaveExcel", 
        "保存Excel", 
        categoryToDisplayName(DataSource),
        "将数据保存到Excel文件",
        categoryToIcon(DataSource),
        false
    );
    

    
    // 显示节点
    s_nodeMap["DisplayCell"] = NodeInfo(
        "DisplayCell", 
        "显示单元格", 
        categoryToDisplayName(Display),
        "显示单个单元格的数据内容",
        categoryToIcon(Display),
        true  // 常用节点
    );
    
    s_nodeMap["DisplayRange"] = NodeInfo(
        "DisplayRange", 
        "显示范围", 
        categoryToDisplayName(Display),
        "以表格形式显示数据范围",
        categoryToIcon(Display),
        false
    );
    




    s_nodeMap["ConstantValue"] = NodeInfo(
        "ConstantValue",
        "常量值",
        categoryToDisplayName(DataSource),
        "提供常量值输出，支持字符串、数值、布尔值",
        categoryToIcon(DataSource),
        true  // 常用节点
    );

    s_nodeMap["BlockScript"] = NodeInfo(
        "BlockScript",
        "积木脚本",
        categoryToDisplayName(Processing),
        "使用积木编程处理Excel数据，支持可视化逻辑编程",
        categoryToIcon(Processing),
        true  // 常用节点
    );



    s_initialized = true;
    qDebug() << "NodeCatalog: Initialized with" << s_nodeMap.size() << "nodes";
}

QList<NodeInfo> NodeCatalog::getAllNodes()
{
    initializeNodeCatalog();
    return s_nodeMap.values();
}

QList<NodeInfo> NodeCatalog::getNodesByCategory(Category category)
{
    return getNodesByCategory(categoryToDisplayName(category));
}

QList<NodeInfo> NodeCatalog::getNodesByCategory(const QString& categoryName)
{
    initializeNodeCatalog();
    QList<NodeInfo> result;
    
    for (const NodeInfo& node : s_nodeMap.values()) {
        if (node.category == categoryName) {
            result.append(node);
        }
    }
    
    return result;
}

QList<NodeInfo> NodeCatalog::searchNodes(const QString& query)
{
    initializeNodeCatalog();
    QList<NodeInfo> result;
    
    if (query.isEmpty()) {
        return getAllNodes();
    }
    
    QString lowerQuery = query.toLower();
    
    for (const NodeInfo& node : s_nodeMap.values()) {
        bool matches = false;
        
        // 搜索显示名称
        if (node.displayName.toLower().contains(lowerQuery)) {
            matches = true;
        }
        
        // 搜索描述
        if (!matches && node.description.toLower().contains(lowerQuery)) {
            matches = true;
        }
        
        // 搜索关键词
        if (!matches) {
            for (const QString& keyword : node.keywords) {
                if (keyword.contains(lowerQuery)) {
                    matches = true;
                    break;
                }
            }
        }
        
        if (matches) {
            result.append(node);
        }
    }
    
    return result;
}

QList<NodeInfo> NodeCatalog::getFrequentlyUsedNodes()
{
    initializeNodeCatalog();
    QList<NodeInfo> result;
    
    for (const NodeInfo& node : s_nodeMap.values()) {
        if (node.isFrequentlyUsed) {
            result.append(node);
        }
    }
    
    return result;
}

NodeInfo NodeCatalog::getNodeInfo(const QString& nodeId)
{
    initializeNodeCatalog();
    return s_nodeMap.value(nodeId, NodeInfo());
}

QStringList NodeCatalog::getAllCategories()
{
    initializeNodeCatalog();
    QStringList categories;
    
    for (const NodeInfo& node : s_nodeMap.values()) {
        if (!categories.contains(node.category)) {
            categories.append(node.category);
        }
    }
    
    categories.sort();
    return categories;
} 