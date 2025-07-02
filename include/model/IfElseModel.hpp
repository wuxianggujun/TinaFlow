//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/BooleanData.hpp"
#include "widget/PropertyWidget.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QColor>
#include <QtNodes/NodeStyle>

/**
 * @brief 条件分支节点（If-Else）
 *
 * 功能：
 * - 根据布尔条件控制数据流向
 * - 只有一个输入端口接收布尔值
 * - 两个输出端口：True端口和False端口
 *
 * 输入端口：
 * - 0: BooleanData - 条件（来自比较节点）
 *
 * 输出端口：
 * - 0: BooleanData - True端口（条件为true时有输出）
 * - 1: BooleanData - False端口（条件为false时有输出）
 */
class IfElseModel : public BaseNodeModel
{
    Q_OBJECT

public:
    IfElseModel() {
        // 初始化输入数据存储
        m_inputData.resize(1);
    }

    QString caption() const override {
        return tr("条件分支");
    }

    bool captionVisible() const override {
        return true;
    }

    QString name() const override {
        return tr("IfElse");
    }

    unsigned int nPorts(QtNodes::PortType portType) const override {
        return portType == QtNodes::PortType::In ? 1 : 2;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        if (portType == QtNodes::PortType::In) {
            return BooleanData().type(); // 只接收布尔输入
        } else {
            // 为输出端口设置自定义标签，使用正确的 BooleanData 类型 ID
            if (portIndex == 0) {
                return {"boolean", "True"};  // True端口
            } else {
                return {"boolean", "False"}; // False端口
            }
        }
    }

    // 自定义端口标题
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        if (portType == QtNodes::PortType::In) {
            return "条件";
        } else {
            if (portIndex == 0) {
                return "True";
            } else {
                return "False";
            }
        }
    }

    // 自定义端口颜色
    QColor portColor(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const {
        if (portType == QtNodes::PortType::Out) {
            if (portIndex == 0) {
                return QColor(34, 139, 34);  // 绿色 - True端口
            } else {
                return QColor(220, 20, 60);  // 红色 - False端口
            }
        }
        return QColor(70, 130, 180); // 默认蓝色 - 输入端口
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override {
        // 获取条件输入
        auto conditionData = std::dynamic_pointer_cast<BooleanData>(getInputData(0));
        if (!conditionData) {
            return nullptr; // 没有条件输入
        }

        bool condition = conditionData->value();

        if (port == 0) {
            // True端口：只有条件为true时才有输出
            return condition ? std::make_shared<BooleanData>(true) : nullptr;
        } else if (port == 1) {
            // False端口：只有条件为false时才有输出
            return !condition ? std::make_shared<BooleanData>(false) : nullptr;
        }

        return nullptr;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex portIndex) override {
        if (portIndex < m_inputData.size()) {
            m_inputData[portIndex] = nodeData;
        }

        // 立即重新计算并通知所有输出端口更新
        emit dataUpdated(0); // 通知 True 端口
        emit dataUpdated(1); // 通知 False 端口
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            m_widget = new QWidget();
            auto layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);
            layout->setSpacing(2);

            // 显示当前状态
            m_statusLabel = new QLabel("等待条件输入");
            m_statusLabel->setStyleSheet("font-size: 10px; color: #666; text-align: center;");
            m_statusLabel->setAlignment(Qt::AlignCenter);
            layout->addWidget(m_statusLabel);

            // 端口说明
            auto infoLabel = new QLabel("输入: 条件\n输出: True(绿) / False(红)");
            infoLabel->setStyleSheet("font-size: 9px; color: #888;");
            layout->addWidget(infoLabel);

            updateStatusDisplay();
        }
        return m_widget;
    }

    QJsonObject save() const override {
        // IfElse节点没有需要保存的属性，只保存基础信息
        return BaseNodeModel::save();
    }

    void load(QJsonObject const& json) override {
        BaseNodeModel::load(json);
        updateStatusDisplay();
    }

    // IPropertyProvider接口实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override {
        if (!propertyWidget) return false;

        propertyWidget->addTitle("条件分支");
        propertyWidget->addDescription("根据布尔条件选择输出不同的数据");

        // 显示当前状态
        if (m_statusLabel) {
            QString statusText = m_statusLabel->text();
            propertyWidget->addInfoProperty("当前状态", statusText);
        }

        // 端口说明
        propertyWidget->addSeparator();
        propertyWidget->addInfoProperty("输入端口", "布尔条件");
        propertyWidget->addInfoProperty("输出端口 0", "True端口（条件为真时有输出）");
        propertyWidget->addInfoProperty("输出端口 1", "False端口（条件为假时有输出）");

        return true;
    }

    QString getDisplayName() const override {
        return tr("条件分支");
    }

protected:
    QString getNodeTypeName() const override {
        return "IfElse";
    }

private:
    QWidget* m_widget = nullptr;
    QLabel* m_statusLabel = nullptr;
    std::vector<std::shared_ptr<QtNodes::NodeData>> m_inputData;

    std::shared_ptr<QtNodes::NodeData> getInputData(QtNodes::PortIndex index) {
        if (index < m_inputData.size()) {
            return m_inputData[index];
        }
        return nullptr;
    }

    void updateStatusDisplay() {
        if (!m_statusLabel) return;

        auto conditionData = std::dynamic_pointer_cast<BooleanData>(getInputData(0));
        auto trueData = getInputData(1);
        auto falseData = getInputData(2);

        if (!conditionData) {
            m_statusLabel->setText("等待条件输入");
            m_statusLabel->setStyleSheet("font-size: 10px; color: #666;");
        } else {
            bool condition = conditionData->value();
            QString selectedBranch = condition ? "True分支" : "False分支";
            
            if (condition && trueData) {
                m_statusLabel->setText(QString("选择: %1 ✓").arg(selectedBranch));
                m_statusLabel->setStyleSheet("font-size: 10px; color: #28a745; font-weight: bold;");
            } else if (!condition && falseData) {
                m_statusLabel->setText(QString("选择: %1 ✓").arg(selectedBranch));
                m_statusLabel->setStyleSheet("font-size: 10px; color: #dc3545; font-weight: bold;");
            } else {
                m_statusLabel->setText(QString("选择: %1 (无数据)").arg(selectedBranch));
                m_statusLabel->setStyleSheet("font-size: 10px; color: #ffc107;");
            }
        }
    }

private slots:
    void onInputChanged() {
        updateStatusDisplay();
        emit dataUpdated(0);
    }
};
