//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/CellData.hpp"
#include "data/BooleanData.hpp"

#include <QLineEdit>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>

/**
 * @brief 字符串比较节点模型
 *
 * 这个节点接收一个单元格数据(CellData)作为输入，
 * 允许用户设置比较操作和目标值，
 * 然后输出两个布尔结果：True端口和False端口。
 * 根据比较结果，只有一个端口会输出有效数据。
 */
class StringCompareModel : public BaseNodeModel
{
    Q_OBJECT

public:
    enum CompareOperation {
        Equals,         // 等于
        NotEquals,      // 不等于
        Contains,       // 包含
        NotContains,    // 不包含
        StartsWith,     // 开始于
        EndsWith,       // 结束于
        IsEmpty,        // 为空
        IsNotEmpty      // 不为空
    };

    StringCompareModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        auto* layout = new QVBoxLayout(m_widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);

        // 比较操作选择
        auto* opLayout = new QHBoxLayout();
        auto* opLabel = new QLabel("操作:");
        opLayout->addWidget(opLabel);

        m_operationCombo = new QComboBox();
        m_operationCombo->addItem("等于", static_cast<int>(Equals));
        m_operationCombo->addItem("不等于", static_cast<int>(NotEquals));
        m_operationCombo->addItem("包含", static_cast<int>(Contains));
        m_operationCombo->addItem("不包含", static_cast<int>(NotContains));
        m_operationCombo->addItem("开始于", static_cast<int>(StartsWith));
        m_operationCombo->addItem("结束于", static_cast<int>(EndsWith));
        m_operationCombo->addItem("为空", static_cast<int>(IsEmpty));
        m_operationCombo->addItem("不为空", static_cast<int>(IsNotEmpty));
        opLayout->addWidget(m_operationCombo);
        layout->addLayout(opLayout);

        // 比较值输入
        auto* valueLayout = new QHBoxLayout();
        auto* valueLabel = new QLabel("值:");
        valueLayout->addWidget(valueLabel);

        m_valueEdit = new QLineEdit();
        m_valueEdit->setPlaceholderText("比较值");
        valueLayout->addWidget(m_valueEdit);
        layout->addLayout(valueLayout);

        // 连接信号
        connect(m_operationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &StringCompareModel::onParameterChanged);
        connect(m_valueEdit, &QLineEdit::textChanged,
                this, &StringCompareModel::onParameterChanged);

        // 注册需要保存的属性
        registerComboBox("operation", m_operationCombo, "比较操作");
        registerLineEdit("value", m_valueEdit, "比较值");
    }

    QString caption() const override
    {
        return tr("字符串比较");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("StringCompare");
    }

    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        if (portType == QtNodes::PortType::In) return 1;  // 输入：CellData
        if (portType == QtNodes::PortType::Out) return 1; // 输出：BooleanData（true/false）
        return 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In)
        {
            return CellData().type();
        }
        else if (portType == QtNodes::PortType::Out)
        {
            // 两个端口都返回相同的BooleanData类型，这样可以连接到DisplayBooleanModel
            return BooleanData().type();
        }
        return {"", ""};
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        if (port == 0) {
            return m_result; // 唯一输出：比较结果（true/false）
        }
        return nullptr;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "StringCompareModel::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << "StringCompareModel: Received null nodeData";
            m_cellData.reset();
            updateComparison();
            return;
        }
        
        m_cellData = std::dynamic_pointer_cast<CellData>(nodeData);
        if (m_cellData) {
            qDebug() << "StringCompareModel: Successfully received CellData";
        } else {
            qDebug() << "StringCompareModel: Failed to cast to CellData";
        }
        
        updateComparison();
    }

protected:
    // 实现基类的虚函数
    QString getNodeTypeName() const override
    {
        return "StringCompareModel";
    }

    // 自定义加载逻辑，处理ComboBox的特殊情况
    void onLoad(const QJsonObject& json) override
    {
        if (json.contains("operation")) {
            int op = json["operation"].toInt();
            for (int i = 0; i < m_operationCombo->count(); ++i) {
                if (m_operationCombo->itemData(i).toInt() == op) {
                    m_operationCombo->setCurrentIndex(i);
                    break;
                }
            }
        }
    }

    // 实现IPropertyProvider接口
    bool createPropertyWidget(QVBoxLayout* parent) override
    {
        addTitle(parent, "字符串比较设置");
        addDescription(parent, "设置比较操作和目标值，输出True/False结果");

        // 比较操作
        addLabeledWidget(parent, "比较操作:", new QLabel(
            m_operationCombo->currentText()
        ));

        // 比较值
        addLabeledWidget(parent, "比较值:", new QLabel(
            m_valueEdit->text().isEmpty() ? "未设置" : m_valueEdit->text()
        ));

        // 当前输入数据
        if (m_cellData && m_cellData->isValid()) {
            addSeparator(parent);
            addTitle(parent, "输入数据");

            try {
                QString address = m_cellData->address();
                QVariant value = m_cellData->value();
                auto* inputInfo = new QLabel(QString("单元格: %1\n值: %2")
                    .arg(address)
                    .arg(value.toString()));
                inputInfo->setStyleSheet("color: #666;");
                inputInfo->setWordWrap(true);
                parent->addWidget(inputInfo);
            } catch (...) {
                auto* errorInfo = new QLabel("无法读取输入数据");
                errorInfo->setStyleSheet("color: #999;");
                parent->addWidget(errorInfo);
            }
        }

        // 比较结果
        if (m_result) {
            addSeparator(parent);
            addTitle(parent, "比较结果");

            auto* resultInfo = new QLabel(QString("结果: %1")
                .arg(m_result->value() ? "True" : "False"));
            resultInfo->setStyleSheet(QString("color: %1; font-weight: bold;")
                .arg(m_result->value() ? "#28a745" : "#dc3545"));
            parent->addWidget(resultInfo);
        }

        return true;
    }

    QString getDisplayName() const override
    {
        return "字符串比较";
    }

    QString getDescription() const override
    {
        return "比较单元格值与指定字符串，输出布尔结果";
    }



private slots:
    void onParameterChanged()
    {
        qDebug() << "StringCompareModel: Parameters changed";
        updateComparison();
    }

private:
    void updateComparison()
    {
        qDebug() << "StringCompareModel::updateComparison called";
        
        // 重置结果
        m_result.reset();

        if (!m_cellData || !m_cellData->isValid()) {
            qDebug() << "StringCompareModel: No valid cell data";
            emit dataUpdated(0);
            return;
        }

        try {
            // 获取单元格值 - 使用新的CellData API
            QString cellValue;

            if (m_cellData->cell()) {
                // 真实的OpenXLSX单元格
                auto cell = m_cellData->cell();
                if (cell->value().type() == OpenXLSX::XLValueType::String) {
                    cellValue = QString::fromUtf8(cell->value().get<std::string>().c_str());
                } else if (cell->value().type() == OpenXLSX::XLValueType::Integer) {
                    cellValue = QString::number(cell->value().get<int64_t>());
                } else if (cell->value().type() == OpenXLSX::XLValueType::Float) {
                    cellValue = QString::number(cell->value().get<double>());
                } else if (cell->value().type() == OpenXLSX::XLValueType::Boolean) {
                    cellValue = cell->value().get<bool>() ? "TRUE" : "FALSE";
                } else {
                    cellValue = "";
                }
            } else {
                // 虚拟单元格，直接使用value()方法
                QVariant value = m_cellData->value();
                cellValue = value.toString();
            }
            
            // 获取比较参数
            CompareOperation operation = static_cast<CompareOperation>(m_operationCombo->currentData().toInt());
            QString compareValue = m_valueEdit->text();
            
            // 执行比较
            bool result = performComparison(cellValue, compareValue, operation);
            
            QString description = QString("'%1' %2 '%3'")
                .arg(cellValue)
                .arg(m_operationCombo->currentText())
                .arg(compareValue);
            
            qDebug() << "StringCompareModel: Comparison result:" << result << "for" << description;

            // 两个端口都始终有输出，但内容不同
            // 第一个端口(0)：True端口 - 只有当结果为true时才输出数据
            // 第二个端口(1)：False端口 - 只有当结果为false时才输出数据
            // 创建统一的比较结果
            m_result = std::make_shared<BooleanData>(result, description);

            emit dataUpdated(0); // 输出比较结果
            
        } catch (const std::exception& e) {
            qDebug() << "StringCompareModel: Error in comparison:" << e.what();
            emit dataUpdated(0);
        }
    }

    bool performComparison(const QString& cellValue, const QString& compareValue, CompareOperation operation)
    {
        switch (operation) {
            case Equals:
                return cellValue == compareValue;
            case NotEquals:
                return cellValue != compareValue;
            case Contains:
                return cellValue.contains(compareValue, Qt::CaseInsensitive);
            case NotContains:
                return !cellValue.contains(compareValue, Qt::CaseInsensitive);
            case StartsWith:
                return cellValue.startsWith(compareValue, Qt::CaseInsensitive);
            case EndsWith:
                return cellValue.endsWith(compareValue, Qt::CaseInsensitive);
            case IsEmpty:
                return cellValue.isEmpty();
            case IsNotEmpty:
                return !cellValue.isEmpty();
            default:
                return false;
        }
    }

    QWidget* m_widget;
    QComboBox* m_operationCombo;
    QLineEdit* m_valueEdit;
    
    std::shared_ptr<CellData> m_cellData;
    std::shared_ptr<BooleanData> m_result;
};
