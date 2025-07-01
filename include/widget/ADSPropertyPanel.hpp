//
// Created by wuxianggujun on 25-7-1.
//

#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QtNodes/Definitions>

// 前向声明
class PropertyWidget;

namespace QtNodes {
    class DataFlowGraphModel;
}

/**
 * @brief ADS专用的轻量级属性面板
 * 
 * 专为ADS系统设计的属性面板，移除了冗余的标题栏和边框，
 * 直接使用PropertyWidget作为内容，让ADS处理所有的窗口管理。
 */
class ADSPropertyPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ADSPropertyPanel(QWidget* parent = nullptr);
    ~ADSPropertyPanel() override;

    /**
     * @brief 设置图形模型
     */
    void setGraphModel(QtNodes::DataFlowGraphModel* model);
    
    /**
     * @brief 更新节点属性
     */
    void updateNodeProperties(QtNodes::NodeId nodeId);
    
    /**
     * @brief 清空属性
     */
    void clearProperties();
    
    /**
     * @brief 获取属性控件（用于直接操作）
     */
    PropertyWidget* getPropertyWidget() const { return m_propertyWidget; }

signals:
    /**
     * @brief 属性值改变信号
     */
    void propertyChanged(const QString& propertyName, const QVariant& value);

private:
    QScrollArea* m_scrollArea;
    PropertyWidget* m_propertyWidget;

    QtNodes::NodeId m_nodeId;
    QtNodes::DataFlowGraphModel* m_graphModel;

    void setupUI();
    void showDefaultContent();
    void connectSignals();

    /**
     * @brief 判断节点类型是否需要编辑模式
     */
    bool isEditableNodeType(const QString& nodeTypeName) const;
};
