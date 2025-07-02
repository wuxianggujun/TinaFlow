//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/BooleanData.hpp"
#include "data/CellData.hpp"
#include "data/IntegerData.hpp"
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
        
        // 注册需要保存的属性
        registerProperty("compareType", nullptr);
        registerProperty("operator", nullptr);
        registerProperty("caseSensitive", nullptr);
    }

    QString caption() const override {
        return tr("通用比较");
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
            return CellData().type(); // 接受任意类型（用CellData作为通用类型）
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
            qDebug() << "UniversalCompareModel: Comparison successful, result:" << result;
            return std::make_shared<BooleanData>(result);
        } else {
            setError(errorMsg);
            qDebug() << "UniversalCompareModel: Comparison failed:" << errorMsg;
            return std::make_shared<BooleanData>(false);
        }
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex portIndex) override {
        qDebug() << "UniversalCompareModel: setInData called, portIndex:" << portIndex;
        
        if (portIndex < m_inputData.size()) {
            m_inputData[portIndex] = nodeData;
            
            if (nodeData) {
                QString dataType = nodeData->type().name;
                qDebug() << "UniversalCompareModel: Received" << dataType << "at port" << portIndex;
            } else {
                qDebug() << "UniversalCompareModel: Received null data at port" << portIndex;
            }
        }
        
        // 重新计算并更新显示
        updateErrorDisplay();
        emit dataUpdated(0);
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            m_widget = new QWidget();
            auto layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);
            layout->setSpacing(2);

            // 错误提示标签
            m_errorLabel = new QLabel();
            m_errorLabel->setStyleSheet("color: red; font-size: 9px; font-weight: bold;");
            m_errorLabel->setWordWrap(true);
            m_errorLabel->hide();
            layout->addWidget(m_errorLabel);

            // 比较类型选择
            auto typeLabel = new QLabel("比较类型:");
            typeLabel->setStyleSheet("font-weight: bold; font-size: 10px;");
            layout->addWidget(typeLabel);

            m_typeCombo = new QComboBox();
            m_typeCombo->addItems({"自动检测", "字符串", "数值", "布尔值"});
            m_typeCombo->setCurrentIndex(static_cast<int>(m_compareType));
            m_typeCombo->setStyleSheet("font-size: 10px;");

            connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [this](int index) {
                        m_compareType = static_cast<CompareType>(index);
                        updateOperatorOptions();
                        updateErrorDisplay();
                        emit dataUpdated(0);
                    });

            layout->addWidget(m_typeCombo);

            // 比较操作符选择
            auto opLabel = new QLabel("操作符:");
            opLabel->setStyleSheet("font-weight: bold; font-size: 10px;");
            layout->addWidget(opLabel);

            m_operatorCombo = new QComboBox();
            updateOperatorOptions();
            m_operatorCombo->setStyleSheet("font-size: 10px;");

            connect(m_operatorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [this](int index) {
                        m_operator = static_cast<CompareOperator>(index);
                        emit dataUpdated(0);
                    });

            layout->addWidget(m_operatorCombo);

            // 大小写敏感选项（仅字符串比较时显示）
            m_caseSensitiveCheck = new QCheckBox("区分大小写");
            m_caseSensitiveCheck->setChecked(m_caseSensitive);
            m_caseSensitiveCheck->setStyleSheet("font-size: 10px;");

            connect(m_caseSensitiveCheck, &QCheckBox::toggled,
                    [this](bool checked) {
                        m_caseSensitive = checked;
                        emit dataUpdated(0);
                    });

            layout->addWidget(m_caseSensitiveCheck);
            updateCaseSensitiveVisibility();
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
        qDebug() << "UniversalCompareModel: Error:" << message;
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
            if (value.canConvert<double>()) return "数值";
            return "字符串";
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
        if (!m_operatorCombo) return;

        m_operatorCombo->clear();

        if (m_compareType == String || (m_compareType == Auto)) {
            m_operatorCombo->addItems({"==", "!=", "包含", "开始于", "结束于"});
        } else {
            m_operatorCombo->addItems({"==", "!=", ">", "<", ">=", "<="});
        }

        // 重置操作符选择
        if (m_operator >= m_operatorCombo->count()) {
            m_operator = Equal;
        }
        m_operatorCombo->setCurrentIndex(static_cast<int>(m_operator));
    }

    void updateCaseSensitiveVisibility() {
        if (!m_caseSensitiveCheck) return;

        // 只有字符串比较时才显示大小写敏感选项
        bool showCaseSensitive = (m_compareType == String) ||
                                (m_compareType == Auto && hasStringInputs());
        m_caseSensitiveCheck->setVisible(showCaseSensitive);
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
        } else if (auto boolData = std::dynamic_pointer_cast<BooleanData>(data)) {
            return boolData->value();
        } else if (auto intData = std::dynamic_pointer_cast<IntegerData>(data)) {
            return intData->value() != 0;
        }
        return false;
    }
};
