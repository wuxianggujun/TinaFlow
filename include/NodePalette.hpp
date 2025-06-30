#ifndef NODEPALETTE_HPP
#define NODEPALETTE_HPP

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "NodeCatalog.hpp"

/**
 * @brief 支持拖拽的节点列表控件
 */
class DraggableNodeList : public QListWidget
{
    Q_OBJECT
    
public:
    explicit DraggableNodeList(QWidget* parent = nullptr);

protected:
    void startDrag(Qt::DropActions supportedActions) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

signals:
    void nodeDragStarted(const QString& nodeId);

private:
    QPoint m_dragStartPosition;
    bool m_dragEnabled;
};

class NodePalette : public QWidget
{
    Q_OBJECT

public:
    explicit NodePalette(QWidget *parent = nullptr);
    ~NodePalette();

    // 设置是否启用拖拽模式
    void setDragDropEnabled(bool enabled);
    
    // 刷新节点列表
    void refreshNodeList();
    
    // 获取当前选中的节点ID
    QString getSelectedNodeId() const;

signals:
    // 用户请求创建节点（双击或拖拽）
    void nodeCreationRequested(const QString& nodeId);
    
    // 节点选择变化
    void nodeSelectionChanged(const QString& nodeId);

private slots:
    void onSearchTextChanged(const QString& text);
    void onCategoryChanged(const QString& category);
    void onNodeItemClicked(QListWidgetItem* item);
    void onNodeItemDoubleClicked(QListWidgetItem* item);
    void onShowFavoritesToggled(bool showFavorites);
    void onNodeDragStarted(const QString& nodeId);

private:
    void setupUI();
    void setupSearchArea();
    void setupNodeList();
    void setupFavoriteNodes();
    void updateNodeList(const QList<NodeInfo>& nodes);
    void createNodeItem(const NodeInfo& nodeInfo);
    QWidget* createNodeItemWidget(const NodeInfo& nodeInfo);
    
    // UI组件
    QVBoxLayout* m_mainLayout;
    QLineEdit* m_searchEdit;
    QComboBox* m_categoryCombo;
    QPushButton* m_favoritesButton;
    DraggableNodeList* m_nodeList;
    DraggableNodeList* m_favoritesList;
    QSplitter* m_splitter;
    QLabel* m_statusLabel;
    
    // 数据
    QString m_currentFilter;
    QString m_currentCategory;
    bool m_showingFavorites;
    bool m_dragDropEnabled;
};

/**
 * @brief 自定义节点项，支持拖拽
 */
class NodeListItem : public QListWidgetItem
{
public:
    NodeListItem(const NodeInfo& nodeInfo, QListWidget* parent = nullptr);
    
    QString getNodeId() const { return m_nodeInfo.id; }
    const NodeInfo& getNodeInfo() const { return m_nodeInfo; }

private:
    NodeInfo m_nodeInfo;
};

#endif // NODEPALETTE_HPP 