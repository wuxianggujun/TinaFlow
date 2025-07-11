//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/CellData.hpp"
#include "data/IntegerData.hpp"
#include "data/BooleanData.hpp"
#include "data/ValueData.hpp"
#include "widget/PropertyWidget.hpp"
#include "widget/StyledLineEdit.hpp"
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

/**
 * @brief 常量值节点
 * 
 * 功能：
 * - 提供常量值输出
 * - 支持多种数据类型（字符串、数值、布尔值）
 * - 用于为比较节点和条件分支提供固定值
 * 
 * 输入端口：无
 * 
 * 输出端口：
 * - 0: CellData/IntegerData/BooleanData - 常量值
 */
class ConstantValueModel : public BaseNodeModel
{
    Q_OBJECT

public:
    enum ValueType {
        String = 0,   // 字符串
        Number = 1,   // 数值
        Boolean = 2   // 布尔值
    };

    ConstantValueModel() : m_valueType(String), m_stringValue(""), m_numberValue(0), m_booleanValue(false) {
        // 属性将在embeddedWidget()创建后注册
    }

    QString caption() const override {
        return tr("常量值");
    }

    bool captionVisible() const override {
        return true;
    }

    QString name() const override {
        return tr("ConstantValue");
    }

    unsigned int nPorts(QtNodes::PortType portType) const override {
        return portType == QtNodes::PortType::Out ? 1 : 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        if (portType == QtNodes::PortType::Out) {
            // 统一使用 "value" 类型，这样可以与 UniversalCompare 的输入端口匹配
            // 具体的类型信息保存在 ValueData 对象内部
            return {"value", "值"};
        }
        return {"", ""};
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override {
        if (port != 0) return nullptr;

        switch (m_valueType) {
            case String: {
                return std::make_shared<ValueData>(m_stringValue);
            }
            case Number: {
                return std::make_shared<ValueData>(m_numberValue);
            }
            case Boolean: {
                return std::make_shared<ValueData>(m_booleanValue);
            }
            default:
                return nullptr;
        }
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex portIndex) override {
        // 常量节点没有输入
    }



    QWidget* embeddedWidget() override {
        if (!m_widget) {
            // 使用自定义的样式化输入框
            m_valueEdit = new ConstantValueLineEdit();

            // 根据当前类型设置占位符和初始值
            updateInputDisplay();

            // 连接防抖动的文本变化事件
            connect(m_valueEdit, &StyledLineEdit::textChangedDebounced, [this](const QString& text) {
                parseAndSetValue(text); // parseAndSetValue 内部会处理信号发射
            });

            // 连接类型切换事件
            connect(m_valueEdit, &ConstantValueLineEdit::typeChangeRequested, [this]() {
                switchToNextType();
            });

            // 注册属性控件
            registerProperty("valueEdit", m_valueEdit);

            m_widget = m_valueEdit;
        }
        return m_widget;
    }

    QJsonObject save() const override {
        QJsonObject modelJson = BaseNodeModel::save();
        modelJson["valueType"] = static_cast<int>(m_valueType);
        modelJson["stringValue"] = m_stringValue;
        modelJson["numberValue"] = m_numberValue;
        modelJson["booleanValue"] = m_booleanValue;
        return modelJson;
    }

    void load(QJsonObject const& json) override {
        BaseNodeModel::load(json);
        if (json.contains("valueType")) {
            m_valueType = static_cast<ValueType>(json["valueType"].toInt());
        }
        if (json.contains("stringValue")) {
            m_stringValue = json["stringValue"].toString();
        }
        if (json.contains("numberValue")) {
            m_numberValue = json["numberValue"].toDouble();
        }
        if (json.contains("booleanValue")) {
            m_booleanValue = json["booleanValue"].toBool();
        }

        if (m_valueEdit) {
            updateInputDisplay();
        }
    }

    // IPropertyProvider接口实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override {
        if (!propertyWidget) return false;

        propertyWidget->addTitle("常量值设置");
        propertyWidget->addDescription("提供常量值输出，支持字符串、数值、布尔值");

        // 添加模式切换按钮
        propertyWidget->addModeToggleButtons();

        // 数据类型选择
        QStringList types = {"字符串", "数值", "布尔值"};
        propertyWidget->addComboProperty("数据类型", types,
            static_cast<int>(m_valueType), "valueType",
            [this](int index) {
                if (index >= 0 && index < 3) {
                    m_valueType = static_cast<ValueType>(index);
                    if (m_valueEdit) {
                        updateInputDisplay();
                    }
                    emit dataUpdated(0);
                }
            });

        // 根据当前类型添加值编辑控件
        switch (m_valueType) {
            case String:
                propertyWidget->addTextProperty("字符串值", m_stringValue,
                    "stringValue", "输入字符串常量",
                    [this](const QString& newValue) {
                        m_stringValue = newValue;
                        if (m_valueEdit) {
                            updateInputDisplay();
                        }
                        emit dataUpdated(0);
                    });
                break;
            case Number:
                propertyWidget->addTextProperty("数值", QString::number(m_numberValue),
                    "numberValue", "输入数值常量",
                    [this](const QString& newValue) {
                        bool ok;
                        double value = newValue.toDouble(&ok);
                        if (ok) {
                            m_numberValue = value;
                            if (m_valueEdit) {
                                updateInputDisplay();
                            }
                            emit dataUpdated(0);
                        }
                    });
                break;
            case Boolean:
                propertyWidget->addCheckBoxProperty("布尔值", m_booleanValue,
                    "booleanValue",
                    [this](bool checked) {
                        m_booleanValue = checked;
                        if (m_valueEdit) {
                            updateInputDisplay();
                        }
                        emit dataUpdated(0);
                    });
                break;
        }

        return true;
    }

    QString getDisplayName() const override {
        return tr("常量值");
    }

protected:
    QString getNodeTypeName() const override {
        return "ConstantValue";
    }

private:
    ValueType m_valueType;
    QString m_stringValue;
    double m_numberValue;
    bool m_booleanValue;

    QWidget* m_widget = nullptr;
    ConstantValueLineEdit* m_valueEdit = nullptr;

    QString getTypeDisplayName() const {
        switch (m_valueType) {
            case String: return "字符串";
            case Number: return "数值";
            case Boolean: return "布尔值";
            default: return "未知";
        }
    }

    void switchToNextType() {
        int newType = (static_cast<int>(m_valueType) + 1) % 3;
        m_valueType = static_cast<ValueType>(newType);
        updateInputDisplay();

        // 只需要通知数据更新，不需要重建端口
        // 因为我们使用统一的 "value" 类型，端口类型不会改变
        emit dataUpdated(0);
    }

    void updateInputDisplay() {
        if (!m_valueEdit) return;

        QString typeName = getTypeDisplayName();
        QString placeholder;
        QString currentValue;

        switch (m_valueType) {
            case String:
                placeholder = "输入文本";
                currentValue = m_stringValue;
                break;
            case Number:
                placeholder = "输入数字";
                currentValue = QString::number(m_numberValue);
                break;
            case Boolean:
                placeholder = "输入 true/false";
                currentValue = m_booleanValue ? "true" : "false";
                break;
        }

        // 使用新的 API 设置类型和占位符
        m_valueEdit->setValueType(typeName, placeholder);
        m_valueEdit->setText(currentValue);
    }

    void parseAndSetValue(const QString& text) {
        bool valueChanged = false;

        switch (m_valueType) {
            case String:
                if (m_stringValue != text) {
                    m_stringValue = text;
                    valueChanged = true;
                }
                break;
            case Number: {
                bool ok;
                double value = text.toDouble(&ok);
                if (ok && qAbs(m_numberValue - value) > 1e-9) {
                    m_numberValue = value;
                    valueChanged = true;
                }
                break;
            }
            case Boolean: {
                QString lowerText = text.toLower().trimmed();
                bool newValue = m_booleanValue;
                if (lowerText == "true" || lowerText == "1" || lowerText == "yes") {
                    newValue = true;
                } else if (lowerText == "false" || lowerText == "0" || lowerText == "no") {
                    newValue = false;
                }
                if (m_booleanValue != newValue) {
                    m_booleanValue = newValue;
                    valueChanged = true;
                }
                break;
            }
        }

        // 只有当值真正改变时才发射信号
        if (valueChanged) {
            emit dataUpdated(0);
        }
    }
};
