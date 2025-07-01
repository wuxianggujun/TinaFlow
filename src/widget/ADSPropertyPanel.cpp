//
// Created by wuxianggujun on 25-7-1.
//

#include "widget/ADSPropertyPanel.hpp"
#include "widget/PropertyWidget.hpp"
#include "IPropertyProvider.hpp"
#include <QVBoxLayout>
#include <QScrollArea>
#include <QDebug>
#include <QTimer>
#include <QApplication>
#include <QtNodes/DataFlowGraphModel>

ADSPropertyPanel::ADSPropertyPanel(QWidget* parent)
    : QWidget(parent)
    , m_scrollArea(nullptr)
    , m_propertyWidget(nullptr)
    , m_nodeId(QtNodes::NodeId{})
    , m_graphModel(nullptr)
{
    setupUI();
    connectSignals();
}

ADSPropertyPanel::~ADSPropertyPanel()
{
    // Qt会自动清理子控件
}

void ADSPropertyPanel::setupUI()
{
    // 极简布局 - 零边距，让ADS完全控制外观
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // 滚动区域 - 无边框
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    
    // 创建属性控件
    m_propertyWidget = new PropertyWidget();
    
    // 设置默认内容
    showDefaultContent();
    
    m_scrollArea->setWidget(m_propertyWidget);
    layout->addWidget(m_scrollArea);
    

}

void ADSPropertyPanel::connectSignals()
{
    if (m_propertyWidget) {
        connect(m_propertyWidget, &PropertyWidget::propertyChanged,
                this, &ADSPropertyPanel::propertyChanged);
    }
}

void ADSPropertyPanel::showDefaultContent()
{
    if (m_propertyWidget) {
        m_propertyWidget->clearAllProperties();
        m_propertyWidget->addDescription("点击节点查看和编辑属性");
    }
}

void ADSPropertyPanel::setGraphModel(QtNodes::DataFlowGraphModel* model)
{
    m_graphModel = model;

}

void ADSPropertyPanel::updateNodeProperties(QtNodes::NodeId nodeId)
{
    // 防止重复更新同一个节点
    if (m_nodeId == nodeId && m_propertyWidget && m_propertyWidget->hasProperties()) {

        return;
    }

    m_nodeId = nodeId;

    if (!m_graphModel) {
        qWarning() << "ADSPropertyPanel: 图形模型未设置";
        clearProperties();
        return;
    }

    auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
    if (!nodeDelegate) {
        qWarning() << "ADSPropertyPanel: 节点委托未找到" << nodeId;
        clearProperties();
        return;
    }

    QString nodeName = nodeDelegate->name();
    QString nodeCaption = nodeDelegate->caption();



    // 简单清空属性
    if (m_propertyWidget) {
        m_propertyWidget->clearAllProperties();

        // 添加节点基本信息
        m_propertyWidget->addTitle(nodeCaption);
        m_propertyWidget->addInfoProperty("节点类型", nodeName);
        m_propertyWidget->addInfoProperty("节点ID", QString::number(nodeId));
        m_propertyWidget->addSeparator();

        // 尝试使用属性提供系统
        bool hasProperties = false;
        auto* nodeModel = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (nodeModel) {
            auto* propertyProvider = dynamic_cast<IPropertyProvider*>(nodeModel);
            if (propertyProvider) {
                hasProperties = propertyProvider->createPropertyPanel(m_propertyWidget);

            }
        }

        // 如果没有属性提供者，显示默认消息
        if (!hasProperties) {
            m_propertyWidget->addDescription("此节点暂无可编辑属性");
        } else {
            // 检查节点类型，只有需要编辑的节点才添加模式切换按钮
            bool needsEditMode = isEditableNodeType(nodeName);
            if (needsEditMode) {
                m_propertyWidget->addModeToggleButtons();
            }
        }


    }
}

void ADSPropertyPanel::clearProperties()
{
    m_nodeId = QtNodes::NodeId{};
    showDefaultContent();


}

bool ADSPropertyPanel::isEditableNodeType(const QString& nodeTypeName) const
{
    // 定义需要编辑模式的节点类型
    static const QStringList editableNodeTypes = {
        "OpenExcel",        // 打开Excel文件
        "SaveExcel",        // 保存Excel文件
        "ReadCell",         // 读取单元格（有地址设置）
        "ReadRange",        // 读取范围（有范围设置）
        "StringCompare",    // 字符串比较（有比较值和操作设置）
        "SmartLoopProcessor", // 智能循环处理器
        "WriteCell",        // 写入单元格
        "WriteRange"        // 写入范围
    };

    // 显示类节点不需要编辑模式
    static const QStringList displayOnlyNodeTypes = {
        "DisplayCell",      // 显示单元格
        "DisplayRange",     // 显示范围
        "DisplayBoolean",   // 显示布尔值
        "DisplayRow",       // 显示行
        "DisplayCellList",  // 显示单元格列表
        "RangeInfo"         // 范围信息（只读显示）
    };

    // 如果是显示类节点，明确返回false
    if (displayOnlyNodeTypes.contains(nodeTypeName)) {

        return false;
    }

    // 如果是可编辑节点，返回true
    if (editableNodeTypes.contains(nodeTypeName)) {

        return true;
    }

    // 默认情况：如果不确定，不添加编辑按钮（更安全）
    return false;
}
