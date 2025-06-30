//
// Created by wuxianggujun on 25-6-29.
//

#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QObject>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QTextEdit>
#include <QPushButton>
#include <QGroupBox>
#include <QSpinBox>
#include <QDebug>

#include "BaseNodeModel.hpp"
#include "PropertyWidget.hpp"
#include "data/RangeData.hpp"
#include "data/RowData.hpp"
#include "data/CellData.hpp"
#include "data/CellListData.hpp"
#include "data/BooleanData.hpp"
#include "data/IntegerData.hpp"

/**
 * @brief 智能循环处理器 - 一体化的循环数据处理节点
 *
 * 功能：
 * - 遍历范围数据（按行或按列）
 * - 内置列选择和条件判断
 * - 收集符合条件的数据
 * - 实时预览处理结果
 * - 多种输出模式
 *
 * 输入端口：
 * - 0: RangeData - 要处理的数据范围
 *
 * 输出端口：
 * - 0: CellListData - 符合条件的单元格列表
 * - 1: IntegerData - 符合条件的行数量
 * - 2: BooleanData - 处理完成状态
 */
class SmartLoopProcessorModel : public BaseNodeModel
{
    Q_OBJECT

public:
    SmartLoopProcessorModel();
    ~SmartLoopProcessorModel() override = default;

public:
    QString caption() const override { return "智能循环处理器"; }
    QString name() const override { return "SmartLoopProcessor"; }

public:
    QJsonObject save() const override;
    void load(QJsonObject const &) override;

public:
    unsigned int nPorts(QtNodes::PortType const portType) const override;
    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;
    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override;
    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override;
    QWidget* embeddedWidget() override;

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override;

private slots:
    void onProcessModeChanged();
    void onColumnIndexChanged();
    void onConditionChanged();
    void onConditionValueChanged();
    void onProcessClicked();


private:
    void processData();
    void updatePreview();
    void updateOutputs();
    void clearResults();

private:
    // 输入数据
    std::shared_ptr<RangeData> m_inputRangeData;
    
    // 输出数据
    std::shared_ptr<QtNodes::NodeData> m_primaryOutput;  // 主要输出，类型根据模式变化
    std::shared_ptr<IntegerData> m_resultCount;
    std::shared_ptr<BooleanData> m_processStatus;
    
    // 处理配置
    enum ProcessMode { ByRow = 0, ByColumn = 1 };
    enum ConditionType { Equal = 0, NotEqual = 1, Contains = 2, NotContains = 3, Greater = 4, Less = 5 };
    
    ProcessMode m_processMode = ByRow;
    int m_columnIndex = 0;
    ConditionType m_conditionType = Equal;
    QString m_conditionValue;
    
    // 处理结果
    QList<QList<QVariant>> m_matchedRows;
    QList<int> m_matchedIndices;
    
    // UI组件
    QWidget* m_widget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QComboBox* m_processModeCombo = nullptr;
    QSpinBox* m_columnSpinBox = nullptr;
    QComboBox* m_conditionCombo = nullptr;
    QLineEdit* m_conditionValueEdit = nullptr;

    QTextEdit* m_previewText = nullptr;
    QPushButton* m_processButton = nullptr;
    QCheckBox* m_autoProcessCheck = nullptr;

protected:
    // 实现BaseNodeModel的虚函数
    QString getNodeTypeName() const override
    {
        return "SmartLoopProcessorModel";
    }

    // 实现IPropertyProvider接口
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle("智能循环处理器设置");
        propertyWidget->addDescription("配置循环处理条件，自动筛选符合条件的数据");

        // 添加模式切换按钮
        propertyWidget->addModeToggleButtons();

        // 处理模式设置
        QStringList processModes = {"按行处理", "按列处理"};
        propertyWidget->addComboProperty("处理模式", processModes,
            static_cast<int>(m_processMode), "processMode",
            [this](int index) {
                if (index >= 0 && index < 2) {
                    m_processMode = static_cast<ProcessMode>(index);
                    if (m_processModeCombo) {
                        m_processModeCombo->setCurrentIndex(index);
                    }
                    qDebug() << "SmartLoopProcessorModel: Process mode changed to" << index;
                }
            });

        // 列索引设置
        propertyWidget->addTextProperty("目标列索引", QString::number(m_columnIndex),
            "columnIndex", "输入列索引（从0开始）",
            [this](const QString& value) {
                bool ok;
                int index = value.toInt(&ok);
                if (ok && index >= 0) {
                    m_columnIndex = index;
                    if (m_columnSpinBox) {
                        m_columnSpinBox->setValue(index);
                    }
                    qDebug() << "SmartLoopProcessorModel: Column index changed to" << index;
                }
            });

        // 条件类型设置
        QStringList conditionTypes = {"等于", "不等于", "包含", "不包含", "大于", "小于"};
        propertyWidget->addComboProperty("条件类型", conditionTypes,
            static_cast<int>(m_conditionType), "conditionType",
            [this](int index) {
                if (index >= 0 && index < 6) {
                    m_conditionType = static_cast<ConditionType>(index);
                    if (m_conditionCombo) {
                        m_conditionCombo->setCurrentIndex(index);
                    }
                    qDebug() << "SmartLoopProcessorModel: Condition type changed to" << index;
                }
            });

        // 条件值设置
        propertyWidget->addTextProperty("条件值", m_conditionValue,
            "conditionValue", "输入比较的目标值",
            [this](const QString& value) {
                m_conditionValue = value;
                if (m_conditionValueEdit) {
                    m_conditionValueEdit->setText(value);
                }
                qDebug() << "SmartLoopProcessorModel: Condition value changed to" << value;
            });

        // 数据连接状态
        propertyWidget->addSeparator();
        propertyWidget->addTitle("连接状态");

        if (m_inputRangeData) {
            propertyWidget->addInfoProperty("输入数据", "已连接", "color: #28a745; font-weight: bold;");
            propertyWidget->addInfoProperty("数据大小",
                QString("%1行 x %2列").arg(m_inputRangeData->rowCount()).arg(m_inputRangeData->columnCount()),
                "color: #666;");
        } else {
            propertyWidget->addInfoProperty("输入数据", "未连接", "color: #999; font-style: italic;");
        }

        // 处理结果状态
        if (!m_matchedRows.isEmpty()) {
            propertyWidget->addSeparator();
            propertyWidget->addTitle("处理结果");

            propertyWidget->addInfoProperty("匹配行数", QString::number(m_matchedRows.size()), "color: #2E86AB; font-weight: bold;");
            propertyWidget->addInfoProperty("处理状态", "已完成", "color: #28a745; font-weight: bold;");

            // 显示前几个匹配结果
            if (m_matchedRows.size() > 0) {
                propertyWidget->addSeparator();
                propertyWidget->addTitle("匹配预览");

                int previewCount = qMin(5, m_matchedRows.size());
                for (int i = 0; i < previewCount; ++i) {
                    if (m_columnIndex < m_matchedRows[i].size()) {
                        QString value = m_matchedRows[i][m_columnIndex].toString();
                        if (value.length() > 20) {
                            value = value.left(20) + "...";
                        }
                        propertyWidget->addInfoProperty(QString("第%1行").arg(i + 1), value, "color: #666; font-family: monospace;");
                    }
                }

                if (m_matchedRows.size() > previewCount) {
                    propertyWidget->addInfoProperty("", QString("... 还有%1行").arg(m_matchedRows.size() - previewCount), "color: #999; font-style: italic;");
                }
            }
        } else {
            propertyWidget->addSeparator();
            propertyWidget->addInfoProperty("处理结果", "无匹配数据", "color: #999; font-style: italic;");
        }

        return true;
    }

    QString getDisplayName() const override
    {
        return "智能循环处理器";
    }

    QString getDescription() const override
    {
        return "智能循环处理数据范围，根据条件筛选符合要求的行或列";
    }
};
