//
// Created by wuxianggujun on 25-6-30.
//

#pragma once

#include "IPropertyProvider.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QJsonObject>
#include <QDebug>
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>

/**
 * @brief 节点模型的基类
 * 
 * 封装了常见的save()和load()方法模式，提供了：
 * - 标准化的JSON保存/加载逻辑
 * - 常用UI控件的自动保存/加载
 * - 统一的调试日志输出
 * - 可扩展的属性管理系统
 * 
 * 子类只需要：
 * 1. 调用registerProperty()注册需要保存的属性
 * 2. 重写getNodeTypeName()返回节点类型名称
 * 3. 可选择重写onSave()和onLoad()进行自定义处理
 */
class BaseNodeModel : public QtNodes::NodeDelegateModel, public PropertyProviderBase
{
    Q_OBJECT

public:
    BaseNodeModel() = default;
    ~BaseNodeModel() override {
        // 断开所有信号连接，避免悬空指针
        disconnect();

        // 清理注册的属性控件
        for (auto& prop : m_properties) {
            if (prop.widget) {
                prop.widget->disconnect();
            }
        }
        m_properties.clear();
    }

    // 标准化的save/load实现
    QJsonObject save() const override
    {
        QJsonObject modelJson = QtNodes::NodeDelegateModel::save();
        
        // 保存注册的属性
        for (const auto& prop : m_properties) {
            QVariant value = getPropertyValue(prop.name, prop.widget);
            if (value.isValid()) {
                modelJson[prop.name] = QJsonValue::fromVariant(value);
            }
        }
        
        // 调用子类的自定义保存逻辑
        onSave(modelJson);

        // 只在调试模式下输出日志
        #ifdef QT_DEBUG
        if (modelJson.size() > 1) { // 只有当有实际属性时才输出
            qDebug() << getNodeTypeName() << ": Saved properties:" << modelJson.keys();
        }
        #endif
        return modelJson;
    }

    void load(QJsonObject const& json) override
    {
        qDebug() << getNodeTypeName() << ": Loading properties:" << json.keys();
        
        // 加载注册的属性
        for (const auto& prop : m_properties) {
            if (json.contains(prop.name)) {
                QVariant value = json[prop.name].toVariant();
                setPropertyValue(prop.name, prop.widget, value);
            }
        }
        
        // 调用子类的自定义加载逻辑
        onLoad(json);
    }

protected:
    // 属性注册系统
    struct PropertyInfo {
        QString name;
        QWidget* widget;
        QString description;
    };

    /**
     * @brief 注册需要自动保存/加载的属性
     * @param name 属性名称（用作JSON键）
     * @param widget 关联的UI控件
     * @param description 属性描述（可选，用于调试）
     */
    void registerProperty(const QString& name, QWidget* widget, const QString& description = "")
    {
        m_properties.append({name, widget, description});
        qDebug() << getNodeTypeName() << ": Registered property" << name << "with widget" << widget;
    }

    /**
     * @brief 批量注册属性
     */
    void registerProperties(const QList<PropertyInfo>& properties)
    {
        for (const auto& prop : properties) {
            registerProperty(prop.name, prop.widget, prop.description);
        }
    }

    // 子类需要实现的虚函数
    virtual QString getNodeTypeName() const = 0;

    // 可选的自定义保存/加载钩子
    virtual void onSave(QJsonObject& json) const 
    {
        Q_UNUSED(json);
        // 默认不做任何处理
    }

    virtual void onLoad(const QJsonObject& json) 
    {
        Q_UNUSED(json);
        // 默认不做任何处理
    }

    // 便利方法：快速注册常用控件
    void registerLineEdit(const QString& name, QLineEdit* lineEdit, const QString& description = "")
    {
        registerProperty(name, lineEdit, description.isEmpty() ? QString("LineEdit: %1").arg(name) : description);
    }

    void registerComboBox(const QString& name, QComboBox* comboBox, const QString& description = "")
    {
        registerProperty(name, comboBox, description.isEmpty() ? QString("ComboBox: %1").arg(name) : description);
    }

    void registerSpinBox(const QString& name, QSpinBox* spinBox, const QString& description = "")
    {
        registerProperty(name, spinBox, description.isEmpty() ? QString("SpinBox: %1").arg(name) : description);
    }

    void registerCheckBox(const QString& name, QCheckBox* checkBox, const QString& description = "")
    {
        registerProperty(name, checkBox, description.isEmpty() ? QString("CheckBox: %1").arg(name) : description);
    }

    void registerTextEdit(const QString& name, QTextEdit* textEdit, const QString& description = "")
    {
        registerProperty(name, textEdit, description.isEmpty() ? QString("TextEdit: %1").arg(name) : description);
    }

private:
    // 从控件获取值
    QVariant getPropertyValue(const QString& name, QWidget* widget) const
    {
        if (!widget) {
            qWarning() << getNodeTypeName() << ": Widget is null for property" << name;
            return QVariant();
        }

        // 根据控件类型获取值
        if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
            return lineEdit->text();
        } else if (auto* comboBox = qobject_cast<QComboBox*>(widget)) {
            return comboBox->currentIndex();
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(widget)) {
            return spinBox->value();
        } else if (auto* checkBox = qobject_cast<QCheckBox*>(widget)) {
            return checkBox->isChecked();
        } else if (auto* textEdit = qobject_cast<QTextEdit*>(widget)) {
            return textEdit->toPlainText();
        } else {
            qWarning() << getNodeTypeName() << ": Unsupported widget type for property" << name;
            return QVariant();
        }
    }

    // 设置控件值
    void setPropertyValue(const QString& name, QWidget* widget, const QVariant& value) const
    {
        if (!widget) {
            qWarning() << getNodeTypeName() << ": Widget is null for property" << name;
            return;
        }

        // 根据控件类型设置值
        if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
            lineEdit->setText(value.toString());
        } else if (auto* comboBox = qobject_cast<QComboBox*>(widget)) {
            int index = value.toInt();
            if (index >= 0 && index < comboBox->count()) {
                comboBox->setCurrentIndex(index);
            }
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(widget)) {
            spinBox->setValue(value.toInt());
        } else if (auto* checkBox = qobject_cast<QCheckBox*>(widget)) {
            checkBox->setChecked(value.toBool());
        } else if (auto* textEdit = qobject_cast<QTextEdit*>(widget)) {
            textEdit->setPlainText(value.toString());
        } else {
            qWarning() << getNodeTypeName() << ": Unsupported widget type for property" << name;
        }
    }

private:
    QList<PropertyInfo> m_properties;
};
