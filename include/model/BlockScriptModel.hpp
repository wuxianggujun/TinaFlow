//
// Created by wuxianggujun on 25-7-3.
//

#pragma once

#include "BaseNodeModel.hpp"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QJsonObject>

// 前向声明
class BlockProgrammingView;

/**
 * @brief 积木脚本节点模型
 *
 * 简化的积木脚本节点，点击编辑按钮进入专门的积木编程页面
 * 用于处理Excel数据的可视化编程
 */
class BlockScriptModel : public BaseNodeModel
{
    Q_OBJECT

public:
    BlockScriptModel();
    ~BlockScriptModel() override = default;

    // 基本节点信息
    QString caption() const override { return tr("积木脚本"); }
    QString name() const override { return tr("BlockScript"); }

    // 端口配置
    unsigned int nPorts(QtNodes::PortType const portType) const override;
    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override;
    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override;

    // 嵌入式控件
    QWidget* embeddedWidget() override;

    // 保存和加载
    QJsonObject save() const override;
    void load(QJsonObject const& json) override;

    // IPropertyProvider 接口
    bool createPropertyPanel(PropertyWidget* propertyWidget) override;
    QString getDisplayName() const override { return "积木脚本"; }
    QString getDescription() const override { return "使用积木编程处理Excel数据"; }

protected:
    QString getNodeTypeName() const override { return "BlockScriptModel"; }

private slots:
    void onEditBlockScript();
    void onExecuteScript();
    void onBlockProgrammingViewClosed();
    void onScriptSaved(const QString& scriptName, const QJsonObject& configuration);

private:
    void createEmbeddedWidget();
    void executeBlockScript();
    void openBlockEditor();

private:
    // UI 组件
    QWidget* m_widget = nullptr;
    QPushButton* m_editButton = nullptr;
    QPushButton* m_executeButton = nullptr;
    QLabel* m_statusLabel = nullptr;

    // 数据
    std::shared_ptr<QtNodes::NodeData> m_inputData;
    std::shared_ptr<QtNodes::NodeData> m_outputData;

    // 积木脚本配置
    QString m_scriptName;
    QJsonObject m_blockConfiguration;

    // 积木编程视图
    BlockProgrammingView* m_blockProgrammingView = nullptr;
};
