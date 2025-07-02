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
 * @brief 逻辑非节点（NOT）
 * 
 * 功能：
 * - 对布尔值执行逻辑非运算
 * - 将true变为false，false变为true
 * 
 * 输入端口：
 * - 0: BooleanData - 条件
 * 
 * 输出端口：
 * - 0: BooleanData - NOT 条件
 */
class LogicalNotModel : public BaseNodeModel
{
    Q_OBJECT

public:
    LogicalNotModel() {
        m_inputData.resize(1);
    }

    QString caption() const override {
        return tr("逻辑非");
    }

    bool captionVisible() const override {
        return true;
    }

    QString name() const override {
        return tr("LogicalNot");
    }

    unsigned int nPorts(QtNodes::PortType portType) const override {
        return 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        return BooleanData().type();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override {
        if (port != 0) return nullptr;

        auto inputData = std::dynamic_pointer_cast<BooleanData>(getInputData(0));
        if (!inputData) {
            return std::make_shared<BooleanData>(true); // 默认返回true
        }

        bool result = !inputData->value();
        QString description = QString("NOT %1 = %2")
            .arg(inputData->value() ? "True" : "False")
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

            auto titleLabel = new QLabel("NOT");
            titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; text-align: center;");
            titleLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(titleLabel);

            auto descLabel = new QLabel("¬A");
            descLabel->setStyleSheet("font-size: 10px; color: #666; text-align: center;");
            descLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(descLabel);
        }
        return m_widget;
    }

    // IPropertyProvider接口实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override {
        if (!propertyWidget) return false;

        propertyWidget->addTitle("逻辑非运算");
        propertyWidget->addDescription("将true变为false，false变为true");

        propertyWidget->addInfoProperty("运算符", "NOT (¬)");
        propertyWidget->addInfoProperty("真值表", "NOT True = False\nNOT False = True");

        return true;
    }

    QString getDisplayName() const override {
        return tr("逻辑非");
    }

protected:
    QString getNodeTypeName() const override {
        return "LogicalNot";
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
