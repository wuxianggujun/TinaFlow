//
// Created by wuxianggujun on 25-7-3.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "IPropertyProvider.hpp"
#include "widget/PropertyWidget.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <functional>

/**
 * @brief 过滤器操作符枚举
 */
enum class FilterOperator {
    // 数值比较
    Equal,              // 等于
    NotEqual,           // 不等于
    GreaterThan,        // 大于
    GreaterThanOrEqual, // 大于等于
    LessThan,           // 小于
    LessThanOrEqual,    // 小于等于
    
    // 文本匹配
    Contains,           // 包含
    NotContains,        // 不包含
    StartsWith,         // 开头是
    EndsWith,           // 结尾是
    Matches,            // 正则匹配
    
    // 范围
    Between,            // 在范围内
    NotBetween,         // 不在范围内
    
    // 空值
    IsNull,             // 为空
    IsNotNull,          // 不为空
    
    // 列表
    In,                 // 在列表中
    NotIn               // 不在列表中
};

/**
 * @brief 过滤器条件结构
 */
struct FilterCondition {
    QString fieldName;          // 字段名称
    FilterOperator op;          // 操作符
    QVariant value;             // 比较值
    QVariant secondValue;       // 第二个值（用于范围操作）
    
    FilterCondition() : op(FilterOperator::Equal) {}
    FilterCondition(const QString& field, FilterOperator operation, const QVariant& val)
        : fieldName(field), op(operation), value(val) {}
};

/**
 * @brief 过滤器节点基类
 * 
 * 提供通用的过滤功能框架，支持：
 * - 灵活的过滤条件配置
 * - 多种数据类型的过滤
 * - 双输出端口（符合/不符合条件）
 * - 可视化的条件编辑界面
 */
template<typename InputDataType, typename OutputDataType = InputDataType>
class BaseFilterModel : public BaseNodeModel
{
public:
    BaseFilterModel() = default;
    ~BaseFilterModel() override = default;

    // 基本节点信息
    unsigned int nPorts(QtNodes::PortType const portType) const override
    {
        if (portType == QtNodes::PortType::In) {
            return 1; // 一个输入端口
        } else {
            return 2; // 两个输出端口：符合条件、不符合条件
        }
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In && portIndex == 0) {
            return InputDataType().type();
        } else if (portType == QtNodes::PortType::Out) {
            if (portIndex == 0 || portIndex == 1) {
                return OutputDataType().type();
            }
        }
        return QtNodes::NodeDataType();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        if (port == 0) {
            return m_matchedData;
        } else if (port == 1) {
            return m_unmatchedData;
        }
        return nullptr;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        if (portIndex != 0) return;

        auto inputData = std::dynamic_pointer_cast<InputDataType>(nodeData);
        if (!inputData) {
            m_inputData.reset();
            m_matchedData.reset();
            m_unmatchedData.reset();
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            return;
        }

        m_inputData = inputData;
        processFilter();
    }

    // 保存和加载
    QJsonObject save() const override
    {
        QJsonObject json = BaseNodeModel::save();
        
        // 保存过滤条件
        QJsonObject conditionJson;
        conditionJson["fieldName"] = m_condition.fieldName;
        conditionJson["operator"] = static_cast<int>(m_condition.op);
        conditionJson["value"] = QJsonValue::fromVariant(m_condition.value);
        conditionJson["secondValue"] = QJsonValue::fromVariant(m_condition.secondValue);
        
        json["filterCondition"] = conditionJson;
        return json;
    }

    void load(QJsonObject const& json) override
    {
        BaseNodeModel::load(json);
        
        // 加载过滤条件
        if (json.contains("filterCondition")) {
            QJsonObject conditionJson = json["filterCondition"].toObject();
            m_condition.fieldName = conditionJson["fieldName"].toString();
            m_condition.op = static_cast<FilterOperator>(conditionJson["operator"].toInt());
            m_condition.value = conditionJson["value"].toVariant();
            m_condition.secondValue = conditionJson["secondValue"].toVariant();
            
            updateUIFromCondition();
        }
    }

    // IPropertyProvider 接口
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle(getDisplayName());
        propertyWidget->addDescription(getDescription());
        propertyWidget->addModeToggleButtons();

        // 字段选择
        QStringList fieldOptions = getAvailableFields();
        int currentFieldIndex = fieldOptions.indexOf(m_condition.fieldName);
        propertyWidget->addComboProperty("过滤字段", fieldOptions, currentFieldIndex, "fieldName",
            [this](int index) {
                QStringList fields = getAvailableFields();
                if (index >= 0 && index < fields.size()) {
                    m_condition.fieldName = fields[index];
                    processFilter();
                }
            });

        // 操作符选择
        QStringList operatorOptions = getOperatorOptions();
        int currentOpIndex = static_cast<int>(m_condition.op);
        propertyWidget->addComboProperty("操作符", operatorOptions, currentOpIndex, "operator",
            [this](int index) {
                m_condition.op = static_cast<FilterOperator>(index);
                processFilter();
            });

        // 比较值输入
        propertyWidget->addTextProperty("比较值", m_condition.value.toString(), "value", "输入比较值",
            [this](const QString& value) {
                m_condition.value = value;
                processFilter();
            });

        // 范围操作的第二个值
        if (m_condition.op == FilterOperator::Between || m_condition.op == FilterOperator::NotBetween) {
            propertyWidget->addTextProperty("结束值", m_condition.secondValue.toString(), "secondValue", "输入结束值",
                [this](const QString& value) {
                    m_condition.secondValue = value;
                    processFilter();
                });
        }

        return true;
    }

    QString getDisplayName() const override
    {
        return QString("过滤器 (%1)").arg(InputDataType().type().name);
    }

    QString getDescription() const override
    {
        return QString("根据指定条件过滤 %1 类型的数据").arg(InputDataType().type().name);
    }

    QWidget* embeddedWidget() override
    {
        return nullptr; // 过滤器节点不需要嵌入式控件
    }

protected:
    // 子类需要实现的方法
    virtual QStringList getAvailableFields() const = 0;
    virtual bool evaluateCondition(std::shared_ptr<InputDataType> data, const FilterCondition& condition) const = 0;
    virtual std::shared_ptr<OutputDataType> createOutputData(std::shared_ptr<InputDataType> inputData) const = 0;

    // 获取操作符选项
    virtual QStringList getOperatorOptions() const
    {
        return {
            "等于", "不等于", "大于", "大于等于", "小于", "小于等于",
            "包含", "不包含", "开头是", "结尾是", "正则匹配",
            "在范围内", "不在范围内", "为空", "不为空", "在列表中", "不在列表中"
        };
    }

    // 处理过滤逻辑
    void processFilter()
    {
        if (!m_inputData) {
            m_matchedData.reset();
            m_unmatchedData.reset();
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            return;
        }

        bool matches = evaluateCondition(m_inputData, m_condition);
        
        if (matches) {
            m_matchedData = createOutputData(m_inputData);
            m_unmatchedData.reset();
        } else {
            m_matchedData.reset();
            m_unmatchedData = createOutputData(m_inputData);
        }

        Q_EMIT dataUpdated(0);
        Q_EMIT dataUpdated(1);
    }

    // 从条件更新UI（加载时使用）
    virtual void updateUIFromCondition() {}

protected:
    std::shared_ptr<InputDataType> m_inputData;
    std::shared_ptr<OutputDataType> m_matchedData;
    std::shared_ptr<OutputDataType> m_unmatchedData;
    
    FilterCondition m_condition;
};
