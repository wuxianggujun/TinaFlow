//
// Created by wuxianggujun on 25-7-3.
//

#include "model/ValueFilterModel.hpp"
#include <QDebug>

// ValueDataFilterModel 实现
ValueDataFilterModel::ValueDataFilterModel()
{
    m_condition.fieldName = "值";
    m_condition.op = FilterOperator::Equal;
    m_condition.value = "";

    qDebug() << "ValueDataFilterModel created";
}

// IntegerFilterModel 实现
IntegerFilterModel::IntegerFilterModel()
{
    m_condition.fieldName = "值";
    m_condition.op = FilterOperator::Equal;
    m_condition.value = 0;

    qDebug() << "IntegerFilterModel created";
}

// BooleanFilterModel 实现
BooleanFilterModel::BooleanFilterModel()
{
    m_condition.fieldName = "值";
    m_condition.op = FilterOperator::Equal;
    m_condition.value = true;

    qDebug() << "BooleanFilterModel created";
}


