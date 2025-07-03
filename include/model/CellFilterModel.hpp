//
// Created by wuxianggujun on 25-7-3.
//

#pragma once

#include "BaseFilterModel.hpp"
#include "data/CellData.hpp"
#include <QRegularExpression>

/**
 * @brief 单元格数据过滤器
 * 
 * 专门用于过滤 CellData 类型的数据，支持：
 * - 按单元格地址过滤
 * - 按单元格值过滤
 * - 按数据类型过滤
 * - 支持各种比较操作符
 */
class CellFilterModel : public BaseFilterModel<CellData, CellData>
{
    Q_OBJECT

public:
    CellFilterModel();
    ~CellFilterModel() override = default;

    // 节点基本信息
    QString caption() const override { return tr("单元格过滤器"); }
    QString name() const override { return tr("CellFilter"); }

    // 显示名称
    QString getDisplayName() const override { return "单元格过滤器"; }
    QString getDescription() const override { return "根据地址、值或类型过滤单元格数据"; }

protected:
    // 实现基类的纯虚函数
    QStringList getAvailableFields() const override
    {
        return {
            "地址",      // 单元格地址 (A1, B2, etc.)
            "值",        // 单元格值
            "类型",      // 数据类型 (String, Integer, Float, Boolean, Empty)
            "行号",      // 行号
            "列号"       // 列号
        };
    }

    bool evaluateCondition(std::shared_ptr<CellData> data, const FilterCondition& condition) const override
    {
        if (!data || !data->isValid()) {
            return condition.op == FilterOperator::IsNull;
        }

        auto cell = data->cell();
        if (!cell) {
            return condition.op == FilterOperator::IsNull;
        }

        try {
            if (condition.fieldName == "地址") {
                return evaluateAddressCondition(cell, condition);
            } else if (condition.fieldName == "值") {
                return evaluateValueCondition(cell, condition);
            } else if (condition.fieldName == "类型") {
                return evaluateTypeCondition(cell, condition);
            } else if (condition.fieldName == "行号") {
                return evaluateRowCondition(cell, condition);
            } else if (condition.fieldName == "列号") {
                return evaluateColumnCondition(cell, condition);
            }
        } catch (const std::exception& e) {
            qWarning() << "CellFilterModel: 评估条件时出错:" << e.what();
            return false;
        }

        return false;
    }

    std::shared_ptr<CellData> createOutputData(std::shared_ptr<CellData> inputData) const override
    {
        // 直接返回输入数据（过滤器不修改数据内容）
        return inputData;
    }

    QString getNodeTypeName() const override
    {
        return "CellFilterModel";
    }

private:
    // 地址条件评估
    bool evaluateAddressCondition(std::shared_ptr<OpenXLSX::XLCell> cell, const FilterCondition& condition) const
    {
        QString address = QString::fromStdString(cell->cellReference().address());
        QString targetValue = condition.value.toString().toUpper();

        switch (condition.op) {
            case FilterOperator::Equal:
                return address == targetValue;
            case FilterOperator::NotEqual:
                return address != targetValue;
            case FilterOperator::Contains:
                return address.contains(targetValue);
            case FilterOperator::NotContains:
                return !address.contains(targetValue);
            case FilterOperator::StartsWith:
                return address.startsWith(targetValue);
            case FilterOperator::EndsWith:
                return address.endsWith(targetValue);
            case FilterOperator::Matches:
                return QRegularExpression(targetValue).match(address).hasMatch();
            default:
                return false;
        }
    }

    // 值条件评估
    bool evaluateValueCondition(std::shared_ptr<OpenXLSX::XLCell> cell, const FilterCondition& condition) const
    {
        auto valueType = cell->value().type();
        
        if (valueType == OpenXLSX::XLValueType::Empty) {
            return condition.op == FilterOperator::IsNull;
        }

        if (condition.op == FilterOperator::IsNotNull) {
            return valueType != OpenXLSX::XLValueType::Empty;
        }

        // 根据数据类型进行比较
        if (valueType == OpenXLSX::XLValueType::String) {
            return evaluateStringValue(cell->value().get<std::string>(), condition);
        } else if (valueType == OpenXLSX::XLValueType::Integer) {
            return evaluateNumericValue(cell->value().get<int64_t>(), condition);
        } else if (valueType == OpenXLSX::XLValueType::Float) {
            return evaluateNumericValue(cell->value().get<double>(), condition);
        } else if (valueType == OpenXLSX::XLValueType::Boolean) {
            return evaluateBooleanValue(cell->value().get<bool>(), condition);
        }

        return false;
    }

    // 类型条件评估
    bool evaluateTypeCondition(std::shared_ptr<OpenXLSX::XLCell> cell, const FilterCondition& condition) const
    {
        QString cellType = getTypeString(cell->value().type());
        QString targetType = condition.value.toString();

        switch (condition.op) {
            case FilterOperator::Equal:
                return cellType == targetType;
            case FilterOperator::NotEqual:
                return cellType != targetType;
            default:
                return false;
        }
    }

    // 行号条件评估
    bool evaluateRowCondition(std::shared_ptr<OpenXLSX::XLCell> cell, const FilterCondition& condition) const
    {
        uint32_t row = cell->cellReference().row();
        return evaluateNumericValue(static_cast<double>(row), condition);
    }

    // 列号条件评估
    bool evaluateColumnCondition(std::shared_ptr<OpenXLSX::XLCell> cell, const FilterCondition& condition) const
    {
        uint16_t col = cell->cellReference().column();
        return evaluateNumericValue(static_cast<double>(col), condition);
    }

    // 字符串值比较
    bool evaluateStringValue(const std::string& cellValue, const FilterCondition& condition) const
    {
        QString value = QString::fromStdString(cellValue);
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
            default:
                return false;
        }
    }

    // 数值比较
    bool evaluateNumericValue(double cellValue, const FilterCondition& condition) const
    {
        double target = condition.value.toDouble();

        switch (condition.op) {
            case FilterOperator::Equal:
                return qFuzzyCompare(cellValue, target);
            case FilterOperator::NotEqual:
                return !qFuzzyCompare(cellValue, target);
            case FilterOperator::GreaterThan:
                return cellValue > target;
            case FilterOperator::GreaterThanOrEqual:
                return cellValue >= target || qFuzzyCompare(cellValue, target);
            case FilterOperator::LessThan:
                return cellValue < target;
            case FilterOperator::LessThanOrEqual:
                return cellValue <= target || qFuzzyCompare(cellValue, target);
            case FilterOperator::Between: {
                double second = condition.secondValue.toDouble();
                return cellValue >= qMin(target, second) && cellValue <= qMax(target, second);
            }
            case FilterOperator::NotBetween: {
                double second = condition.secondValue.toDouble();
                return cellValue < qMin(target, second) || cellValue > qMax(target, second);
            }
            default:
                return false;
        }
    }

    // 布尔值比较
    bool evaluateBooleanValue(bool cellValue, const FilterCondition& condition) const
    {
        bool target = condition.value.toBool();

        switch (condition.op) {
            case FilterOperator::Equal:
                return cellValue == target;
            case FilterOperator::NotEqual:
                return cellValue != target;
            default:
                return false;
        }
    }

    // 获取类型字符串
    QString getTypeString(OpenXLSX::XLValueType type) const
    {
        switch (type) {
            case OpenXLSX::XLValueType::Empty:
                return "Empty";
            case OpenXLSX::XLValueType::Boolean:
                return "Boolean";
            case OpenXLSX::XLValueType::Integer:
                return "Integer";
            case OpenXLSX::XLValueType::Float:
                return "Float";
            case OpenXLSX::XLValueType::String:
                return "String";
            default:
                return "Unknown";
        }
    }
};
