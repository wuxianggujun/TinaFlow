//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/CellData.hpp"
#include "data/IntegerData.hpp"
#include "data/BooleanData.hpp"
#include "widget/PropertyWidget.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QVBoxLayout>
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
            switch (m_valueType) {
                case String: return CellData().type();
                case Number: return IntegerData().type();
                case Boolean: return BooleanData().type();
                default: return CellData().type();
            }
        }
        return {"", ""};
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override {
        if (port != 0) return nullptr;

        switch (m_valueType) {
            case String: {
                return std::make_shared<CellData>("CONST", QVariant(m_stringValue));
            }
            case Number: {
                return std::make_shared<IntegerData>(static_cast<int>(m_numberValue));
            }
            case Boolean: {
                return std::make_shared<BooleanData>(m_booleanValue);
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
            m_widget = new QWidget();
            auto layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);
            layout->setSpacing(2);

            // 类型选择
            auto typeLabel = new QLabel("类型:");
            typeLabel->setStyleSheet("font-weight: bold; font-size: 10px;");
            layout->addWidget(typeLabel);

            m_typeCombo = new QComboBox();
            m_typeCombo->addItems({"字符串", "数值", "布尔值"});
            m_typeCombo->setCurrentIndex(static_cast<int>(m_valueType));
            m_typeCombo->setStyleSheet("font-size: 10px;");

            connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [this](int index) {
                        m_valueType = static_cast<ValueType>(index);
                        updateValueWidget();
                        emit dataUpdated(0);
                    });

            layout->addWidget(m_typeCombo);

            // 值输入区域
            m_valueWidget = new QWidget();
            m_valueLayout = new QVBoxLayout(m_valueWidget);
            m_valueLayout->setContentsMargins(0, 0, 0, 0);
            m_valueLayout->setSpacing(2);
            layout->addWidget(m_valueWidget);

            updateValueWidget();

            // 注册属性控件
            registerProperty("valueType", m_typeCombo);
            // 注意：stringValue, numberValue, booleanValue的控件在updateValueWidget()中动态创建
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

        if (m_typeCombo) {
            m_typeCombo->setCurrentIndex(static_cast<int>(m_valueType));
            updateValueWidget();
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
                    if (m_typeCombo) {
                        m_typeCombo->setCurrentIndex(index);
                    }
                    updateValueWidget();
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
                        if (m_stringEdit) {
                            m_stringEdit->setText(newValue);
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
                            if (m_numberEdit) {
                                m_numberEdit->setText(newValue);
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
                        if (m_booleanCheck) {
                            m_booleanCheck->setChecked(checked);
                            m_booleanCheck->setText(checked ? "True" : "False");
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
    QComboBox* m_typeCombo = nullptr;
    QWidget* m_valueWidget = nullptr;
    QVBoxLayout* m_valueLayout = nullptr;

    // 当前值输入控件
    QLineEdit* m_stringEdit = nullptr;
    QLineEdit* m_numberEdit = nullptr;
    QCheckBox* m_booleanCheck = nullptr;

    void updateValueWidget() {
        if (!m_valueLayout) return;

        // 清除现有控件
        QLayoutItem* item;
        while ((item = m_valueLayout->takeAt(0)) != nullptr) {
            delete item->widget();
            delete item;
        }

        m_stringEdit = nullptr;
        m_numberEdit = nullptr;
        m_booleanCheck = nullptr;

        auto valueLabel = new QLabel("值:");
        valueLabel->setStyleSheet("font-weight: bold; font-size: 10px;");
        m_valueLayout->addWidget(valueLabel);

        switch (m_valueType) {
            case String: {
                m_stringEdit = new QLineEdit(m_stringValue);
                m_stringEdit->setPlaceholderText("输入字符串值");
                m_stringEdit->setStyleSheet("font-size: 10px;");
                connect(m_stringEdit, &QLineEdit::textChanged, [this](const QString& text) {
                    m_stringValue = text;
                    emit dataUpdated(0);
                });
                m_valueLayout->addWidget(m_stringEdit);
                break;
            }
            case Number: {
                m_numberEdit = new QLineEdit(QString::number(m_numberValue));
                m_numberEdit->setPlaceholderText("输入数值");
                m_numberEdit->setStyleSheet("font-size: 10px;");
                connect(m_numberEdit, &QLineEdit::textChanged, [this](const QString& text) {
                    bool ok;
                    double value = text.toDouble(&ok);
                    if (ok) {
                        m_numberValue = value;
                        emit dataUpdated(0);
                    }
                });
                m_valueLayout->addWidget(m_numberEdit);
                break;
            }
            case Boolean: {
                m_booleanCheck = new QCheckBox("True");
                m_booleanCheck->setChecked(m_booleanValue);
                m_booleanCheck->setStyleSheet("font-size: 10px;");
                connect(m_booleanCheck, &QCheckBox::toggled, [this](bool checked) {
                    m_booleanValue = checked;
                    m_booleanCheck->setText(checked ? "True" : "False");
                    emit dataUpdated(0);
                });
                m_valueLayout->addWidget(m_booleanCheck);
                break;
            }
        }
    }
};
