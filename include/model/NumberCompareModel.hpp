//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/BooleanData.hpp"
#include "data/CellData.hpp"
#include "data/IntegerData.hpp"
#include "widget/PropertyWidget.hpp"
#include <QComboBox>
#include <QVBoxLayout>
#include <QLabel>

/**
 * @brief 数值比较节点
 * 
 * 功能：
 * - 比较两个数值的大小关系
 * - 支持多种比较操作符
 * - 输出布尔结果供条件分支使用
 * 
 * 输入端口：
 * - 0: CellData/IntegerData - 左操作数
 * - 1: CellData/IntegerData - 右操作数
 * 
 * 输出端口：
 * - 0: BooleanData - 比较结果
 */
class NumberCompareModel : public BaseNodeModel
{
    Q_OBJECT

public:
    enum CompareOperator {
        Greater = 0,      // >
        Less = 1,         // <
        Equal = 2,        // ==
        NotEqual = 3,     // !=
        GreaterEqual = 4, // >=
        LessEqual = 5     // <=
    };

    NumberCompareModel() : m_operator(Equal) {
        // 注册需要保存的属性
        registerProperty("operator", nullptr); // 手动处理
    }

    QString caption() const override {
        return tr("数值比较");
    }

    bool captionVisible() const override {
        return true;
    }

    QString name() const override {
        return tr("NumberCompare");
    }

    unsigned int nPorts(QtNodes::PortType portType) const override {
        return portType == QtNodes::PortType::In ? 2 : 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        if (portType == QtNodes::PortType::In) {
            return CellData().type(); // 接受CellData或IntegerData
        } else {
            return BooleanData().type(); // 输出布尔值
        }
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override {
        auto leftData = getInputData(0);
        auto rightData = getInputData(1);

        if (!leftData || !rightData) {
            return std::make_shared<BooleanData>(false);
        }

        double leftValue = extractNumericValue(leftData);
        double rightValue = extractNumericValue(rightData);

        bool result = performComparison(leftValue, rightValue);
        return std::make_shared<BooleanData>(result);
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex portIndex) override {
        // 存储输入数据
        if (portIndex < m_inputData.size()) {
            m_inputData[portIndex] = nodeData;
        }

        // 立即重新计算并通知输出更新
        emit dataUpdated(0);
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            m_widget = new QWidget();
            auto layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);
            layout->setSpacing(2);

            // 标题
            auto titleLabel = new QLabel("比较操作:");
            titleLabel->setStyleSheet("font-weight: bold; font-size: 10px;");
            layout->addWidget(titleLabel);

            // 操作符选择
            m_operatorCombo = new QComboBox();
            m_operatorCombo->addItems({">", "<", "==", "!=", ">=", "<="});
            m_operatorCombo->setCurrentIndex(static_cast<int>(m_operator));
            m_operatorCombo->setStyleSheet("font-size: 10px;");

            connect(m_operatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [this](int index) {
                        m_operator = static_cast<CompareOperator>(index);
                        emit dataUpdated(0); // 重新计算输出
                    });

            layout->addWidget(m_operatorCombo);
        }
        return m_widget;
    }

    QJsonObject save() const override {
        QJsonObject modelJson = BaseNodeModel::save();
        modelJson["operator"] = static_cast<int>(m_operator);
        return modelJson;
    }

    void load(QJsonObject const& json) override {
        BaseNodeModel::load(json);
        if (json.contains("operator")) {
            m_operator = static_cast<CompareOperator>(json["operator"].toInt());
            if (m_operatorCombo) {
                m_operatorCombo->setCurrentIndex(static_cast<int>(m_operator));
            }
        }
    }

    // IPropertyProvider接口实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override {
        if (!propertyWidget) return false;

        propertyWidget->addTitle("数值比较设置");
        propertyWidget->addDescription("比较两个数值的大小关系，输出布尔结果");

        // 比较操作符设置
        QStringList operators = {">", "<", "==", "!=", ">=", "<="};
        propertyWidget->addComboProperty("比较操作", operators,
            static_cast<int>(m_operator), "operator",
            [this](int index) {
                if (index >= 0 && index < 6) {
                    m_operator = static_cast<CompareOperator>(index);
                    if (m_operatorCombo) {
                        m_operatorCombo->setCurrentIndex(index);
                    }
                    emit dataUpdated(0);
                }
            });

        return true;
    }

    QString getDisplayName() const override {
        return tr("数值比较");
    }

protected:
    QString getNodeTypeName() const override {
        return "NumberCompare";
    }

private:
    CompareOperator m_operator;
    QWidget* m_widget = nullptr;
    QComboBox* m_operatorCombo = nullptr;
    std::vector<std::shared_ptr<QtNodes::NodeData>> m_inputData{2};

    std::shared_ptr<QtNodes::NodeData> getInputData(QtNodes::PortIndex index) {
        if (index < m_inputData.size()) {
            return m_inputData[index];
        }
        return nullptr;
    }

    double extractNumericValue(std::shared_ptr<QtNodes::NodeData> data) {
        if (auto cellData = std::dynamic_pointer_cast<CellData>(data)) {
            return cellData->value().toDouble();
        } else if (auto intData = std::dynamic_pointer_cast<IntegerData>(data)) {
            return static_cast<double>(intData->value());
        }
        return 0.0;
    }

    bool performComparison(double left, double right) {
        switch (m_operator) {
            case Greater: return left > right;
            case Less: return left < right;
            case Equal: return qAbs(left - right) < 1e-9; // 浮点数相等比较
            case NotEqual: return qAbs(left - right) >= 1e-9;
            case GreaterEqual: return left >= right;
            case LessEqual: return left <= right;
            default: return false;
        }
    }
};
