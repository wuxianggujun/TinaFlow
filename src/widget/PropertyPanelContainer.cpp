//
// Created by wuxianggujun on 25-7-1.
//

#include "widget/PropertyPanelContainer.hpp"
#include "widget/PropertyWidget.hpp"
#include "IPropertyProvider.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QDebug>
#include <QtNodes/DataFlowGraphModel>

PropertyPanelContainer::PropertyPanelContainer(QWidget* parent)
    : PanelContainer(parent)
    , m_titleLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_propertyWidget(nullptr)
    , m_nodeId(QtNodes::NodeId{})
    , m_graphModel(nullptr)
{
    setupUI();
}

PropertyPanelContainer::~PropertyPanelContainer()
{
    // 析构函数，Qt会自动清理子控件
}

void PropertyPanelContainer::setupUI()
{
    // 主布局
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // 标题栏
    setupTitleBar(mainLayout);
    
    // 内容区域
    setupContentArea(mainLayout);
    
    // 设置样式
    setStyleSheet(
        "PropertyPanelContainer {"
        "background-color: #f8f9fa;"
        "border: 1px solid #dee2e6;"
        "border-radius: 6px;"
        "}"
    );
    
    qDebug() << "PropertyPanelContainer: UI setup completed";
}

void PropertyPanelContainer::setupTitleBar(QVBoxLayout* layout)
{
    // 标题标签
    m_titleLabel = new QLabel("属性面板");
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "font-weight: bold;"
        "font-size: 14px;"
        "color: #2E86AB;"
        "padding: 6px;"
        "background-color: #e9ecef;"
        "border-radius: 4px;"
        "}"
    );
    m_titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_titleLabel);
}

void PropertyPanelContainer::setupContentArea(QVBoxLayout* layout)
{
    // 滚动区域
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // 创建复用的PropertyWidget
    m_propertyWidget = new PropertyWidget();

    // 默认内容
    showDefaultContent();

    m_scrollArea->setWidget(m_propertyWidget);
    layout->addWidget(m_scrollArea);
}

void PropertyPanelContainer::showDefaultContent()
{
    if (m_propertyWidget) {
        m_propertyWidget->clearAllProperties();
        m_propertyWidget->addDescription("点击节点查看和编辑属性");
    }
}



void PropertyPanelContainer::setGraphModel(QtNodes::DataFlowGraphModel* model)
{
    m_graphModel = model;
}

void PropertyPanelContainer::updateNodeProperties(QtNodes::NodeId nodeId)
{
    m_nodeId = nodeId;

    if (!m_graphModel) {
        qWarning() << "PropertyPanelContainer: No graph model set!";
        clearProperties();
        return;
    }

    auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
    if (!nodeDelegate) {
        qWarning() << "PropertyPanelContainer: Node delegate not found for" << nodeId;
        clearProperties();
        return;
    }

    QString nodeName = nodeDelegate->name();
    QString nodeCaption = nodeDelegate->caption();

    // 更新标题
    m_titleLabel->setText(QString("%1 属性").arg(nodeCaption));

    // 清空属性控件内容（复用控件）
    if (m_propertyWidget) {
        m_propertyWidget->clearAllProperties();

        // 添加节点基本信息
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
                qDebug() << "PropertyPanelContainer: Used PropertyWidget system for node" << nodeId;
            }
        }

        // 如果没有属性提供者，显示默认消息
        if (!hasProperties) {
            m_propertyWidget->addDescription("此节点暂无可编辑属性");
        }
    }

    // 触发标题变化信号
    emit titleChanged(m_titleLabel->text());

    qDebug() << "PropertyPanelContainer: Updated properties for node" << nodeId << "(" << nodeCaption << ")";
}

void PropertyPanelContainer::clearProperties()
{
    m_nodeId = QtNodes::NodeId{};
    m_titleLabel->setText("属性面板");
    showDefaultContent();
    
    emit titleChanged(m_titleLabel->text());
    qDebug() << "PropertyPanelContainer: Cleared properties";
}

void PropertyPanelContainer::setPanelContent(QWidget* content)
{
    if (!content) {
        qWarning() << "PropertyPanelContainer: Attempted to set null content";
        return;
    }

    // 直接设置到滚动区域
    m_scrollArea->setWidget(content);
    qDebug() << "PropertyPanelContainer: Set custom panel content";
}

void PropertyPanelContainer::setCollapsible(bool collapsible)
{
    // 实现折叠功能（暂时留空，后续可扩展）
    Q_UNUSED(collapsible);
    qDebug() << "PropertyPanelContainer: Collapsible set to" << collapsible;
}

void PropertyPanelContainer::setDragAndDropEnabled(bool enabled)
{
    // 实现拖拽功能（暂时留空，后续可扩展）
    Q_UNUSED(enabled);
    qDebug() << "PropertyPanelContainer: Drag and drop set to" << enabled;
}
