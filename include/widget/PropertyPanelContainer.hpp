//
// Created by wuxianggujun on 25-7-1.
//

#pragma once

#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>
#include "PanelContainer.hpp"
#include <QtNodes/Definitions>

// 前向声明
class PropertyWidget;

namespace QtNodes {
    class DataFlowGraphModel;
}

class PropertyPanelContainer : public PanelContainer
{
    Q_OBJECT

public:
    explicit PropertyPanelContainer(QWidget* parent = nullptr);
    ~PropertyPanelContainer() override;

    void setPanelContent(QWidget* content) override;
    void setCollapsible(bool collapsible) override;
    void setDragAndDropEnabled(bool enabled) override;

    QString getPanelTitle() const override
    {
        return tr("属性面板");
    }

    QString getPanelId() const override
    {
        return "property_panel";
    }

    void setGraphModel(QtNodes::DataFlowGraphModel* model);
    void updateNodeProperties(QtNodes::NodeId nodeId);
    void clearProperties();

private:
    QLabel* m_titleLabel;
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_contentLayout;

    PropertyWidget* m_currentPropertyWidget;
    QtNodes::NodeId m_nodeId;
    QtNodes::DataFlowGraphModel* m_graphModel;

    void setupUI();
    void setupTitleBar(QVBoxLayout* layout);
    void setupContentArea(QVBoxLayout* layout);
    void showDefaultContent();
    void clearContent();
};
