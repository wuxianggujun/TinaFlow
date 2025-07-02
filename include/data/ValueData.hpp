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
        // 统一返回 "value" 类型，具体类型信息通过 valueType() 方法获取
        // 这样可以让所有 ValueData 都能连接到接受 "value" 类型的端口
        return {"value", "值"};
    }

    // 获取值
    QVariant value() const { return m_value; }
    
    // 获取类型
    ValueType valueType() const { return m_type; }
    
    // 类型转换方法
    QString toString() const {
        switch (m_type) {
            case String: return m_value.toString();
            case Number: return QString::number(m_value.toDouble());
            case Boolean: return m_value.toBool() ? "true" : "false";
            default: return m_value.toString();
        }
    }

    double toDouble() const {
        switch (m_type) {
            case Number: return m_value.toDouble();
            case Boolean: return m_value.toBool() ? 1.0 : 0.0;
            case String: {
                bool ok;
                double result = m_value.toString().toDouble(&ok);
                return ok ? result : 0.0;
            }
            default: return 0.0;
        }
    }

    bool toBool() const {
        switch (m_type) {
            case Boolean: return m_value.toBool();
            case Number: return qAbs(m_value.toDouble()) > 1e-9;
            case String: {
                QString str = m_value.toString().toLower().trimmed();
                return str == "true" || str == "1" || str == "yes";
            }
            default: return false;
        }
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
