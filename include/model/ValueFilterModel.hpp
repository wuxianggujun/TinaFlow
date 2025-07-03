//
// Created by wuxianggujun on 25-7-3.
//

#pragma once

#include "BaseFilterModel.hpp"
#include "data/ValueData.hpp"
#include "data/IntegerData.hpp"
#include "data/BooleanData.hpp"
#include <QRegularExpression>

/**
 * @brief 通用值过滤器模板
 * 
 * 用于过滤基本数据类型（字符串、整数、浮点数、布尔值）
 */
template<typename DataType>
class ValueFilterModel : public BaseFilterModel<DataType, DataType>
{
public:
    ValueFilterModel() = default;
    ~ValueFilterModel() override = default;

protected:
    QStringList getAvailableFields() const override
    {
        return {"值"}; // 基本数据类型只有一个字段
    }

    bool evaluateCondition(std::shared_ptr<DataType> data, const FilterCondition& condition) const override
    {
        if (!data) {
            return condition.op == FilterOperator::IsNull;
        }

        if (condition.op == FilterOperator::IsNotNull) {
            return true;
        }

        // 根据数据类型进行不同的比较
        return evaluateValue(data, condition);
    }

    std::shared_ptr<DataType> createOutputData(std::shared_ptr<DataType> inputData) const override
    {
        return inputData; // 直接返回输入数据
    }

private:
    virtual bool evaluateValue(std::shared_ptr<DataType> data, const FilterCondition& condition) const = 0;
};

/**
 * @brief 值过滤器（支持字符串、数值、布尔值）
 */
class ValueDataFilterModel : public ValueFilterModel<ValueData>
{
    Q_OBJECT

public:
    ValueDataFilterModel();
    ~ValueDataFilterModel() override = default;

    QString caption() const override { return tr("值过滤器"); }
    QString name() const override { return tr("ValueFilter"); }
    QString getDisplayName() const override { return "值过滤器"; }
    QString getDescription() const override { return "根据条件过滤值数据（字符串、数值、布尔值）"; }

protected:
    QString getNodeTypeName() const override { return "ValueDataFilterModel"; }

    QStringList getOperatorOptions() const override
    {
        return {
            "等于", "不等于", "包含", "不包含", "开头是", "结尾是", 
            "正则匹配", "为空", "不为空"
        };
    }

private:
    bool evaluateValue(std::shared_ptr<ValueData> data, const FilterCondition& condition) const override
    {
        // 根据 ValueData 的类型进行不同的比较
        ValueData::ValueType dataType = data->valueType();

        if (dataType == ValueData::String) {
            QString value = data->toString();
            QString target = condition.value.toString();

            switch (condition.op) {
                case FilterOperator::Equal:
                    return value == target;
                case FilterOperator::NotEqual:
                    return value != target;
                case FilterOperator::Contains:
                    return value.contains(target, Qt::CaseInsensitive);
                case FilterOperator::NotContains:
                    return !value.contains(target, Qt::CaseInsensitive);
                case FilterOperator::StartsWith:
                    return value.startsWith(target, Qt::CaseInsensitive);
                case FilterOperator::EndsWith:
                    return value.endsWith(target, Qt::CaseInsensitive);
                case FilterOperator::Matches:
                    return QRegularExpression(target).match(value).hasMatch();
                case FilterOperator::IsNull:
                    return value.isEmpty();
                case FilterOperator::IsNotNull:
                    return !value.isEmpty();
                default:
                    return false;
            }
        } else if (dataType == ValueData::Number) {
            double value = data->toDouble();
            double target = condition.value.toDouble();

            switch (condition.op) {
                case FilterOperator::Equal:
                    return qFuzzyCompare(value, target);
                case FilterOperator::NotEqual:
                    return !qFuzzyCompare(value, target);
                case FilterOperator::GreaterThan:
                    return value > target;
                case FilterOperator::GreaterThanOrEqual:
                    return value >= target || qFuzzyCompare(value, target);
                case FilterOperator::LessThan:
                    return value < target;
                case FilterOperator::LessThanOrEqual:
                    return value <= target || qFuzzyCompare(value, target);
                case FilterOperator::Between: {
                    double second = condition.secondValue.toDouble();
                    return value >= qMin(target, second) && value <= qMax(target, second);
                }
                case FilterOperator::NotBetween: {
                    double second = condition.secondValue.toDouble();
                    return value < qMin(target, second) || value > qMax(target, second);
                }
                default:
                    return false;
            }
        } else if (dataType == ValueData::Boolean) {
            bool value = data->toBool();
            bool target = condition.value.toBool();

            switch (condition.op) {
                case FilterOperator::Equal:
                    return value == target;
                case FilterOperator::NotEqual:
                    return value != target;
                default:
                    return false;
            }
        }

        return false;
    }
};

/**
 * @brief 整数过滤器
 */
class IntegerFilterModel : public ValueFilterModel<IntegerData>
{
    Q_OBJECT

public:
    IntegerFilterModel();
    ~IntegerFilterModel() override = default;

    QString caption() const override { return tr("整数过滤器"); }
    QString name() const override { return tr("IntegerFilter"); }
    QString getDisplayName() const override { return "整数过滤器"; }
    QString getDescription() const override { return "根据条件过滤整数数据"; }

protected:
    QString getNodeTypeName() const override { return "IntegerFilterModel"; }

    QStringList getOperatorOptions() const override
    {
        return {
            "等于", "不等于", "大于", "大于等于", "小于", "小于等于",
            "在范围内", "不在范围内"
        };
    }

private:
    bool evaluateValue(std::shared_ptr<IntegerData> data, const FilterCondition& condition) const override
    {
        int value = data->value();
        int target = condition.value.toInt();

        switch (condition.op) {
            case FilterOperator::Equal:
                return value == target;
            case FilterOperator::NotEqual:
                return value != target;
            case FilterOperator::GreaterThan:
                return value > target;
            case FilterOperator::GreaterThanOrEqual:
                return value >= target;
            case FilterOperator::LessThan:
                return value < target;
            case FilterOperator::LessThanOrEqual:
                return value <= target;
            case FilterOperator::Between: {
                int second = condition.secondValue.toInt();
                return value >= qMin(target, second) && value <= qMax(target, second);
            }
            case FilterOperator::NotBetween: {
                int second = condition.secondValue.toInt();
                return value < qMin(target, second) || value > qMax(target, second);
            }
            default:
                return false;
        }
    }
};

/**
 * @brief 布尔值过滤器
 */
class BooleanFilterModel : public ValueFilterModel<BooleanData>
{
    Q_OBJECT

public:
    BooleanFilterModel();
    ~BooleanFilterModel() override = default;

    QString caption() const override { return tr("布尔值过滤器"); }
    QString name() const override { return tr("BooleanFilter"); }
    QString getDisplayName() const override { return "布尔值过滤器"; }
    QString getDescription() const override { return "根据条件过滤布尔值数据"; }

protected:
    QString getNodeTypeName() const override { return "BooleanFilterModel"; }

    QStringList getOperatorOptions() const override
    {
        return {"等于", "不等于"};
    }

private:
    bool evaluateValue(std::shared_ptr<BooleanData> data, const FilterCondition& condition) const override
    {
        bool value = data->value();
        bool target = condition.value.toBool();

        switch (condition.op) {
            case FilterOperator::Equal:
                return value == target;
            case FilterOperator::NotEqual:
                return value != target;
            default:
                return false;
        }
    }
};
