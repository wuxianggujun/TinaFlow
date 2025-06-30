#include "NodePalette.hpp"
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QFrame>
#include <QPainter>
#include <QStyleOption>
#include <QMouseEvent>
#include <QDebug>

// DraggableNodeList 实现
DraggableNodeList::DraggableNodeList(QWidget* parent)
    : QListWidget(parent), m_dragEnabled(true)
{
    setDragDropMode(QAbstractItemView::DragOnly);
    setDefaultDropAction(Qt::CopyAction);
}

void DraggableNodeList::startDrag(Qt::DropActions supportedActions)
{
    if (!m_dragEnabled || !currentItem()) {
        return;
    }
    
    NodeListItem* nodeItem = dynamic_cast<NodeListItem*>(currentItem());
    if (!nodeItem) {
        return;
    }
    
    // 发送拖拽开始信号
    emit nodeDragStarted(nodeItem->getNodeId());
    
    // 创建拖拽数据
    QMimeData* mimeData = new QMimeData;
    mimeData->setText(nodeItem->getNodeId());
    mimeData->setData("application/x-tinaflow-node", nodeItem->getNodeId().toUtf8());
    
    // 创建拖拽对象
    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    
    // 设置拖拽图标
    QPixmap dragPixmap(140, 40);
    dragPixmap.fill(Qt::transparent);
    QPainter painter(&dragPixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    painter.setBrush(QBrush(QColor(70, 130, 180, 200)));
    painter.setPen(QPen(QColor(70, 130, 180), 2));
    painter.drawRoundedRect(dragPixmap.rect().adjusted(2, 2, -2, -2), 6, 6);
    
    // 绘制文本
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(dragPixmap.rect(), Qt::AlignCenter, nodeItem->getNodeInfo().displayName);
    painter.end();
    
    drag->setPixmap(dragPixmap);
    drag->setHotSpot(QPoint(70, 20));
    
    // 执行拖拽
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    qDebug() << "DraggableNodeList: Drag completed with action:" << dropAction;
}

void DraggableNodeList::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }
    QListWidget::mousePressEvent(event);
}

void DraggableNodeList::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_dragEnabled || !(event->buttons() & Qt::LeftButton)) {
        QListWidget::mouseMoveEvent(event);
        return;
    }
    
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        QListWidget::mouseMoveEvent(event);
        return;
    }
    
    // 开始拖拽
    startDrag(Qt::CopyAction);
}

NodePalette::NodePalette(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_searchEdit(nullptr)
    , m_categoryCombo(nullptr)
    , m_favoritesButton(nullptr)
    , m_nodeList(nullptr)
    , m_favoritesList(nullptr)
    , m_splitter(nullptr)
    , m_statusLabel(nullptr)
    , m_currentCategory("所有分类")
    , m_showingFavorites(false)
    , m_dragDropEnabled(true)
{
    setupUI();
    refreshNodeList();
}

NodePalette::~NodePalette()
{
}

void NodePalette::setupUI()
{
    setFixedWidth(280);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(8, 8, 8, 8);
    m_mainLayout->setSpacing(8);
    
    // 标题
    QLabel* titleLabel = new QLabel("节点面板", this);
    titleLabel->setStyleSheet(
        "QLabel {"
        "font-size: 14px;"
        "font-weight: bold;"
        "color: #2c3e50;"
        "padding: 4px;"
        "}"
    );
    m_mainLayout->addWidget(titleLabel);
    
    setupSearchArea();
    setupFavoriteNodes();
    setupNodeList();
    
    // 状态标签
    m_statusLabel = new QLabel("共 0 个节点", this);
    m_statusLabel->setStyleSheet("color: #7f8c8d; font-size: 11px; padding: 2px;");
    m_mainLayout->addWidget(m_statusLabel);
    
    // 设置整体样式
    setStyleSheet(
        "NodePalette {"
        "background-color: #f8f9fa;"
        "border: 1px solid #dee2e6;"
        "border-radius: 8px;"
        "}"
    );
}

void NodePalette::setupSearchArea()
{
    // 搜索区域
    QGroupBox* searchGroup = new QGroupBox("搜索和筛选", this);
    QVBoxLayout* searchLayout = new QVBoxLayout(searchGroup);
    searchLayout->setSpacing(6);
    
    // 搜索框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("搜索节点...");
    m_searchEdit->setStyleSheet(
        "QLineEdit {"
        "padding: 8px 12px;"
        "border: 1px solid #ced4da;"
        "border-radius: 4px;"
        "font-size: 12px;"
        "}"
        "QLineEdit:focus {"
        "border-color: #80bdff;"
        "outline: 0;"
        "box-shadow: 0 0 0 0.2rem rgba(0,123,255,.25);"
        "}"
    );
    connect(m_searchEdit, &QLineEdit::textChanged, this, &NodePalette::onSearchTextChanged);
    searchLayout->addWidget(m_searchEdit);
    
    // 分类筛选
    m_categoryCombo = new QComboBox(this);
    m_categoryCombo->addItem("所有分类");
    QStringList categories = NodeCatalog::getAllCategories();
    m_categoryCombo->addItems(categories);
    m_categoryCombo->setStyleSheet(
        "QComboBox {"
        "padding: 6px 12px;"
        "border: 1px solid #ced4da;"
        "border-radius: 4px;"
        "font-size: 12px;"
        "}"
    );
    connect(m_categoryCombo, &QComboBox::currentTextChanged, this, &NodePalette::onCategoryChanged);
    searchLayout->addWidget(m_categoryCombo);
    
    m_mainLayout->addWidget(searchGroup);
}

void NodePalette::setupFavoriteNodes()
{
    // 常用节点区域
    m_favoritesButton = new QPushButton("⭐ 常用节点", this);
    m_favoritesButton->setCheckable(true);
    m_favoritesButton->setStyleSheet(
        "QPushButton {"
        "text-align: left;"
        "padding: 8px 12px;"
        "border: 1px solid #007bff;"
        "border-radius: 4px;"
        "background-color: #007bff;"
        "color: white;"
        "font-size: 12px;"
        "}"
        "QPushButton:checked {"
        "background-color: #0056b3;"
        "}"
        "QPushButton:hover {"
        "background-color: #0056b3;"
        "}"
    );
    connect(m_favoritesButton, &QPushButton::toggled, this, &NodePalette::onShowFavoritesToggled);
    m_mainLayout->addWidget(m_favoritesButton);
    
    // 常用节点列表
    m_favoritesList = new DraggableNodeList(this);
    m_favoritesList->setMaximumHeight(120);
    m_favoritesList->setStyleSheet(
        "QListWidget {"
        "border: 1px solid #dee2e6;"
        "border-radius: 4px;"
        "background-color: white;"
        "}"
        "QListWidget::item {"
        "padding: 4px 8px;"
        "border-bottom: 1px solid #f1f3f4;"
        "}"
        "QListWidget::item:hover {"
        "background-color: #e3f2fd;"
        "}"
        "QListWidget::item:selected {"
        "background-color: #2196f3;"
        "color: white;"
        "}"
    );
    connect(m_favoritesList, &QListWidget::itemClicked, this, &NodePalette::onNodeItemClicked);
    connect(m_favoritesList, &QListWidget::itemDoubleClicked, this, &NodePalette::onNodeItemDoubleClicked);
    connect(m_favoritesList, &DraggableNodeList::nodeDragStarted, this, &NodePalette::onNodeDragStarted);
    m_mainLayout->addWidget(m_favoritesList);
    
    // 加载常用节点
    QList<NodeInfo> favoriteNodes = NodeCatalog::getFrequentlyUsedNodes();
    for (const NodeInfo& nodeInfo : favoriteNodes) {
        NodeListItem* item = new NodeListItem(nodeInfo, m_favoritesList);
        item->setText(QString("⭐ %1").arg(nodeInfo.displayName));
        item->setToolTip(nodeInfo.description);
    }
}

void NodePalette::setupNodeList()
{
    // 所有节点区域
    QLabel* allNodesLabel = new QLabel("所有节点", this);
    allNodesLabel->setStyleSheet("font-weight: bold; padding: 4px;");
    m_mainLayout->addWidget(allNodesLabel);
    
    m_nodeList = new DraggableNodeList(this);
    m_nodeList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_nodeList->setStyleSheet(
        "QListWidget {"
        "border: 1px solid #dee2e6;"
        "border-radius: 4px;"
        "background-color: white;"
        "}"
        "QListWidget::item {"
        "padding: 8px;"
        "border-bottom: 1px solid #f1f3f4;"
        "}"
        "QListWidget::item:hover {"
        "background-color: #e3f2fd;"
        "}"
        "QListWidget::item:selected {"
        "background-color: #2196f3;"
        "color: white;"
        "}"
    );
    connect(m_nodeList, &QListWidget::itemClicked, this, &NodePalette::onNodeItemClicked);
    connect(m_nodeList, &QListWidget::itemDoubleClicked, this, &NodePalette::onNodeItemDoubleClicked);
    connect(m_nodeList, &DraggableNodeList::nodeDragStarted, this, &NodePalette::onNodeDragStarted);
    
    m_mainLayout->addWidget(m_nodeList);
}

void NodePalette::refreshNodeList()
{
    m_nodeList->clear();
    
    QList<NodeInfo> nodes;
    
    if (!m_currentFilter.isEmpty()) {
        nodes = NodeCatalog::searchNodes(m_currentFilter);
    } else if (m_currentCategory != "所有分类") {
        nodes = NodeCatalog::getNodesByCategory(m_currentCategory);
    } else {
        nodes = NodeCatalog::getAllNodes();
    }
    
    updateNodeList(nodes);
}

void NodePalette::updateNodeList(const QList<NodeInfo>& nodes)
{
    for (const NodeInfo& nodeInfo : nodes) {
        createNodeItem(nodeInfo);
    }
    
    // 更新状态
    m_statusLabel->setText(QString("共 %1 个节点").arg(nodes.size()));
}

void NodePalette::createNodeItem(const NodeInfo& nodeInfo)
{
    NodeListItem* item = new NodeListItem(nodeInfo, m_nodeList);
    
    // 使用简单文本，避免HTML标签显示问题
    QString displayText = QString("%1\n%2").arg(nodeInfo.displayName).arg(nodeInfo.description);
    item->setText(displayText);
    
    // 设置字体样式
    QFont font = item->font();
    font.setBold(false);
    item->setFont(font);
    
    // 设置工具提示
    item->setToolTip(QString("节点: %1\n分类: %2\n描述: %3\n\n双击创建节点，或拖拽到画布")
                    .arg(nodeInfo.displayName)
                    .arg(nodeInfo.category)
                    .arg(nodeInfo.description));
    
    // 设置项目高度和样式
    item->setSizeHint(QSize(0, 50));
    
    // 设置文本颜色和格式
    item->setData(Qt::ForegroundRole, QColor("#2c3e50"));
}

void NodePalette::setDragDropEnabled(bool enabled)
{
    m_dragDropEnabled = enabled;
    // DraggableNodeList 会自动处理拖拽设置
}

QString NodePalette::getSelectedNodeId() const
{
    QListWidgetItem* currentItem = m_nodeList->currentItem();
    NodeListItem* nodeItem = dynamic_cast<NodeListItem*>(currentItem);
    
    if (nodeItem) {
        return nodeItem->getNodeId();
    }
    
    // 检查常用节点列表
    currentItem = m_favoritesList->currentItem();
    nodeItem = dynamic_cast<NodeListItem*>(currentItem);
    
    if (nodeItem) {
        return nodeItem->getNodeId();
    }
    
    return QString();
}

void NodePalette::onSearchTextChanged(const QString& text)
{
    m_currentFilter = text.trimmed();
    refreshNodeList();
}

void NodePalette::onCategoryChanged(const QString& category)
{
    m_currentCategory = category;
    m_currentFilter.clear();
    m_searchEdit->clear();
    refreshNodeList();
}

void NodePalette::onNodeItemClicked(QListWidgetItem* item)
{
    NodeListItem* nodeItem = dynamic_cast<NodeListItem*>(item);
    if (nodeItem) {
        emit nodeSelectionChanged(nodeItem->getNodeId());
    }
}

void NodePalette::onNodeItemDoubleClicked(QListWidgetItem* item)
{
    NodeListItem* nodeItem = dynamic_cast<NodeListItem*>(item);
    if (nodeItem) {
        emit nodeCreationRequested(nodeItem->getNodeId());
        qDebug() << "NodePalette: Requesting creation of node:" << nodeItem->getNodeId();
    }
}

void NodePalette::onShowFavoritesToggled(bool showFavorites)
{
    m_showingFavorites = showFavorites;
    m_favoritesList->setVisible(showFavorites);
    
    if (showFavorites) {
        m_favoritesButton->setText("⭐ 隐藏常用");
    } else {
        m_favoritesButton->setText("⭐ 常用节点");
    }
}

void NodePalette::onNodeDragStarted(const QString& nodeId)
{
    qDebug() << "NodePalette: Drag started for node:" << nodeId;
    // 可以在这里添加拖拽开始时的额外处理
}

// NodeListItem 实现
NodeListItem::NodeListItem(const NodeInfo& nodeInfo, QListWidget* parent)
    : QListWidgetItem(parent), m_nodeInfo(nodeInfo)
{
    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled);
} 