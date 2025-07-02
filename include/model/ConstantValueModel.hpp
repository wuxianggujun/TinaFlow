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
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QEvent>

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
            switch (m_valueType) {
                case String: return {"value_string", "Value(字符串)"};
                case Number: return {"value_number", "Value(数值)"};
                case Boolean: return {"value_boolean", "Value(布尔值)"};
                default: return {"value", "Value"};
            }
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

    bool eventFilter(QObject* obj, QEvent* event) override {
        if (obj == m_valueEdit && event->type() == QEvent::MouseButtonDblClick) {
            switchToNextType();
            return true;
        }
        return BaseNodeModel::eventFilter(obj, event);
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            // 直接使用自定义的输入框，避免任何容器嵌套问题
            m_valueEdit = new QLineEdit();
            m_valueEdit->setStyleSheet(
                "QLineEdit {"
                "  font-size: 10px;"
                "  border: 1px solid #ccc;"
                "  border-radius: 3px;"
                "  padding: 2px 4px;"
                "  background: white;"
                "}"
                "QLineEdit:focus {"
                "  border: 2px solid #0066cc;"
                "  background: #f8f8ff;"
                "}"
            );

            // 根据当前类型设置占位符和初始值
            updateInputDisplay();

            // 连接文本变化事件
            connect(m_valueEdit, &QLineEdit::textChanged, [this](const QString& text) {
                parseAndSetValue(text);
                emit dataUpdated(0);
            });

            // 使用事件过滤器来处理双击切换类型
            m_valueEdit->installEventFilter(this);

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
    QLineEdit* m_valueEdit = nullptr;

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

        // 通知端口类型变化 - 这会让连接的节点知道端口类型改变了
        emit portsAboutToBeDeleted(QtNodes::PortType::Out, 0, 0);
        emit portsDeleted();

        emit dataUpdated(0);
    }

    void updateInputDisplay() {
        if (!m_valueEdit) return;

        QString placeholder;
        QString currentValue;

        switch (m_valueType) {
            case String:
                placeholder = QString("[字符串] 输入文本 (双击切换类型)");
                currentValue = m_stringValue;
                break;
            case Number:
                placeholder = QString("[数值] 输入数字 (双击切换类型)");
                currentValue = QString::number(m_numberValue);
                break;
            case Boolean:
                placeholder = QString("[布尔值] 输入 true/false (双击切换类型)");
                currentValue = m_booleanValue ? "true" : "false";
                break;
        }

        m_valueEdit->setPlaceholderText(placeholder);
        m_valueEdit->setText(currentValue);
        m_valueEdit->setToolTip(QString("当前类型: %1\n双击可切换类型").arg(getTypeDisplayName()));
    }

    void parseAndSetValue(const QString& text) {
        switch (m_valueType) {
            case String:
                m_stringValue = text;
                break;
            case Number: {
                bool ok;
                double value = text.toDouble(&ok);
                if (ok) {
                    m_numberValue = value;
                }
                break;
            }
            case Boolean: {
                QString lowerText = text.toLower().trimmed();
                if (lowerText == "true" || lowerText == "1" || lowerText == "yes") {
                    m_booleanValue = true;
                } else if (lowerText == "false" || lowerText == "0" || lowerText == "no") {
                    m_booleanValue = false;
                }
                break;
            }
        }
    }
};
