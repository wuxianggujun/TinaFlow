//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "data/RangeData.hpp"
#include "data/RowData.hpp"
#include "data/BooleanData.hpp"
#include "data/CellData.hpp"

#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QtNodes/NodeDelegateModel>
#include <QDebug>

/**
 * @brief 真正的循环遍历节点
 *
 * 这个节点接收一个范围数据(RangeData)作为输入，
 * 自动遍历每一行并输出：
 * - 当前行数据(RowData)
 * - 循环控制信号(继续/结束)
 *
 * 使用方式：
 * 1. 连接RangeData到输入
 * 2. 连接RowData输出到处理节点
 * 3. 连接控制信号到条件判断
 * 4. 点击运行开始自动循环
 */
class ForEachRowModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    ForEachRowModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        m_widget->setMinimumSize(200, 120);
        
        auto* layout = new QVBoxLayout(m_widget);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);

        // 状态标签
        m_statusLabel = new QLabel("等待数据");
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setStyleSheet("font-weight: bold; color: #2E86AB;");
        layout->addWidget(m_statusLabel);

        // 进度显示
        m_progressLabel = new QLabel("进度: --");
        m_progressLabel->setAlignment(Qt::AlignCenter);
        m_progressLabel->setStyleSheet("color: #666666; font-size: 11px;");
        layout->addWidget(m_progressLabel);

        // 控制按钮
        auto* buttonLayout = new QHBoxLayout();

        m_startButton = new QPushButton("开始循环");
        m_startButton->setEnabled(false);
        m_startButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }");
        buttonLayout->addWidget(m_startButton);

        m_stopButton = new QPushButton("停止");
        m_stopButton->setEnabled(false);
        m_stopButton->setStyleSheet("QPushButton { background-color: #F44336; color: white; }");
        buttonLayout->addWidget(m_stopButton);

        layout->addLayout(buttonLayout);

        // 设置定时器用于控制循环
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        m_timer->setInterval(500); // 500ms间隔，让用户能看到过程

        // 连接信号
        connect(m_startButton, &QPushButton::clicked, this, &ForEachRowModel::startLoop);
        connect(m_stopButton, &QPushButton::clicked, this, &ForEachRowModel::stopLoop);
        connect(m_timer, &QTimer::timeout, this, &ForEachRowModel::processNextRow);

        // 初始状态
        m_currentRowIndex = 0;
        m_targetColumnIndex = 0; // 默认提取第一列（A列）
        m_isRunning = false;
    }

    QString caption() const override
    {
        return tr("行提取器");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("ForEachRow");
    }

    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        if (portType == QtNodes::PortType::In) return 2;  // 输入：RangeData + 可选条件
        if (portType == QtNodes::PortType::Out) return 3; // 输出：当前行数据 + 当前单元格 + 循环状态
        return 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In)
        {
            if (portIndex == 0) {
                return RangeData().type();   // 数据源
            } else {
                return BooleanData().type(); // 可选条件（true=继续，false=停止）
            }
        }
        else if (portType == QtNodes::PortType::Out)
        {
            if (portIndex == 0) {
                return RowData().type();     // 当前行数据
            } else if (portIndex == 1) {
                return CellData().type();    // 当前单元格数据（用于StringCompare）
            } else {
                return BooleanData().type(); // 循环状态（true=继续，false=结束）
            }
        }
        return {"", ""};
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        if (port == 0) {
            return m_currentRowData;  // 当前行数据
        } else if (port == 1) {
            return m_currentCellData; // 当前单元格数据
        } else if (port == 2) {
            return m_loopStatus;      // 循环状态
        }
        return nullptr;
    }

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In) {
            if (portIndex == 0) {
                return "范围数据";
            } else if (portIndex == 1) {
                return "循环条件";
            }
        } else if (portType == QtNodes::PortType::Out) {
            if (portIndex == 0) {
                return "当前行";
            } else if (portIndex == 1) {
                return "当前单元格";
            } else if (portIndex == 2) {
                return "循环状态";
            }
        }
        return "";
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "ForEachRowModel::setInData called, portIndex:" << portIndex;

        if (portIndex == 0) {
            // 端口0：RangeData输入
            if (!nodeData) {
                qDebug() << "ForEachRowModel: Received null RangeData";
                m_rangeData.reset();
                updateDisplay();
                return;
            }

            m_rangeData = std::dynamic_pointer_cast<RangeData>(nodeData);
            if (m_rangeData) {
                qDebug() << "ForEachRowModel: Successfully received RangeData with"
                         << m_rangeData->rowCount() << "rows";
                // 不自动启动，只输出第一行作为预览
                if (!m_isRunning) {
                    m_currentRowIndex = 0;
                    updateCurrentRow();
                    qDebug() << "ForEachRowModel: Data received, showing first row as preview";
                }
            } else {
                qDebug() << "ForEachRowModel: Failed to cast to RangeData";
            }

        } else if (portIndex == 1) {
            // 端口1：BooleanData条件输入
            if (!nodeData) {
                qDebug() << "ForEachRowModel: Received null condition data";
                m_conditionData.reset();
                return;
            }

            m_conditionData = std::dynamic_pointer_cast<BooleanData>(nodeData);
            if (m_conditionData) {
                bool conditionValue = m_conditionData->value();
                qDebug() << "ForEachRowModel: Received condition:" << conditionValue;

                // 根据条件决定是否继续循环
                if (m_isRunning) {
                    if (!conditionValue) {
                        qDebug() << "ForEachRowModel: Condition is false, stopping loop";
                        stopLoop();
                    } else {
                        qDebug() << "ForEachRowModel: Condition is true, continuing to next row";
                        // 移动到下一行并继续
                        m_currentRowIndex++;
                        updateDisplay();

                        if (m_currentRowIndex < m_rangeData->rowCount()) {
                            QTimer::singleShot(m_timer->interval(), this, &ForEachRowModel::processNextRow);
                        } else {
                            // 自然结束
                            stopLoop();
                        }
                    }
                }
            } else {
                qDebug() << "ForEachRowModel: Failed to cast to BooleanData";
            }
        }

        updateDisplay();
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["currentRowIndex"] = m_currentRowIndex;
        return modelJson;
    }

    void load(QJsonObject const& json) override
    {
        if (json.contains("currentRowIndex")) {
            m_currentRowIndex = json["currentRowIndex"].toInt();
            updateCurrentRow();
        }
    }

    // 公共方法供属性面板调用
    void setCurrentRowIndex(int rowIndex)
    {
        if (m_rangeData && rowIndex >= 0 && rowIndex < m_rangeData->rowCount()) {
            m_currentRowIndex = rowIndex;
            updateCurrentRow();
            updateDisplay();
        }
    }

    int getCurrentRowIndex() const
    {
        return m_currentRowIndex;
    }

    int getTotalRows() const
    {
        return m_rangeData ? m_rangeData->rowCount() : 0;
    }

    int getTotalColumns() const
    {
        if (!m_rangeData || m_rangeData->isEmpty()) {
            return 0;
        }
        return m_rangeData->columnCount();
    }

    void setTargetColumn(int columnIndex)
    {
        m_targetColumnIndex = columnIndex;
        if (!m_isRunning && m_rangeData) {
            // 如果不在运行中，立即更新当前行的单元格数据
            updateCurrentRow();
        }
    }

    int getTargetColumn() const
    {
        return m_targetColumnIndex;
    }

private:
    bool hasConditionConnection() const
    {
        // 简单的方法：检查是否接收过条件数据
        // 在实际应用中，可以通过检查图模型的连接来判断
        return m_conditionData != nullptr;
    }

public slots:
    void startLoop()
    {
        if (!m_rangeData || m_rangeData->isEmpty()) {
            qDebug() << "ForEachRowModel: No data to process";
            return;
        }

        if (m_isRunning) {
            qDebug() << "ForEachRowModel: Loop is already running, ignoring start request";
            return;
        }

        qDebug() << "ForEachRowModel: Starting loop";
        m_isRunning = true;
        m_currentRowIndex = 0; // 从第一行开始

        updateDisplay();
        processNextRow(); // 开始处理第一行
    }

private:

private slots:

    void stopLoop()
    {
        qDebug() << "ForEachRowModel: Stopping loop";
        m_isRunning = false;
        m_timer->stop();

        // 发送循环结束信号
        m_loopStatus = std::make_shared<BooleanData>(false, "Loop stopped");
        emit dataUpdated(2);

        updateDisplay();
    }

    void processNextRow()
    {
        if (!m_rangeData || !m_isRunning) {
            return;
        }

        if (m_currentRowIndex >= m_rangeData->rowCount()) {
            // 循环完成
            qDebug() << "ForEachRowModel: Loop completed";
            m_isRunning = false;

            // 发送循环完成信号
            m_loopStatus = std::make_shared<BooleanData>(false, "Loop completed");
            emit dataUpdated(2);

            updateDisplay();
            return;
        }

        // 处理当前行
        updateCurrentRow();

        // 发送循环继续信号
        m_loopStatus = std::make_shared<BooleanData>(true, QString("Processing row %1").arg(m_currentRowIndex + 1));
        emit dataUpdated(2); // 循环状态端口

        // 检查是否有条件输入连接
        bool hasConditionInput = hasConditionConnection();

        if (hasConditionInput) {
            // 有条件输入，等待条件反馈，不自动继续
            qDebug() << "ForEachRowModel: Waiting for condition feedback";
        } else {
            // 没有条件输入，自动继续下一行
            m_currentRowIndex++;
            updateDisplay();

            if (m_isRunning && m_currentRowIndex < m_rangeData->rowCount()) {
                m_timer->start();
            } else if (m_currentRowIndex >= m_rangeData->rowCount()) {
                // 自然结束
                stopLoop();
            }
        }
    }

private:
    void updateCurrentRow()
    {
        if (!m_rangeData || m_currentRowIndex >= m_rangeData->rowCount()) {
            m_currentRowData.reset();
            return;
        }

        // 获取当前行数据
        auto rowData = m_rangeData->rowData(m_currentRowIndex);
        m_currentRowData = std::make_shared<RowData>(
            m_currentRowIndex,
            rowData,
            m_rangeData->rowCount()
        );

        // 获取指定列的单元格数据
        if (m_targetColumnIndex >= 0 && m_targetColumnIndex < static_cast<int>(rowData.size())) {
            QVariant cellValue = rowData[m_targetColumnIndex];
            QString cellAddress = QString("%1%2")
                .arg(QChar('A' + m_targetColumnIndex))
                .arg(m_currentRowIndex + 1);
            m_currentCellData = std::make_shared<CellData>(cellAddress, cellValue);
        } else {
            m_currentCellData.reset();
        }

        qDebug() << "ForEachRowModel: Updated to row" << (m_currentRowIndex + 1)
                 << "of" << m_rangeData->rowCount()
                 << "column" << QChar('A' + m_targetColumnIndex);

        // 输出当前行数据和单元格数据
        emit dataUpdated(0); // 行数据
        emit dataUpdated(1); // 单元格数据
    }

    void updateDisplay()
    {
        if (!m_rangeData || m_rangeData->isEmpty()) {
            m_statusLabel->setText("等待数据");
            m_progressLabel->setText("进度: --");
            m_startButton->setEnabled(false);
            m_stopButton->setEnabled(false);
            return;
        }

        int totalRows = m_rangeData->rowCount();

        if (m_isRunning) {
            m_statusLabel->setText("循环运行中...");
            m_progressLabel->setText(QString("进度: %1/%2").arg(m_currentRowIndex + 1).arg(totalRows));
            m_startButton->setEnabled(false);
            m_stopButton->setEnabled(true);
        } else {
            m_statusLabel->setText(QString("准备就绪 (%1行)").arg(totalRows));
            m_progressLabel->setText(QString("当前: %1/%2").arg(m_currentRowIndex + 1).arg(totalRows));
            m_startButton->setEnabled(true);
            m_stopButton->setEnabled(false);
        }
    }

    QWidget* m_widget;
    QLabel* m_statusLabel;
    QLabel* m_progressLabel;
    QPushButton* m_startButton;
    QPushButton* m_stopButton;
    QTimer* m_timer;

    std::shared_ptr<RangeData> m_rangeData;
    std::shared_ptr<RowData> m_currentRowData;
    std::shared_ptr<CellData> m_currentCellData;
    std::shared_ptr<BooleanData> m_loopStatus;
    std::shared_ptr<BooleanData> m_conditionData;

    int m_currentRowIndex;
    int m_targetColumnIndex; // 要提取的列索引（用于StringCompare）
    bool m_isRunning;
};
