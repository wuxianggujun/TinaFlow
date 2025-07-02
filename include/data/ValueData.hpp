//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include <QtNodes/NodeData>
#include <QVariant>
#include <QString>

/**
 * @brief 通用值数据类型
 * 
 * 用于传递各种类型的常量值，比 CellData 更通用
 * 支持字符串、数值、布尔值等基本类型
 */
class ValueData : public QtNodes::NodeData
{
public:
    enum ValueType {
        String = 0,
        Number = 1,
        Boolean = 2
    };

    ValueData() = default;

    // 字符串构造函数
    explicit ValueData(const QString& stringValue)
        : m_value(stringValue), m_type(String)
    {
    }

    // 数值构造函数
    explicit ValueData(double numberValue)
        : m_value(numberValue), m_type(Number)
    {
    }

    // 整数构造函数
    explicit ValueData(int numberValue)
        : m_value(numberValue), m_type(Number)
    {
    }

    // 布尔值构造函数
    explicit ValueData(bool booleanValue)
        : m_value(booleanValue), m_type(Boolean)
    {
    }

    // 通用构造函数
    ValueData(const QVariant& value, ValueType type)
        : m_value(value), m_type(type)
    {
    }

    QtNodes::NodeDataType type() const override
    {
        switch (m_type) {
            case String: return {"value_string", "Value(字符串)"};
            case Number: return {"value_number", "Value(数值)"};
            case Boolean: return {"value_boolean", "Value(布尔值)"};
            default: return {"value", "Value"};
        }
    }

    // 获取值
    QVariant value() const { return m_value; }
    
    // 获取类型
    ValueType valueType() const { return m_type; }
    
    // 类型转换方法
    QString toString() const {
        return m_value.toString();
    }
    
    double toDouble() const {
        return m_value.toDouble();
    }
    
    bool toBool() const {
        return m_value.toBool();
    }
    
    // 检查是否有效
    bool isValid() const {
        return m_value.isValid();
    }

    // 获取类型显示名称
    QString getTypeDisplayName() const {
        switch (m_type) {
            case String: return "字符串";
            case Number: return "数值";
            case Boolean: return "布尔值";
            default: return "未知";
        }
    }

private:
    QVariant m_value;
    ValueType m_type = String;
};
