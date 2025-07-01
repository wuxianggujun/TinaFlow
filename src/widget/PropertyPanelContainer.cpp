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
    , m_contentWidget(nullptr)
    , m_contentLayout(nullptr)
    , m_currentPropertyWidget(nullptr)
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
    
    // 内容控件
    m_contentWidget = new QWidget();
    m_contentLayout = new QVBoxLayout(m_contentWidget);
    m_contentLayout->setContentsMargins(6, 6, 6, 6);
    m_contentLayout->setSpacing(4);
    
    // 默认内容
    showDefaultContent();
    
    m_scrollArea->setWidget(m_contentWidget);
    layout->addWidget(m_scrollArea);
}

void PropertyPanelContainer::showDefaultContent()
{
    clearContent();
    
    auto* defaultLabel = new QLabel("点击节点查看和编辑属性");
    defaultLabel->setAlignment(Qt::AlignCenter);
    defaultLabel->setStyleSheet("color: #666666; padding: 20px;");
    m_contentLayout->addWidget(defaultLabel);
    
    m_contentLayout->addStretch();
}

void PropertyPanelContainer::clearContent()
{
    // 清除旧内容
    QLayoutItem* item;
    while ((item = m_contentLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    
    m_currentPropertyWidget = nullptr;
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
    
    // 清除旧内容
    clearContent();

    // 基本信息
    QLabel* infoLabel = new QLabel(QString("节点类型: %1\nID: %2").arg(nodeName).arg(nodeId));
    infoLabel->setStyleSheet("color: #666666; font-size: 11px; padding: 4px;");
    m_contentLayout->addWidget(infoLabel);

    // 尝试使用新的属性提供系统
    bool hasProperties = false;

    // 获取节点模型并检查是否实现了IPropertyProvider接口
    auto* nodeModel = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
    if (nodeModel) {
        auto* propertyProvider = dynamic_cast<IPropertyProvider*>(nodeModel);
        if (propertyProvider) {
            // 使用新的PropertyWidget系统
            auto* propertyWidget = PropertyPanelManager::createPropertyPanel(m_contentLayout);
            m_currentPropertyWidget = propertyWidget;
            hasProperties = propertyProvider->createPropertyPanel(propertyWidget);
            qDebug() << "PropertyPanelContainer: Used PropertyWidget system for node" << nodeId;
        }
    }

    // 如果没有属性提供者，显示默认消息
    if (!hasProperties) {
        QLabel* genericLabel = new QLabel("此节点暂无可编辑属性");
        genericLabel->setAlignment(Qt::AlignCenter);
        genericLabel->setStyleSheet("color: #999999; padding: 20px;");
        m_contentLayout->addWidget(genericLabel);
    }

    // 添加弹性空间
    m_contentLayout->addStretch();
    
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
    
    clearContent();
    m_contentLayout->addWidget(content);
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
