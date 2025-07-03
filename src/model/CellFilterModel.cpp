//
// Created by wuxianggujun on 25-7-3.
//

#include "model/CellFilterModel.hpp"
#include <QDebug>

CellFilterModel::CellFilterModel()
{
    // 设置默认过滤条件
    m_condition.fieldName = "值";
    m_condition.op = FilterOperator::Equal;
    m_condition.value = "";

    qDebug() << "CellFilterModel created";
}
