//
// Created by wuxianggujun on 25-6-29.
//

#include "SmartLoopProcessorModel.hpp"
#include <QtCore/qglobal.h>

SmartLoopProcessorModel::SmartLoopProcessorModel()
{
    m_primaryOutput = std::make_shared<CellListData>();
    m_resultCount = std::make_shared<IntegerData>(0);
    m_processStatus = std::make_shared<BooleanData>(false);

    qDebug() << "SmartLoopProcessorModel: Created";
}

QJsonObject SmartLoopProcessorModel::save() const
{
    QJsonObject obj;
    obj["model-name"] = name();
    obj["processMode"] = static_cast<int>(m_processMode);
    obj["columnIndex"] = m_columnIndex;
    obj["conditionType"] = static_cast<int>(m_conditionType);
    obj["conditionValue"] = m_conditionValue;

    return obj;
}

void SmartLoopProcessorModel::load(const QJsonObject& obj)
{
    m_processMode = static_cast<ProcessMode>(obj["processMode"].toInt());
    m_columnIndex = obj["columnIndex"].toInt();
    m_conditionType = static_cast<ConditionType>(obj["conditionType"].toInt());
    m_conditionValue = obj["conditionValue"].toString();

    
    // 更新UI
    if (m_processModeCombo) {
        m_processModeCombo->setCurrentIndex(static_cast<int>(m_processMode));
    }
    if (m_columnSpinBox) {
        m_columnSpinBox->setValue(m_columnIndex);
    }
    if (m_conditionCombo) {
        m_conditionCombo->setCurrentIndex(static_cast<int>(m_conditionType));
    }
    if (m_conditionValueEdit) {
        m_conditionValueEdit->setText(m_conditionValue);
    }

}

unsigned int SmartLoopProcessorModel::nPorts(QtNodes::PortType const portType) const
{
    if (portType == QtNodes::PortType::In) {
        return 1; // RangeData输入
    } else {
        return 3; // RangeData, IntegerData, BooleanData输出
    }
}

QtNodes::NodeDataType SmartLoopProcessorModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        return RangeData().type();
    } else {
        if (portIndex == 0) {
            return CellListData().type();  // 单元格列表
        } else if (portIndex == 1) {
            return IntegerData().type();   // 结果数量
        } else {
            return BooleanData().type();   // 处理状态
        }
    }
}

std::shared_ptr<QtNodes::NodeData> SmartLoopProcessorModel::outData(QtNodes::PortIndex const port)
{
    if (port == 0) {
        return m_primaryOutput;
    } else if (port == 1) {
        return m_resultCount;
    } else if (port == 2) {
        return m_processStatus;
    }
    return nullptr;
}

void SmartLoopProcessorModel::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex)
{
    qDebug() << "SmartLoopProcessorModel::setInData called, portIndex:" << portIndex;
    
    if (portIndex == 0) {
        if (auto rangeData = std::dynamic_pointer_cast<RangeData>(nodeData)) {
            m_inputRangeData = rangeData;
            
            // 更新列选择器的最大值
            if (m_columnSpinBox && m_inputRangeData) {
                m_columnSpinBox->setMaximum(qMax(0, m_inputRangeData->columnCount() - 1));
            }
            
            // 自动处理（如果启用）
            if (m_autoProcessCheck && m_autoProcessCheck->isChecked()) {
                processData();
            }
            
            updatePreview();
            qDebug() << "SmartLoopProcessorModel: Received range data with" 
                     << rangeData->rowCount() << "rows and" << rangeData->columnCount() << "columns";
        } else {
            m_inputRangeData.reset();
            clearResults();
            qDebug() << "SmartLoopProcessorModel: Received null range data";
        }
    }
}

QString SmartLoopProcessorModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        return "数据源";
    } else {
        if (portIndex == 0) {
            return QString("匹配单元格(第%1列)").arg(m_columnIndex + 1);
        } else if (portIndex == 1) {
            return "匹配数量";
        } else {
            return "处理状态";
        }
    }
}

void SmartLoopProcessorModel::onProcessModeChanged()
{
    if (m_processModeCombo) {
        m_processMode = static_cast<ProcessMode>(m_processModeCombo->currentIndex());
        updatePreview();
        qDebug() << "SmartLoopProcessorModel: Process mode changed to" << m_processMode;
    }
}

void SmartLoopProcessorModel::onColumnIndexChanged()
{
    if (m_columnSpinBox) {
        m_columnIndex = m_columnSpinBox->value();
        updatePreview();
        qDebug() << "SmartLoopProcessorModel: Column index changed to" << m_columnIndex;
    }
}

void SmartLoopProcessorModel::onConditionChanged()
{
    if (m_conditionCombo) {
        m_conditionType = static_cast<ConditionType>(m_conditionCombo->currentIndex());
        updatePreview();
        qDebug() << "SmartLoopProcessorModel: Condition type changed to" << m_conditionType;
    }
}

void SmartLoopProcessorModel::onConditionValueChanged()
{
    if (m_conditionValueEdit) {
        m_conditionValue = m_conditionValueEdit->text();
        updatePreview();
        qDebug() << "SmartLoopProcessorModel: Condition value changed to" << m_conditionValue;
    }
}



void SmartLoopProcessorModel::onProcessClicked()
{
    processData();
    qDebug() << "SmartLoopProcessorModel: Manual process triggered";
}

void SmartLoopProcessorModel::processData()
{
    if (!m_inputRangeData) {
        clearResults();
        return;
    }
    
    m_matchedRows.clear();
    m_matchedIndices.clear();
    
    // 按行处理
    if (m_processMode == ByRow) {
        for (int row = 0; row < m_inputRangeData->rowCount(); ++row) {
            auto rowData = m_inputRangeData->rowData(row);
            
            if (m_columnIndex < rowData.size()) {
                QString cellValue = rowData[m_columnIndex].toString();
                bool matches = false;
                
                // 条件判断
                switch (m_conditionType) {
                    case Equal:
                        matches = (cellValue == m_conditionValue);
                        break;
                    case NotEqual:
                        matches = (cellValue != m_conditionValue);
                        break;
                    case Contains:
                        matches = cellValue.contains(m_conditionValue, Qt::CaseInsensitive);
                        break;
                    case NotContains:
                        matches = !cellValue.contains(m_conditionValue, Qt::CaseInsensitive);
                        break;
                    case Greater:
                        matches = (cellValue.toDouble() > m_conditionValue.toDouble());
                        break;
                    case Less:
                        matches = (cellValue.toDouble() < m_conditionValue.toDouble());
                        break;
                }
                
                if (matches) {
                    // 转换std::vector<QVariant>到QList<QVariant>
                    QList<QVariant> qlistRow;
                    for (const auto& cell : rowData) {
                        qlistRow.append(cell);
                    }
                    m_matchedRows.append(qlistRow);
                    m_matchedIndices.append(row);
                }
            }
        }
    }
    
    updateOutputs();
    updatePreview();
    
    qDebug() << "SmartLoopProcessorModel: Processed data, found" << m_matchedRows.size() << "matches";
}

void SmartLoopProcessorModel::updatePreview()
{
    if (!m_previewText) return;
    
    QString preview;
    
    if (!m_inputRangeData) {
        preview = "等待数据输入...";
    } else if (m_matchedRows.isEmpty()) {
        preview = QString("数据源: %1行 x %2列\n条件: 第%3列 %4 '%5'\n结果: 无匹配数据")
                    .arg(m_inputRangeData->rowCount())
                    .arg(m_inputRangeData->columnCount())
                    .arg(m_columnIndex + 1)
                    .arg(m_conditionCombo ? m_conditionCombo->currentText() : "")
                    .arg(m_conditionValue);
    } else {
        preview = QString("数据源: %1行 x %2列\n条件: 第%3列 %4 '%5'\n\n找到 %6 行符合条件:\n")
                    .arg(m_inputRangeData->rowCount())
                    .arg(m_inputRangeData->columnCount())
                    .arg(m_columnIndex + 1)
                    .arg(m_conditionCombo ? m_conditionCombo->currentText() : "")
                    .arg(m_conditionValue)
                    .arg(m_matchedRows.size());
        
        // 显示前5行结果
        for (int i = 0; i < qMin(5, m_matchedRows.size()); ++i) {
            preview += QString("行%1: [").arg(m_matchedIndices[i] + 1);
            for (int j = 0; j < qMin(3, m_matchedRows[i].size()); ++j) {
                if (j > 0) preview += ", ";
                preview += m_matchedRows[i][j].toString();
            }
            if (m_matchedRows[i].size() > 3) {
                preview += ", ...";
            }
            preview += "]\n";
        }
        
        if (m_matchedRows.size() > 5) {
            preview += QString("... 还有 %1 行").arg(m_matchedRows.size() - 5);
        }
    }
    
    m_previewText->setPlainText(preview);
}

void SmartLoopProcessorModel::updateOutputs()
{
    // 更新结果数量
    m_resultCount->setValue(m_matchedRows.size());

    // 更新处理状态
    m_processStatus->setValue(true);

    // 创建CellListData输出
    auto cellList = std::make_shared<CellListData>();

    if (!m_matchedRows.isEmpty()) {
        for (int i = 0; i < m_matchedRows.size(); ++i) {
            const auto& row = m_matchedRows[i];
            int rowIndex = (i < m_matchedIndices.size()) ? m_matchedIndices[i] : i;

            // 从指定列创建CellData
            if (m_columnIndex < row.size()) {
                QString cellAddress = QString("%1%2")
                    .arg(QChar('A' + m_columnIndex))
                    .arg(rowIndex + 1);

                CellData cellData(cellAddress, row[m_columnIndex]);
                cellList->addCell(cellData, rowIndex);
            }
        }
    }

    m_primaryOutput = cellList;

    // 触发数据更新
    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);

    // 更新状态显示
    if (m_statusLabel) {
        m_statusLabel->setText(QString("已处理，找到 %1 个匹配的单元格(第%2列)")
                              .arg(m_matchedRows.size()).arg(m_columnIndex + 1));
    }

    qDebug() << "SmartLoopProcessorModel: Created CellListData with" << cellList->count() << "cells";
}

void SmartLoopProcessorModel::clearResults()
{
    m_matchedRows.clear();
    m_matchedIndices.clear();
    m_resultCount->setValue(0);
    m_processStatus->setValue(false);

    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);

    if (m_statusLabel) {
        m_statusLabel->setText("等待处理");
    }

    updatePreview();
}

QWidget* SmartLoopProcessorModel::embeddedWidget()
{
    if (!m_widget) {
        m_widget = new QWidget();
        auto mainLayout = new QVBoxLayout(m_widget);

        // 状态显示
        m_statusLabel = new QLabel("等待数据输入");
        m_statusLabel->setStyleSheet("QLabel { font-weight: bold; color: #333; }");
        mainLayout->addWidget(m_statusLabel);

        // 处理模式
        auto modeGroup = new QGroupBox("处理模式");
        auto modeLayout = new QHBoxLayout(modeGroup);
        modeLayout->addWidget(new QLabel("模式:"));
        m_processModeCombo = new QComboBox();
        m_processModeCombo->addItems({"按行处理", "按列处理"});
        modeLayout->addWidget(m_processModeCombo);
        mainLayout->addWidget(modeGroup);

        // 条件设置
        auto conditionGroup = new QGroupBox("条件设置");
        auto conditionLayout = new QVBoxLayout(conditionGroup);

        // 列选择
        auto columnLayout = new QHBoxLayout();
        columnLayout->addWidget(new QLabel("目标列:"));
        m_columnSpinBox = new QSpinBox();
        m_columnSpinBox->setMinimum(0);
        m_columnSpinBox->setMaximum(99);
        m_columnSpinBox->setSuffix(" (第1列)");
        columnLayout->addWidget(m_columnSpinBox);
        columnLayout->addStretch();
        conditionLayout->addLayout(columnLayout);

        // 条件类型和值
        auto condValueLayout = new QHBoxLayout();
        condValueLayout->addWidget(new QLabel("条件:"));
        m_conditionCombo = new QComboBox();
        m_conditionCombo->addItems({"等于", "不等于", "包含", "不包含", "大于", "小于"});
        condValueLayout->addWidget(m_conditionCombo);

        m_conditionValueEdit = new QLineEdit();
        m_conditionValueEdit->setPlaceholderText("输入条件值...");
        condValueLayout->addWidget(m_conditionValueEdit);
        conditionLayout->addLayout(condValueLayout);

        mainLayout->addWidget(conditionGroup);



        // 控制按钮
        auto controlLayout = new QHBoxLayout();
        m_autoProcessCheck = new QCheckBox("自动处理");
        m_autoProcessCheck->setChecked(true);
        controlLayout->addWidget(m_autoProcessCheck);

        m_processButton = new QPushButton("立即处理");
        controlLayout->addWidget(m_processButton);
        controlLayout->addStretch();
        mainLayout->addLayout(controlLayout);

        // 预览区域
        auto previewGroup = new QGroupBox("实时预览");
        auto previewLayout = new QVBoxLayout(previewGroup);
        m_previewText = new QTextEdit();
        m_previewText->setMaximumHeight(120);
        m_previewText->setReadOnly(true);
        m_previewText->setStyleSheet("QTextEdit { font-family: 'Consolas', monospace; font-size: 9pt; }");
        previewLayout->addWidget(m_previewText);
        mainLayout->addWidget(previewGroup);

        // 连接信号
        connect(m_processModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SmartLoopProcessorModel::onProcessModeChanged);
        connect(m_columnSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &SmartLoopProcessorModel::onColumnIndexChanged);
        connect(m_conditionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SmartLoopProcessorModel::onConditionChanged);
        connect(m_conditionValueEdit, &QLineEdit::textChanged,
                this, &SmartLoopProcessorModel::onConditionValueChanged);

        connect(m_processButton, &QPushButton::clicked,
                this, &SmartLoopProcessorModel::onProcessClicked);

        updatePreview();
    }

    return m_widget;
}
