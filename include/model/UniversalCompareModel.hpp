//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/BooleanData.hpp"
#include "data/CellData.hpp"
#include "data/IntegerData.hpp"
#include "data/ValueData.hpp"
#include "widget/PropertyWidget.hpp"
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QColor>
#include <QToolTip>

/**
 * @brief 通用比较节点
 * 
 * 功能：
 * - 支持多种数据类型的比较（字符串、数值、布尔值）
 * - 自动类型检查和转换
 * - 类型不匹配时显示错误提示
 * - 手动指定比较类型
 * 
 * 输入端口：
 * - 0: 任意类型 - 左操作数
 * - 1: 任意类型 - 右操作数
 * 
 * 输出端口：
 * - 0: BooleanData - 比较结果
 */
class UniversalCompareModel : public BaseNodeModel
{
    Q_OBJECT

public:
    enum CompareType {
        Auto = 0,         // 自动检测类型
        String = 1,       // 强制字符串比较
        Number = 2,       // 强制数值比较
        Boolean = 3       // 强制布尔值比较
    };

    enum CompareOperator {
        Equal = 0,        // ==
        NotEqual = 1,     // !=
        Greater = 2,      // >
        Less = 3,         // <
        GreaterEqual = 4, // >=
        LessEqual = 5,    // <=
        Contains = 6,     // 包含（仅字符串）
        StartsWith = 7,   // 开始于（仅字符串）
        EndsWith = 8      // 结束于（仅字符串）
    };

    UniversalCompareModel() : m_compareType(Auto), m_operator(Equal), m_caseSensitive(false), m_hasError(false) {
        m_inputData.resize(2);
        // 属性将在embeddedWidget()创建后注册
    }

    QString caption() const override {
        return tr("智能比较");
    }

    bool captionVisible() const override {
        return true;
    }

    QString name() const override {
        return tr("UniversalCompare");
    }

    unsigned int nPorts(QtNodes::PortType portType) const override {
        return portType == QtNodes::PortType::In ? 2 : 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        if (portType == QtNodes::PortType::In) {
            // 输入端口接受 ValueData 的所有变体类型
            // 使用基础的 value 类型，这样可以匹配所有 value_* 类型
            return {"value", "值"};
        } else {
            return BooleanData().type();
        }
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override {
        if (port != 0) return nullptr;

        auto leftData = getInputData(0);
        auto rightData = getInputData(1);

        if (!leftData || !rightData) {
            setError("缺少输入数据");
            return std::make_shared<BooleanData>(false);
        }

        // 检查类型兼容性并执行比较
        bool result = false;
        QString errorMsg;
        
        if (performComparison(leftData, rightData, result, errorMsg)) {
            clearError();
            return std::make_shared<BooleanData>(result);
        } else {
            setError(errorMsg);
            return std::make_shared<BooleanData>(false);
        }
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex portIndex) override {
        if (portIndex < m_inputData.size()) {
            m_inputData[portIndex] = nodeData;
        }

        // 只有当两个输入都有数据时才更新错误显示和触发计算
        // 这样可以避免不必要的重复计算
        updateErrorDisplay();

        // 只有当有有效的比较结果时才触发更新
        auto leftData = getInputData(0);
        auto rightData = getInputData(1);
        if (leftData && rightData) {
            emit dataUpdated(0);
        }
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            // 直接使用QComboBox作为主控件，避免容器嵌套导致的下拉框问题
            m_typeCombo = new QComboBox();
            m_typeCombo->addItems({"自动检测", "字符串", "数值", "布尔值"});
            m_typeCombo->setCurrentIndex(static_cast<int>(m_compareType));
            m_typeCombo->setStyleSheet("font-size: 10px;");
            m_typeCombo->setToolTip("选择比较数据类型\n操作符和其他设置请在属性面板中调整");

            connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [this](int index) {
                        m_compareType = static_cast<CompareType>(index);
                        updateOperatorOptions();
                        updateErrorDisplay();
                        emit dataUpdated(0);
                    });

            // 注册属性控件
            registerProperty("compareType", m_typeCombo);

            m_widget = m_typeCombo;
        }
        return m_widget;
    }

    QJsonObject save() const override {
        QJsonObject modelJson = BaseNodeModel::save();
        modelJson["compareType"] = static_cast<int>(m_compareType);
        modelJson["operator"] = static_cast<int>(m_operator);
        modelJson["caseSensitive"] = m_caseSensitive;
        return modelJson;
    }

    void load(QJsonObject const& json) override {
        BaseNodeModel::load(json);
        if (json.contains("compareType")) {
            m_compareType = static_cast<CompareType>(json["compareType"].toInt());
            if (m_typeCombo) {
                m_typeCombo->setCurrentIndex(static_cast<int>(m_compareType));
                updateOperatorOptions();
            }
        }
        if (json.contains("operator")) {
            m_operator = static_cast<CompareOperator>(json["operator"].toInt());
            if (m_operatorCombo) {
                m_operatorCombo->setCurrentIndex(static_cast<int>(m_operator));
            }
        }
        if (json.contains("caseSensitive")) {
            m_caseSensitive = json["caseSensitive"].toBool();
            if (m_caseSensitiveCheck) {
                m_caseSensitiveCheck->setChecked(m_caseSensitive);
            }
        }
        updateCaseSensitiveVisibility();
    }

    // IPropertyProvider接口实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override {
        if (!propertyWidget) return false;
        
        propertyWidget->addTitle("通用比较设置");
        propertyWidget->addDescription("支持多种数据类型的智能比较");
        
        // 比较类型设置
        QStringList types = {"自动检测", "字符串", "数值", "布尔值"};
        propertyWidget->addComboProperty("比较类型", types,
            static_cast<int>(m_compareType), "compareType",
            [this](int index) {
                if (index >= 0 && index < 4) {
                    m_compareType = static_cast<CompareType>(index);
                    if (m_typeCombo) {
                        m_typeCombo->setCurrentIndex(index);
                        updateOperatorOptions();
                        updateCaseSensitiveVisibility();
                    }
                    updateErrorDisplay();
                    emit dataUpdated(0);
                }
            });
        
        // 显示当前状态
        if (m_hasError) {
            propertyWidget->addInfoProperty("状态", m_errorMessage, "color: red; font-weight: bold;");
        } else {
            propertyWidget->addInfoProperty("状态", "正常", "color: green;");
        }
        
        return true;
    }
    
    QString getDisplayName() const override {
        return tr("通用比较");
    }

protected:
    QString getNodeTypeName() const override {
        return "UniversalCompare";
    }

private:
    CompareType m_compareType;
    CompareOperator m_operator;
    bool m_caseSensitive;
    bool m_hasError;
    QString m_errorMessage;
    
    QWidget* m_widget = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QComboBox* m_operatorCombo = nullptr;
    QCheckBox* m_caseSensitiveCheck = nullptr;
    QLabel* m_errorLabel = nullptr;
    
    std::vector<std::shared_ptr<QtNodes::NodeData>> m_inputData;

    std::shared_ptr<QtNodes::NodeData> getInputData(QtNodes::PortIndex index) {
        if (index < m_inputData.size()) {
            return m_inputData[index];
        }
        return nullptr;
    }

    void setError(const QString& message) {
        m_hasError = true;
        m_errorMessage = message;
        if (m_errorLabel) {
            m_errorLabel->setText(message);
            m_errorLabel->show();
        }
        // 可以在这里设置节点为红色
    }

    void clearError() {
        m_hasError = false;
        m_errorMessage.clear();
        if (m_errorLabel) {
            m_errorLabel->hide();
        }
    }

    void updateErrorDisplay() {
        // 检查当前输入是否有类型冲突
        auto leftData = getInputData(0);
        auto rightData = getInputData(1);
        
        if (leftData && rightData) {
            QString leftType = getDataTypeName(leftData);
            QString rightType = getDataTypeName(rightData);
            
            if (m_compareType == Auto) {
                if (!areTypesCompatible(leftData, rightData)) {
                    setError(QString("类型不兼容: %1 vs %2").arg(leftType, rightType));
                    return;
                }
            }
        }
        
        clearError();
    }

    QString getDataTypeName(std::shared_ptr<QtNodes::NodeData> data) {
        if (auto cellData = std::dynamic_pointer_cast<CellData>(data)) {
            QVariant value = cellData->value();
            if (value.type() == QVariant::Bool) return "布尔值";

            // 更严格的数值检查：只有当字符串确实是有效数字时才认为是数值
            if (value.type() == QVariant::Int || value.type() == QVariant::Double ||
                value.type() == QVariant::LongLong || value.type() == QVariant::UInt ||
                value.type() == QVariant::ULongLong) {
                return "数值";
            }

            // 对于字符串类型，检查是否为纯数字
            if (value.type() == QVariant::String) {
                QString str = value.toString().trimmed();
                if (!str.isEmpty()) {
                    bool ok;
                    str.toDouble(&ok);
                    if (ok) return "数值";
                }
            }

            return "字符串";
        } else if (auto valueData = std::dynamic_pointer_cast<ValueData>(data)) {
            // 新的 ValueData 类型支持
            switch (valueData->valueType()) {
                case ValueData::String: return "字符串";
                case ValueData::Number: return "数值";
                case ValueData::Boolean: return "布尔值";
                default: return "未知";
            }
        } else if (std::dynamic_pointer_cast<BooleanData>(data)) {
            return "布尔值";
        } else if (std::dynamic_pointer_cast<IntegerData>(data)) {
            return "数值";
        }
        return "未知";
    }

    bool areTypesCompatible(std::shared_ptr<QtNodes::NodeData> left, std::shared_ptr<QtNodes::NodeData> right) {
        QString leftType = getDataTypeName(left);
        QString rightType = getDataTypeName(right);

        // 数值类型之间兼容
        if ((leftType == "数值" || leftType == "布尔值") && (rightType == "数值" || rightType == "布尔值")) {
            return true;
        }

        // 相同类型兼容
        return leftType == rightType;
    }

    void updateOperatorOptions() {
        // 现在操作符选择在属性面板中处理，这里只需要重置操作符值
        if (m_compareType == String || m_compareType == Auto) {
            // 字符串操作符：==, !=, 包含, 开始于, 结束于
            if (m_operator > EndsWith) {
                m_operator = Equal;
            }
        } else {
            // 数值操作符：==, !=, >, <, >=, <=
            if (m_operator > LessEqual) {
                m_operator = Equal;
            }
        }
    }

    void updateCaseSensitiveVisibility() {
        // 大小写敏感选项现在在属性面板中处理
        // 这里不需要做任何操作
    }

    bool hasStringInputs() {
        auto leftData = getInputData(0);
        auto rightData = getInputData(1);

        if (leftData && rightData) {
            return getDataTypeName(leftData) == "字符串" || getDataTypeName(rightData) == "字符串";
        }
        return false;
    }

    bool performComparison(std::shared_ptr<QtNodes::NodeData> left,
                          std::shared_ptr<QtNodes::NodeData> right,
                          bool& result, QString& errorMsg) {

        CompareType actualType = m_compareType;

        // 自动检测类型
        if (actualType == Auto) {
            if (!areTypesCompatible(left, right)) {
                errorMsg = QString("无法比较 %1 和 %2").arg(getDataTypeName(left), getDataTypeName(right));
                return false;
            }

            QString leftType = getDataTypeName(left);
            if (leftType == "字符串") actualType = String;
            else if (leftType == "数值") actualType = Number;
            else if (leftType == "布尔值") actualType = Boolean;
        }

        // 根据类型执行比较
        switch (actualType) {
            case String:
                return compareAsString(left, right, result, errorMsg);
            case Number:
                return compareAsNumber(left, right, result, errorMsg);
            case Boolean:
                return compareAsBoolean(left, right, result, errorMsg);
            default:
                errorMsg = "未知比较类型";
                return false;
        }
    }

    bool compareAsString(std::shared_ptr<QtNodes::NodeData> left,
                        std::shared_ptr<QtNodes::NodeData> right,
                        bool& result, QString& errorMsg) {
        QString leftStr = extractStringValue(left);
        QString rightStr = extractStringValue(right);

        Qt::CaseSensitivity cs = m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;

        switch (m_operator) {
            case Equal: result = leftStr.compare(rightStr, cs) == 0; break;
            case NotEqual: result = leftStr.compare(rightStr, cs) != 0; break;
            case Contains: result = leftStr.contains(rightStr, cs); break;
            case StartsWith: result = leftStr.startsWith(rightStr, cs); break;
            case EndsWith: result = leftStr.endsWith(rightStr, cs); break;
            default:
                errorMsg = "字符串不支持此操作符";
                return false;
        }
        return true;
    }

    bool compareAsNumber(std::shared_ptr<QtNodes::NodeData> left,
                        std::shared_ptr<QtNodes::NodeData> right,
                        bool& result, QString& errorMsg) {
        double leftNum = extractNumericValue(left);
        double rightNum = extractNumericValue(right);

        switch (m_operator) {
            case Equal: result = qAbs(leftNum - rightNum) < 1e-9; break;
            case NotEqual: result = qAbs(leftNum - rightNum) >= 1e-9; break;
            case Greater: result = leftNum > rightNum; break;
            case Less: result = leftNum < rightNum; break;
            case GreaterEqual: result = leftNum >= rightNum; break;
            case LessEqual: result = leftNum <= rightNum; break;
            default:
                errorMsg = "数值不支持此操作符";
                return false;
        }
        return true;
    }

    bool compareAsBoolean(std::shared_ptr<QtNodes::NodeData> left,
                         std::shared_ptr<QtNodes::NodeData> right,
                         bool& result, QString& errorMsg) {
        bool leftBool = extractBooleanValue(left);
        bool rightBool = extractBooleanValue(right);

        switch (m_operator) {
            case Equal: result = leftBool == rightBool; break;
            case NotEqual: result = leftBool != rightBool; break;
            default:
                errorMsg = "布尔值只支持相等和不相等比较";
                return false;
        }
        return true;
    }

    QString extractStringValue(std::shared_ptr<QtNodes::NodeData> data) {
        if (auto cellData = std::dynamic_pointer_cast<CellData>(data)) {
            return cellData->value().toString();
        } else if (auto valueData = std::dynamic_pointer_cast<ValueData>(data)) {
            return valueData->toString();
        } else if (auto boolData = std::dynamic_pointer_cast<BooleanData>(data)) {
            return boolData->value() ? "true" : "false";
        } else if (auto intData = std::dynamic_pointer_cast<IntegerData>(data)) {
            return QString::number(intData->value());
        }
        return QString();
    }

    double extractNumericValue(std::shared_ptr<QtNodes::NodeData> data) {
        if (auto cellData = std::dynamic_pointer_cast<CellData>(data)) {
            return cellData->value().toDouble();
        } else if (auto valueData = std::dynamic_pointer_cast<ValueData>(data)) {
            return valueData->toDouble();
        } else if (auto boolData = std::dynamic_pointer_cast<BooleanData>(data)) {
            return boolData->value() ? 1.0 : 0.0;
        } else if (auto intData = std::dynamic_pointer_cast<IntegerData>(data)) {
            return static_cast<double>(intData->value());
        }
        return 0.0;
    }

    bool extractBooleanValue(std::shared_ptr<QtNodes::NodeData> data) {
        if (auto cellData = std::dynamic_pointer_cast<CellData>(data)) {
            return cellData->value().toBool();
        } else if (auto valueData = std::dynamic_pointer_cast<ValueData>(data)) {
            return valueData->toBool();
        } else if (auto boolData = std::dynamic_pointer_cast<BooleanData>(data)) {
            return boolData->value();
        } else if (auto intData = std::dynamic_pointer_cast<IntegerData>(data)) {
            return intData->value() != 0;
        }
        return false;
    }
};
