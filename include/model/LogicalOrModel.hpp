//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/BooleanData.hpp"
#include "widget/PropertyWidget.hpp"
#include <QVBoxLayout>
#include <QLabel>

/**
 * @brief 逻辑或节点（OR）
 * 
 * 功能：
 * - 对两个布尔值执行逻辑或运算
 * - 只要有一个输入为true，输出就为true
 * 
 * 输入端口：
 * - 0: BooleanData - 条件A
 * - 1: BooleanData - 条件B
 * 
 * 输出端口：
 * - 0: BooleanData - A OR B
 */
class LogicalOrModel : public BaseNodeModel
{
    Q_OBJECT

public:
    LogicalOrModel() {
        m_inputData.resize(2);
    }

    QString caption() const override {
        return tr("逻辑或");
    }

    bool captionVisible() const override {
        return true;
    }

    QString name() const override {
        return tr("LogicalOr");
    }

    unsigned int nPorts(QtNodes::PortType portType) const override {
        return portType == QtNodes::PortType::In ? 2 : 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        return BooleanData().type();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override {
        if (port != 0) return nullptr;

        auto dataA = std::dynamic_pointer_cast<BooleanData>(getInputData(0));
        auto dataB = std::dynamic_pointer_cast<BooleanData>(getInputData(1));

        if (!dataA || !dataB) {
            return std::make_shared<BooleanData>(false);
        }

        bool result = dataA->value() || dataB->value();
        QString description = QString("%1 OR %2 = %3")
            .arg(dataA->value() ? "True" : "False")
            .arg(dataB->value() ? "True" : "False")
            .arg(result ? "True" : "False");

        return std::make_shared<BooleanData>(result, description);
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex portIndex) override {
        if (portIndex < m_inputData.size()) {
            m_inputData[portIndex] = nodeData;
        }
        emit dataUpdated(0);
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            m_widget = new QWidget();
            auto layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);
            layout->setSpacing(2);

            auto titleLabel = new QLabel("OR");
            titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; text-align: center;");
            titleLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(titleLabel);

            auto descLabel = new QLabel("A ∨ B");
            descLabel->setStyleSheet("font-size: 10px; color: #666; text-align: center;");
            descLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(descLabel);
        }
        return m_widget;
    }

    // IPropertyProvider接口实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override {
        if (!propertyWidget) return false;

        propertyWidget->addTitle("逻辑或运算");
        propertyWidget->addDescription("只要有一个输入为true，输出就为true");

        propertyWidget->addInfoProperty("运算符", "OR (∨)");
        propertyWidget->addInfoProperty("真值表", "False OR False = False\n其他情况 = True");

        return true;
    }

    QString getDisplayName() const override {
        return tr("逻辑或");
    }

protected:
    QString getNodeTypeName() const override {
        return "LogicalOr";
    }

private:
    QWidget* m_widget = nullptr;
    std::vector<std::shared_ptr<QtNodes::NodeData>> m_inputData;

    std::shared_ptr<QtNodes::NodeData> getInputData(QtNodes::PortIndex index) {
        if (index < m_inputData.size()) {
            return m_inputData[index];
        }
        return nullptr;
    }
};
