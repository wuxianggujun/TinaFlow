//
// Created by wuxianggujun on 25-6-29.
//

#include "DisplayCellListModel.hpp"

DisplayCellListModel::DisplayCellListModel()
{
    m_selectedCellData = std::make_shared<CellData>();
    qDebug() << "DisplayCellListModel: Created";
}

QJsonObject DisplayCellListModel::save() const
{
    QJsonObject obj;
    obj["model-name"] = name();
    obj["selectedIndex"] = m_selectedIndex;
    obj["displayMode"] = static_cast<int>(m_displayMode);
    return obj;
}

void DisplayCellListModel::load(const QJsonObject& obj)
{
    m_selectedIndex = obj["selectedIndex"].toInt();
    m_displayMode = static_cast<DisplayMode>(obj["displayMode"].toInt());
    
    if (m_cellListWidget) {
        m_cellListWidget->setCurrentRow(m_selectedIndex);
    }
    if (m_displayModeCombo) {
        m_displayModeCombo->setCurrentIndex(static_cast<int>(m_displayMode));
    }
}

unsigned int DisplayCellListModel::nPorts(QtNodes::PortType const portType) const
{
    if (portType == QtNodes::PortType::In) {
        return 1; // CellListData输入
    } else {
        return 1; // CellData输出
    }
}

QtNodes::NodeDataType DisplayCellListModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        return CellListData().type();
    } else {
        return CellData().type();
    }
}

std::shared_ptr<QtNodes::NodeData> DisplayCellListModel::outData(QtNodes::PortIndex const port)
{
    if (port == 0) {
        return m_selectedCellData;
    }
    return nullptr;
}

void DisplayCellListModel::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex)
{
    qDebug() << "DisplayCellListModel::setInData called, portIndex:" << portIndex;
    
    if (portIndex == 0) {
        if (auto cellListData = std::dynamic_pointer_cast<CellListData>(nodeData)) {
            m_cellListData = cellListData;
            updateDisplay();
            qDebug() << "DisplayCellListModel: Received cell list with" << cellListData->count() << "cells";
        } else {
            m_cellListData.reset();
            updateDisplay();
            qDebug() << "DisplayCellListModel: Received null cell list data";
        }
    }
}

QString DisplayCellListModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        return "单元格列表";
    } else {
        return "选中单元格";
    }
}

QWidget* DisplayCellListModel::embeddedWidget()
{
    if (!m_widget) {
        m_widget = new QWidget();
        auto layout = new QVBoxLayout(m_widget);
        
        // 状态显示
        m_statusLabel = new QLabel("等待单元格列表数据");
        m_statusLabel->setStyleSheet("QLabel { font-weight: bold; color: #333; }");
        layout->addWidget(m_statusLabel);
        
        // 显示模式选择
        auto modeLayout = new QHBoxLayout();
        modeLayout->addWidget(new QLabel("显示模式:"));
        m_displayModeCombo = new QComboBox();
        m_displayModeCombo->addItems({"值", "地址", "地址+值"});
        m_displayModeCombo->setCurrentIndex(static_cast<int>(m_displayMode));
        modeLayout->addWidget(m_displayModeCombo);
        modeLayout->addStretch();
        layout->addLayout(modeLayout);
        
        // 单元格列表
        auto listGroup = new QGroupBox("单元格列表");
        auto listLayout = new QVBoxLayout(listGroup);
        m_cellListWidget = new QListWidget();
        m_cellListWidget->setMaximumHeight(150);
        listLayout->addWidget(m_cellListWidget);
        layout->addWidget(listGroup);
        
        // 选中单元格信息
        auto selectedGroup = new QGroupBox("选中单元格");
        auto selectedLayout = new QVBoxLayout(selectedGroup);
        m_selectedCellLabel = new QLabel("(未选择)");
        m_selectedCellLabel->setWordWrap(true);
        m_selectedCellLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 5px; border: 1px solid #ccc; }");
        selectedLayout->addWidget(m_selectedCellLabel);
        layout->addWidget(selectedGroup);
        
        // 连接信号
        connect(m_cellListWidget, &QListWidget::currentRowChanged,
                this, &DisplayCellListModel::onCellSelectionChanged);
        connect(m_displayModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &DisplayCellListModel::onDisplayModeChanged);
        
        updateDisplay();
    }
    
    return m_widget;
}

void DisplayCellListModel::onCellSelectionChanged()
{
    if (m_cellListWidget) {
        m_selectedIndex = m_cellListWidget->currentRow();
        updateSelectedCell();
        qDebug() << "DisplayCellListModel: Selected cell index changed to" << m_selectedIndex;
    }
}

void DisplayCellListModel::onDisplayModeChanged()
{
    if (m_displayModeCombo) {
        m_displayMode = static_cast<DisplayMode>(m_displayModeCombo->currentIndex());
        updateDisplay();
        qDebug() << "DisplayCellListModel: Display mode changed to" << m_displayMode;
    }
}

void DisplayCellListModel::updateDisplay()
{
    if (!m_cellListWidget) return;
    
    m_cellListWidget->clear();
    
    if (!m_cellListData || m_cellListData->isEmpty()) {
        if (m_statusLabel) {
            m_statusLabel->setText("无单元格数据");
        }
        if (m_selectedCellLabel) {
            m_selectedCellLabel->setText("(无数据)");
        }
        return;
    }
    
    // 更新状态
    if (m_statusLabel) {
        m_statusLabel->setText(QString("共 %1 个单元格").arg(m_cellListData->count()));
    }
    
    // 填充列表
    for (int i = 0; i < m_cellListData->count(); ++i) {
        CellData cell = m_cellListData->at(i);
        QString displayText;
        
        switch (m_displayMode) {
            case ShowValues:
                displayText = cell.value().toString();
                break;
            case ShowAddresses:
                displayText = cell.address();
                break;
            case ShowBoth:
                displayText = QString("%1: %2").arg(cell.address()).arg(cell.value().toString());
                break;
        }
        
        m_cellListWidget->addItem(displayText);
    }
    
    // 恢复选择
    if (m_selectedIndex >= 0 && m_selectedIndex < m_cellListWidget->count()) {
        m_cellListWidget->setCurrentRow(m_selectedIndex);
    } else if (m_cellListWidget->count() > 0) {
        m_cellListWidget->setCurrentRow(0);
        m_selectedIndex = 0;
    }
    
    updateSelectedCell();
}

void DisplayCellListModel::updateSelectedCell()
{
    if (!m_cellListData || m_selectedIndex < 0 || m_selectedIndex >= m_cellListData->count()) {
        m_selectedCellData = std::make_shared<CellData>();
        if (m_selectedCellLabel) {
            m_selectedCellLabel->setText("(未选择)");
        }
        Q_EMIT dataUpdated(0);
        return;
    }
    
    // 更新选中的单元格数据
    CellData selectedCell = m_cellListData->at(m_selectedIndex);
    m_selectedCellData = std::make_shared<CellData>(selectedCell);
    
    // 更新显示
    if (m_selectedCellLabel) {
        int rowIndex = m_cellListData->rowIndexAt(m_selectedIndex);
        QString info = QString("地址: %1\n值: %2\n行索引: %3")
            .arg(selectedCell.address())
            .arg(selectedCell.value().toString())
            .arg(rowIndex >= 0 ? QString::number(rowIndex + 1) : "未知");
        m_selectedCellLabel->setText(info);
    }
    
    Q_EMIT dataUpdated(0);
    qDebug() << "DisplayCellListModel: Updated selected cell:" << selectedCell.address();
}
