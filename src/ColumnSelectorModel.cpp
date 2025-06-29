//
// Created by wuxianggujun on 25-6-29.
//

#include "ColumnSelectorModel.hpp"
#include <QtCore/qglobal.h>

ColumnSelectorModel::ColumnSelectorModel()
{
    m_cellData = std::make_shared<CellData>();
    qDebug() << "ColumnSelectorModel: Created";
}

QJsonObject ColumnSelectorModel::save() const
{
    QJsonObject obj;
    obj["model-name"] = name();  // 保存节点类型名称
    obj["columnIndex"] = m_columnIndex;
    return obj;
}

void ColumnSelectorModel::load(const QJsonObject& obj)
{
    m_columnIndex = obj["columnIndex"].toInt();
    if (m_columnSpinBox)
    {
        m_columnSpinBox->setValue(m_columnIndex);
    }
    updateCellData();
}

unsigned int ColumnSelectorModel::nPorts(QtNodes::PortType const portType) const
{
    if (portType == QtNodes::PortType::In)
    {
        return 1; // RowData输入
    }
    else
    {
        return 1; // CellData输出
    }
}

QtNodes::NodeDataType ColumnSelectorModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In)
    {
        return RowData().type(); // 行数据输入
    }
    else
    {
        return CellData().type(); // 单元格数据输出
    }
}

std::shared_ptr<QtNodes::NodeData> ColumnSelectorModel::outData(QtNodes::PortIndex const port)
{
    if (port == 0)
    {
        return m_cellData;
    }
    return nullptr;
}

void ColumnSelectorModel::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex)
{
    qDebug() << "ColumnSelectorModel::setInData called, portIndex:" << portIndex;

    if (portIndex == 0)
    {
        if (auto rowData = std::dynamic_pointer_cast<RowData>(nodeData))
        {
            m_rowData = rowData;
            updateCellData();
            qDebug() << "ColumnSelectorModel: Received row data with" << rowData->columnCount() << "cells";
        }
        else
        {
            m_rowData.reset();
            m_cellData = std::make_shared<CellData>();
            Q_EMIT dataUpdated(0);
            qDebug() << "ColumnSelectorModel: Received null row data";
        }
        updateDisplay();
    }
}

QWidget* ColumnSelectorModel::embeddedWidget()
{
    if (!m_widget)
    {
        m_widget = new QWidget();
        auto layout = new QVBoxLayout(m_widget);

        // 信息显示
        m_infoLabel = new QLabel("选择列索引:");
        layout->addWidget(m_infoLabel);

        // 列索引选择
        auto columnLayout = new QHBoxLayout();
        columnLayout->addWidget(new QLabel("列索引:"));

        m_columnSpinBox = new QSpinBox();
        m_columnSpinBox->setMinimum(0);
        m_columnSpinBox->setMaximum(99);
        m_columnSpinBox->setValue(m_columnIndex);
        columnLayout->addWidget(m_columnSpinBox);

        layout->addLayout(columnLayout);

        // 预览显示
        m_previewLabel = new QLabel("预览: (无数据)");
        m_previewLabel->setWordWrap(true);
        m_previewLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc; }");
        layout->addWidget(m_previewLabel);

        // 连接信号
        connect(m_columnSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                this, &ColumnSelectorModel::onColumnIndexChanged);

        updateDisplay();
    }

    return m_widget;
}

QString ColumnSelectorModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In)
    {
        return "行数据";
    }
    else
    {
        return QString("列%1数据").arg(m_columnIndex);
    }
}

void ColumnSelectorModel::onColumnIndexChanged(int index)
{
    m_columnIndex = index;
    updateCellData();
    updateDisplay();
    qDebug() << "ColumnSelectorModel: Column index changed to" << index;
}

void ColumnSelectorModel::updateCellData()
{
    if (m_rowData && m_columnIndex < m_rowData->columnCount())
    {
        // 从行数据中提取指定列的单元格
        QVariant cellValue = m_rowData->cellValue(m_columnIndex);
        QString cellAddress = QString("Col%1Row%2").arg(m_columnIndex).arg(m_rowData->rowIndex());
        m_cellData = std::make_shared<CellData>(cellAddress, cellValue);
        Q_EMIT dataUpdated(0);
        qDebug() << "ColumnSelectorModel: Updated cell data for column" << m_columnIndex;
    }
    else
    {
        m_cellData = std::make_shared<CellData>();
        Q_EMIT dataUpdated(0);
        qDebug() << "ColumnSelectorModel: No valid data for column" << m_columnIndex;
    }
}

void ColumnSelectorModel::updateDisplay()
{
    if (m_previewLabel)
    {
        if (m_rowData && m_columnIndex < m_rowData->columnCount())
        {
            QVariant cellValue = m_rowData->cellValue(m_columnIndex);
            QString cellText = cellValue.toString();
            m_previewLabel->setText(QString("预览: %1").arg(cellText.isEmpty() ? "(空)" : cellText));
        }
        else
        {
            m_previewLabel->setText("预览: (列索引超出范围)");
        }
    }
    else
    {
        m_previewLabel->setText("预览: (无数据)");
    }

    if (m_columnSpinBox && m_rowData)
    {
        m_columnSpinBox->setMaximum(qMax(0, m_rowData->columnCount() - 1));
    }
}
