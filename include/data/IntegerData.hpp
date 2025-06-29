//
// Created by wuxianggujun on 25-6-29.
//

#pragma once

#include <QtNodes/NodeData>

/**
 * @brief 整数数据类型
 * 
 * 用于在节点之间传递整数值，如行索引、计数器等
 */
class IntegerData : public QtNodes::NodeData
{
public:
    IntegerData() = default;
    IntegerData(int value) : m_value(value) {}
    
    QtNodes::NodeDataType type() const override
    {
        return {"IntegerData", "整数"};
    }
    
    int value() const { return m_value; }
    void setValue(int value) { m_value = value; }

private:
    int m_value = 0;
};
