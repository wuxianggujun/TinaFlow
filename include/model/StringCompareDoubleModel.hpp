//
// Created by wuxianggujun on 25-7-2.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/BooleanData.hpp"
#include "data/CellData.hpp"
#include "widget/PropertyWidget.hpp"
#include <QComboBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>

/**
 * @brief 双输入字符串比较节点
 * 
 * 功能：
 * - 比较两个字符串输入的关系
 * - 支持多种比较类型（相等、包含、开始于等）
 * - 支持大小写敏感设置
 * 
 * 输入端口：
 * - 0: CellData - 左字符串
 * - 1: CellData - 右字符串
 * 
 * 输出端口：
 * - 0: BooleanData - 比较结果
 */
class StringCompareDoubleModel : public BaseNodeModel
{
    Q_OBJECT

public:
    enum CompareType {
        Equal = 0,        // 相等
        NotEqual = 1,     // 不相等
        Contains = 2,     // 包含
        StartsWith = 3,   // 开始于
        EndsWith = 4,     // 结束于
        IsEmpty = 5       // 为空（只检查左操作数）
    };

    StringCompareDoubleModel() : m_compareType(Equal), m_caseSensitive(false) {
        m_inputData.resize(2);
        
        // 注册需要保存的属性
        registerProperty("compareType", nullptr);
        registerProperty("caseSensitive", nullptr);
    }

    QString caption() const override {
        return tr("字符串比较");
    }

    bool captionVisible() const override {
        return true;
    }

    QString name() const override {
        return tr("StringCompare");
    }

    unsigned int nPorts(QtNodes::PortType portType) const override {
        return portType == QtNodes::PortType::In ? 2 : 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override {
        if (portType == QtNodes::PortType::In) {
            return CellData().type();
        } else {
            return BooleanData().type();
        }
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex port) override {
        if (port != 0) return nullptr;

        auto leftData = getInputData(0);
        
        if (!leftData) {
            qDebug() << "StringCompareDoubleModel: No left input data";
            return std::make_shared<BooleanData>(false);
        }

        QString leftString = extractStringValue(leftData);
        qDebug() << "StringCompareDoubleModel: Left string:" << leftString;
        
        // 对于IsEmpty类型，只需要检查左操作数
        if (m_compareType == IsEmpty) {
            bool result = leftString.isEmpty();
            qDebug() << "StringCompareDoubleModel: IsEmpty result:" << result;
            return std::make_shared<BooleanData>(result);
        }

        auto rightData = getInputData(1);
        if (!rightData) {
            qDebug() << "StringCompareDoubleModel: No right input data";
            return std::make_shared<BooleanData>(false);
        }

        QString rightString = extractStringValue(rightData);
        qDebug() << "StringCompareDoubleModel: Right string:" << rightString;
        
        bool result = performComparison(leftString, rightString);
        qDebug() << "StringCompareDoubleModel: Comparison result:" << result 
                 << "(" << leftString << getOperatorString() << rightString << ")";
        
        return std::make_shared<BooleanData>(result);
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex portIndex) override {
        qDebug() << "StringCompareDoubleModel: setInData called, portIndex:" << portIndex;
        
        if (portIndex < m_inputData.size()) {
            m_inputData[portIndex] = nodeData;
            
            if (nodeData) {
                if (auto cellData = std::dynamic_pointer_cast<CellData>(nodeData)) {
                    qDebug() << "StringCompareDoubleModel: Received CellData at port" << portIndex 
                             << "value:" << cellData->value().toString();
                } else {
                    qDebug() << "StringCompareDoubleModel: Received non-CellData at port" << portIndex;
                }
            } else {
                qDebug() << "StringCompareDoubleModel: Received null data at port" << portIndex;
            }
        }
        
        // 立即重新计算并通知输出更新
        emit dataUpdated(0);
    }

    QWidget* embeddedWidget() override {
        if (!m_widget) {
            m_widget = new QWidget();
            auto layout = new QVBoxLayout(m_widget);
            layout->setContentsMargins(4, 4, 4, 4);
            layout->setSpacing(2);

            // 比较类型选择
            auto typeLabel = new QLabel("比较类型:");
            typeLabel->setStyleSheet("font-weight: bold; font-size: 10px;");
            layout->addWidget(typeLabel);

            m_typeCombo = new QComboBox();
            m_typeCombo->addItems({"相等", "不相等", "包含", "开始于", "结束于", "为空"});
            m_typeCombo->setCurrentIndex(static_cast<int>(m_compareType));
            m_typeCombo->setStyleSheet("font-size: 10px;");

            connect(m_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    [this](int index) {
                        m_compareType = static_cast<CompareType>(index);
                        qDebug() << "StringCompareDoubleModel: Compare type changed to" << index;
                        emit dataUpdated(0);
                    });

            layout->addWidget(m_typeCombo);

            // 大小写敏感选项
            m_caseSensitiveCheck = new QCheckBox("大小写敏感");
            m_caseSensitiveCheck->setChecked(m_caseSensitive);
            m_caseSensitiveCheck->setStyleSheet("font-size: 10px;");

            connect(m_caseSensitiveCheck, &QCheckBox::toggled,
                    [this](bool checked) {
                        m_caseSensitive = checked;
                        qDebug() << "StringCompareDoubleModel: Case sensitive changed to" << checked;
                        emit dataUpdated(0);
                    });

            layout->addWidget(m_caseSensitiveCheck);
        }
        return m_widget;
    }

    QJsonObject save() const override {
        QJsonObject modelJson = BaseNodeModel::save();
        modelJson["compareType"] = static_cast<int>(m_compareType);
        modelJson["caseSensitive"] = m_caseSensitive;
        return modelJson;
    }

    void load(QJsonObject const& json) override {
        BaseNodeModel::load(json);
        if (json.contains("compareType")) {
            m_compareType = static_cast<CompareType>(json["compareType"].toInt());
            if (m_typeCombo) {
                m_typeCombo->setCurrentIndex(static_cast<int>(m_compareType));
            }
        }
        if (json.contains("caseSensitive")) {
            m_caseSensitive = json["caseSensitive"].toBool();
            if (m_caseSensitiveCheck) {
                m_caseSensitiveCheck->setChecked(m_caseSensitive);
            }
        }
    }

    // IPropertyProvider接口实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override {
        if (!propertyWidget) return false;
        
        propertyWidget->addTitle("字符串比较设置");
        propertyWidget->addDescription("比较两个字符串输入的关系");
        
        // 比较类型设置
        QStringList types = {"相等", "不相等", "包含", "开始于", "结束于", "为空"};
        propertyWidget->addComboProperty("比较类型", types,
            static_cast<int>(m_compareType), "compareType",
            [this](int index) {
                if (index >= 0 && index < 6) {
                    m_compareType = static_cast<CompareType>(index);
                    if (m_typeCombo) {
                        m_typeCombo->setCurrentIndex(index);
                    }
                    emit dataUpdated(0);
                }
            });
        
        // 大小写敏感设置
        propertyWidget->addCheckBoxProperty("大小写敏感", m_caseSensitive,
            "caseSensitive",
            [this](bool checked) {
                m_caseSensitive = checked;
                if (m_caseSensitiveCheck) {
                    m_caseSensitiveCheck->setChecked(checked);
                }
                emit dataUpdated(0);
            });
        
        return true;
    }
    
    QString getDisplayName() const override {
        return tr("字符串比较");
    }

protected:
    QString getNodeTypeName() const override {
        return "StringCompare";
    }

private:
    CompareType m_compareType;
    bool m_caseSensitive;
    QWidget* m_widget = nullptr;
    QComboBox* m_typeCombo = nullptr;
    QCheckBox* m_caseSensitiveCheck = nullptr;
    std::vector<std::shared_ptr<QtNodes::NodeData>> m_inputData;

    std::shared_ptr<QtNodes::NodeData> getInputData(QtNodes::PortIndex index) {
        if (index < m_inputData.size()) {
            return m_inputData[index];
        }
        return nullptr;
    }

    QString extractStringValue(std::shared_ptr<QtNodes::NodeData> data) {
        if (auto cellData = std::dynamic_pointer_cast<CellData>(data)) {
            return cellData->value().toString();
        }
        return QString();
    }

    QString getOperatorString() const {
        switch (m_compareType) {
            case Equal: return " == ";
            case NotEqual: return " != ";
            case Contains: return " contains ";
            case StartsWith: return " starts with ";
            case EndsWith: return " ends with ";
            case IsEmpty: return " is empty ";
            default: return " ? ";
        }
    }

    bool performComparison(const QString& left, const QString& right) {
        Qt::CaseSensitivity cs = m_caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        
        switch (m_compareType) {
            case Equal:
                return left.compare(right, cs) == 0;
            case NotEqual:
                return left.compare(right, cs) != 0;
            case Contains:
                return left.contains(right, cs);
            case StartsWith:
                return left.startsWith(right, cs);
            case EndsWith:
                return left.endsWith(right, cs);
            case IsEmpty:
                return left.isEmpty();
            default:
                return false;
        }
    }
};
