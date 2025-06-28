//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "data/RangeData.hpp"
#include "data/RowData.hpp"
#include "data/BooleanData.hpp"

#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QProgressBar>
#include <QTimer>
#include <QtNodes/NodeDelegateModel>
#include <QDebug>

/**
 * @brief 遍历行数据的循环节点模型
 * 
 * 这个节点接收一个范围数据(RangeData)作为输入，
 * 然后逐行输出每一行的数据(RowData)。
 * 用户可以控制循环的开始、暂停、停止。
 */
class ForEachRowModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    enum LoopState {
        Stopped,    // 停止状态
        Running,    // 运行状态
        Paused,     // 暂停状态
        Finished    // 完成状态
    };

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

        // 进度条
        m_progressBar = new QProgressBar();
        m_progressBar->setVisible(false);
        layout->addWidget(m_progressBar);

        // 控制按钮
        auto* buttonLayout = new QHBoxLayout();

        m_startButton = new QPushButton("开始");
        m_startButton->setEnabled(false);
        buttonLayout->addWidget(m_startButton);

        m_pauseButton = new QPushButton("暂停");
        m_pauseButton->setEnabled(false);
        buttonLayout->addWidget(m_pauseButton);

        m_stopButton = new QPushButton("停止");
        m_stopButton->setEnabled(false);
        buttonLayout->addWidget(m_stopButton);

        layout->addLayout(buttonLayout);

        // 自动开始选项
        auto* optionLayout = new QHBoxLayout();
        m_autoStartCheckBox = new QCheckBox("自动开始");
        m_autoStartCheckBox->setChecked(true); // 默认启用自动开始
        optionLayout->addWidget(m_autoStartCheckBox);
        optionLayout->addStretch();
        layout->addLayout(optionLayout);

        // 设置定时器用于控制循环速度
        m_timer = new QTimer(this);
        m_timer->setSingleShot(true);
        m_timer->setInterval(100); // 100ms间隔

        // 连接信号
        connect(m_startButton, &QPushButton::clicked, this, &ForEachRowModel::startLoop);
        connect(m_pauseButton, &QPushButton::clicked, this, &ForEachRowModel::pauseLoop);
        connect(m_stopButton, &QPushButton::clicked, this, &ForEachRowModel::stopLoop);
        connect(m_timer, &QTimer::timeout, this, &ForEachRowModel::processNextRow);

        // 初始状态
        m_currentState = Stopped;
        m_currentRowIndex = 0;
    }

    QString caption() const override
    {
        return tr("遍历行");
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
        if (portType == QtNodes::PortType::In) return 1;  // 输入：RangeData
        if (portType == QtNodes::PortType::Out) return 2; // 输出：当前行数据 + 完成信号
        return 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In)
        {
            return RangeData().type();
        }
        else if (portType == QtNodes::PortType::Out)
        {
            if (portIndex == 0) {
                return RowData().type();     // 当前行数据
            } else {
                return BooleanData().type(); // 完成信号
            }
        }
        return {"", ""};
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        if (port == 0) {
            return m_currentRowData;  // 当前行数据
        } else if (port == 1) {
            return m_finishedSignal;  // 完成信号
        }
        return nullptr;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "ForEachRowModel::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << "ForEachRowModel: Received null nodeData";
            m_rangeData.reset();
            updateUI();
            return;
        }
        
        m_rangeData = std::dynamic_pointer_cast<RangeData>(nodeData);
        if (m_rangeData) {
            qDebug() << "ForEachRowModel: Successfully received RangeData with"
                     << m_rangeData->rowCount() << "rows";
            resetLoop();

            // 自动输出第一行数据，让用户可以立即看到效果
            if (m_rangeData->rowCount() > 0) {
                auto firstRowData = m_rangeData->rowData(0);
                m_currentRowData = std::make_shared<RowData>(0, firstRowData, m_rangeData->rowCount());
                emit dataUpdated(0);
                qDebug() << "ForEachRowModel: Auto-output first row for preview";

                // 如果启用了自动开始，则自动开始循环
                if (m_autoStartCheckBox->isChecked()) {
                    QTimer::singleShot(500, this, &ForEachRowModel::startLoop); // 延迟500ms开始
                    qDebug() << "ForEachRowModel: Auto-start enabled, will begin loop in 500ms";
                }
            }
        } else {
            qDebug() << "ForEachRowModel: Failed to cast to RangeData";
        }

        updateUI();
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
        }
    }

private slots:
    void startLoop()
    {
        if (!m_rangeData || m_rangeData->isEmpty()) {
            qDebug() << "ForEachRowModel: No data to process";
            return;
        }

        qDebug() << "ForEachRowModel: Starting loop";
        m_currentState = Running;
        updateUI();
        
        // 开始处理第一行（或继续从当前行）
        processNextRow();
    }

    void pauseLoop()
    {
        qDebug() << "ForEachRowModel: Pausing loop";
        m_currentState = Paused;
        m_timer->stop();
        updateUI();
    }

    void stopLoop()
    {
        qDebug() << "ForEachRowModel: Stopping loop";
        m_currentState = Stopped;
        m_timer->stop();
        resetLoop();
        updateUI();
    }

    void processNextRow()
    {
        if (!m_rangeData || m_currentState != Running) {
            return;
        }

        if (m_currentRowIndex >= m_rangeData->rowCount()) {
            // 循环完成
            qDebug() << "ForEachRowModel: Loop finished";
            m_currentState = Finished;
            
            // 发送完成信号
            m_finishedSignal = std::make_shared<BooleanData>(true, "循环完成");
            emit dataUpdated(1);
            
            updateUI();
            return;
        }

        // 获取当前行数据
        auto rowData = m_rangeData->rowData(m_currentRowIndex);
        m_currentRowData = std::make_shared<RowData>(
            m_currentRowIndex, 
            rowData, 
            m_rangeData->rowCount()
        );

        qDebug() << "ForEachRowModel: Processing row" << (m_currentRowIndex + 1) 
                 << "of" << m_rangeData->rowCount();

        // 输出当前行数据
        emit dataUpdated(0);

        // 更新进度
        updateUI();

        // 移动到下一行
        m_currentRowIndex++;

        // 继续下一次循环
        if (m_currentState == Running) {
            m_timer->start();
        }
    }

private:
    void resetLoop()
    {
        m_currentRowIndex = 0;
        m_currentRowData.reset();
        m_finishedSignal.reset();
    }

    void updateUI()
    {
        if (!m_rangeData || m_rangeData->isEmpty()) {
            m_statusLabel->setText("等待数据");
            m_progressBar->setVisible(false);
            m_startButton->setEnabled(false);
            m_pauseButton->setEnabled(false);
            m_stopButton->setEnabled(false);
            return;
        }

        int totalRows = m_rangeData->rowCount();
        
        switch (m_currentState) {
            case Stopped:
                m_statusLabel->setText(QString("准备就绪 (%1行)").arg(totalRows));
                m_progressBar->setVisible(false);
                m_startButton->setEnabled(true);
                m_pauseButton->setEnabled(false);
                m_stopButton->setEnabled(false);
                break;
                
            case Running:
                m_statusLabel->setText(QString("运行中 %1/%2").arg(m_currentRowIndex + 1).arg(totalRows));
                m_progressBar->setVisible(true);
                m_progressBar->setMaximum(totalRows);
                m_progressBar->setValue(m_currentRowIndex + 1);
                m_startButton->setEnabled(false);
                m_pauseButton->setEnabled(true);
                m_stopButton->setEnabled(true);
                break;
                
            case Paused:
                m_statusLabel->setText(QString("已暂停 %1/%2").arg(m_currentRowIndex + 1).arg(totalRows));
                m_progressBar->setVisible(true);
                m_startButton->setEnabled(true);
                m_pauseButton->setEnabled(false);
                m_stopButton->setEnabled(true);
                break;
                
            case Finished:
                m_statusLabel->setText(QString("已完成 (%1行)").arg(totalRows));
                m_progressBar->setVisible(true);
                m_progressBar->setValue(totalRows);
                m_startButton->setEnabled(false);
                m_pauseButton->setEnabled(false);
                m_stopButton->setEnabled(true);
                break;
        }
    }

    QWidget* m_widget;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QPushButton* m_startButton;
    QPushButton* m_pauseButton;
    QPushButton* m_stopButton;
    QCheckBox* m_autoStartCheckBox;
    QTimer* m_timer;
    
    std::shared_ptr<RangeData> m_rangeData;
    std::shared_ptr<RowData> m_currentRowData;
    std::shared_ptr<BooleanData> m_finishedSignal;
    
    LoopState m_currentState;
    int m_currentRowIndex;
};
