//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include <QString>
#include <QtNodes/NodeData>

/**
 * @brief 封装布尔值和条件结果的数据类
 * 
 * 这个类用于在节点之间传递布尔值，通常用于条件判断的结果。
 * 包含布尔值本身以及相关的描述信息。
 */
class BooleanData : public QtNodes::NodeData
{
public:
    BooleanData() = default;

    /**
     * @brief 构造函数
     * @param value 布尔值
     * @param description 描述信息（可选）
     */
    explicit BooleanData(bool value, const QString& description = QString())
        : m_value(value), m_description(description)
    {
    }

    /**
     * @brief 返回节点数据类型
     */
    QtNodes::NodeDataType type() const override
    {
        return {"boolean", "Boolean"};
    }

    /**
     * @brief 获取布尔值
     * @return 布尔值
     */
    bool value() const
    {
        return m_value;
    }

    /**
     * @brief 设置布尔值
     * @param value 布尔值
     */
    void setValue(bool value)
    {
        m_value = value;
    }

    /**
     * @brief 获取描述信息
     * @return 描述字符串
     */
    const QString& description() const
    {
        return m_description;
    }

    /**
     * @brief 设置描述信息
     * @param description 描述字符串
     */
    void setDescription(const QString& description)
    {
        m_description = description;
    }

    /**
     * @brief 获取值的字符串表示
     * @return "true" 或 "false"
     */
    QString valueAsString() const
    {
        return m_value ? "true" : "false";
    }

    /**
     * @brief 获取本地化的字符串表示
     * @return "真" 或 "假"
     */
    QString localizedString() const
    {
        return m_value ? "真" : "假";
    }

    /**
     * @brief 获取调试信息字符串
     * @return 包含值和描述的调试信息
     */
    QString debugString() const
    {
        if (m_description.isEmpty()) {
            return QString("Boolean[%1]").arg(valueAsString());
        } else {
            return QString("Boolean[%1]: %2").arg(valueAsString()).arg(m_description);
        }
    }

    /**
     * @brief 逻辑非操作
     * @return 取反后的BooleanData
     */
    BooleanData operator!() const
    {
        return BooleanData(!m_value, QString("NOT (%1)").arg(m_description));
    }

    /**
     * @brief 逻辑与操作
     * @param other 另一个BooleanData
     * @return 逻辑与的结果
     */
    BooleanData operator&&(const BooleanData& other) const
    {
        bool result = m_value && other.m_value;
        QString desc = QString("(%1) AND (%2)").arg(m_description).arg(other.m_description);
        return BooleanData(result, desc);
    }

    /**
     * @brief 逻辑或操作
     * @param other 另一个BooleanData
     * @return 逻辑或的结果
     */
    BooleanData operator||(const BooleanData& other) const
    {
        bool result = m_value || other.m_value;
        QString desc = QString("(%1) OR (%2)").arg(m_description).arg(other.m_description);
        return BooleanData(result, desc);
    }

private:
    bool m_value = false;           ///< 布尔值
    QString m_description;          ///< 描述信息
};
