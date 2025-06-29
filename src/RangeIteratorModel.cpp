//
// Created by wuxianggujun on 25-6-29.
//

#include "RangeIteratorModel.hpp"

RangeIteratorModel::RangeIteratorModel()
{
    m_currentRowData = std::make_shared<RowData>();
    m_currentRowIndex = std::make_shared<IntegerData>(0);
    m_hasMoreRows = std::make_shared<BooleanData>(false);
    
    qDebug() << "RangeIteratorModel: Created";
}

QJsonObject RangeIteratorModel::save() const
{
    QJsonObject obj;
    obj["model-name"] = name();  // 保存节点类型名称
    obj["currentRow"] = m_currentRow;
    obj["isRunning"] = m_isRunning;
    return obj;
}

void RangeIteratorModel::load(const QJsonObject& obj)
{
    m_currentRow = obj["currentRow"].toInt();
    m_isRunning = obj["isRunning"].toBool();
    updateDisplay();
}

unsigned int RangeIteratorModel::nPorts(QtNodes::PortType const portType) const
{
    if (portType == QtNodes::PortType::In) {
        return 1; // RangeData输入
    } else {
        return 3; // RowData, IntegerData, BooleanData输出
    }
}

QtNodes::NodeDataType RangeIteratorModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        return RangeData().type(); // 范围数据输入
    } else {
        if (portIndex == 0) {
            return RowData().type();     // 当前行数据
        } else if (portIndex == 1) {
            return IntegerData().type(); // 当前行索引
        } else {
            return BooleanData().type(); // 是否有更多行
        }
    }
}

std::shared_ptr<QtNodes::NodeData> RangeIteratorModel::outData(QtNodes::PortIndex const port)
{
    if (port == 0) {
        return m_currentRowData;
    } else if (port == 1) {
        return m_currentRowIndex;
    } else if (port == 2) {
        return m_hasMoreRows;
    }
    return nullptr;
}

void RangeIteratorModel::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex)
{
    qDebug() << "RangeIteratorModel::setInData called, portIndex:" << portIndex;
    
    if (portIndex == 0) {
        if (auto rangeData = std::dynamic_pointer_cast<RangeData>(nodeData)) {
            m_rangeData = rangeData;
            updateIterator();
            qDebug() << "RangeIteratorModel: Received range data";
        } else {
            m_rangeData.reset();
            qDebug() << "RangeIteratorModel: Received null range data";
        }
    }
}

QWidget* RangeIteratorModel::embeddedWidget()
{
    if (!m_widget) {
        m_widget = new QWidget();
        auto layout = new QVBoxLayout(m_widget);
        
        // 状态显示
        m_statusLabel = new QLabel("状态: 停止");
        m_progressLabel = new QLabel("进度: 0/0");
        layout->addWidget(m_statusLabel);
        layout->addWidget(m_progressLabel);
        
        // 控制按钮
        auto buttonLayout = new QHBoxLayout();
        
        m_startButton = new QPushButton("开始");
        m_pauseButton = new QPushButton("暂停");
        m_resetButton = new QPushButton("重置");
        m_stepButton = new QPushButton("单步");
        
        m_pauseButton->setEnabled(false);
        
        buttonLayout->addWidget(m_startButton);
        buttonLayout->addWidget(m_pauseButton);
        buttonLayout->addWidget(m_resetButton);
        buttonLayout->addWidget(m_stepButton);
        
        layout->addLayout(buttonLayout);
        
        // 连接信号
        connect(m_startButton, &QPushButton::clicked, this, &RangeIteratorModel::onStartClicked);
        connect(m_pauseButton, &QPushButton::clicked, this, &RangeIteratorModel::onPauseClicked);
        connect(m_resetButton, &QPushButton::clicked, this, &RangeIteratorModel::onResetClicked);
        connect(m_stepButton, &QPushButton::clicked, this, &RangeIteratorModel::onStepClicked);
        
        updateDisplay();
    }
    
    return m_widget;
}

QString RangeIteratorModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In) {
        return "范围数据";
    } else {
        if (portIndex == 0) {
            return "当前行";
        } else if (portIndex == 1) {
            return "行索引";
        } else {
            return "有更多行";
        }
    }
}

void RangeIteratorModel::onStartClicked()
{
    if (!m_rangeData) {
        qDebug() << "RangeIteratorModel: No range data available";
        return;
    }
    
    m_isRunning = true;
    m_startButton->setEnabled(false);
    m_pauseButton->setEnabled(true);
    
    updateDisplay();
    qDebug() << "RangeIteratorModel: Started iteration";
}

void RangeIteratorModel::onPauseClicked()
{
    m_isRunning = false;
    m_startButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    
    updateDisplay();
    qDebug() << "RangeIteratorModel: Paused iteration";
}

void RangeIteratorModel::onResetClicked()
{
    m_currentRow = 0;
    m_isRunning = false;
    m_startButton->setEnabled(true);
    m_pauseButton->setEnabled(false);
    
    updateIterator();
    updateDisplay();
    qDebug() << "RangeIteratorModel: Reset iteration";
}

void RangeIteratorModel::onStepClicked()
{
    if (!m_rangeData) {
        qDebug() << "RangeIteratorModel: No range data available";
        return;
    }
    
    moveToNextRow();
    updateDisplay();
    qDebug() << "RangeIteratorModel: Step to row" << m_currentRow;
}

void RangeIteratorModel::updateIterator()
{
    if (m_rangeData) {
        m_totalRows = m_rangeData->rowCount();
        m_currentRow = 0;

        // 更新当前行数据
        if (m_totalRows > 0) {
            auto rowData = m_rangeData->rowData(m_currentRow);
            m_currentRowData = std::make_shared<RowData>(m_currentRow, rowData, m_totalRows);
            m_currentRowIndex->setValue(m_currentRow);
            m_hasMoreRows->setValue(m_currentRow < m_totalRows - 1);
        } else {
            m_currentRowData = std::make_shared<RowData>();
            m_currentRowIndex->setValue(0);
            m_hasMoreRows->setValue(false);
        }
    } else {
        m_totalRows = 0;
        m_currentRow = 0;
        m_currentRowData = std::make_shared<RowData>();
        m_currentRowIndex->setValue(0);
        m_hasMoreRows->setValue(false);
    }

    // 触发数据更新
    Q_EMIT dataUpdated(0);
    Q_EMIT dataUpdated(1);
    Q_EMIT dataUpdated(2);
}

void RangeIteratorModel::moveToNextRow()
{
    if (m_rangeData && m_currentRow < m_totalRows - 1) {
        m_currentRow++;

        // 更新当前行数据
        auto rowData = m_rangeData->rowData(m_currentRow);
        m_currentRowData = std::make_shared<RowData>(m_currentRow, rowData, m_totalRows);
        m_currentRowIndex->setValue(m_currentRow);
        m_hasMoreRows->setValue(m_currentRow < m_totalRows - 1);

        // 触发数据更新
        Q_EMIT dataUpdated(0);
        Q_EMIT dataUpdated(1);
        Q_EMIT dataUpdated(2);
    }
}

void RangeIteratorModel::updateDisplay()
{
    if (m_statusLabel) {
        m_statusLabel->setText(QString("状态: %1").arg(m_isRunning ? "运行中" : "停止"));
    }
    
    if (m_progressLabel) {
        m_progressLabel->setText(QString("进度: %1/%2").arg(m_currentRow + 1).arg(m_totalRows));
    }
}
