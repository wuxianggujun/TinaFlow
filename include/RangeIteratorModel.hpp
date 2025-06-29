//
// Created by wuxianggujun on 25-6-29.
//

#pragma once

#include <QtNodes/NodeDelegateModel>
#include <QObject>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDebug>

#include "data/RangeData.hpp"
#include "data/RowData.hpp"
#include "data/BooleanData.hpp"
#include "data/IntegerData.hpp"

/**
 * @brief 范围迭代器节点 - 纯粹的循环控制
 * 
 * 功能：
 * - 遍历指定范围的每一行
 * - 输出当前行索引和行数据
 * - 提供循环控制（开始/暂停/重置）
 * 
 * 输入端口：
 * - 0: RangeData - 要遍历的数据范围
 * 
 * 输出端口：
 * - 0: RowData - 当前行的完整数据
 * - 1: IntegerData - 当前行索引（从0开始）
 * - 2: BooleanData - 是否还有更多行（true=继续，false=结束）
 */
class RangeIteratorModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    RangeIteratorModel();
    ~RangeIteratorModel() override = default;

public:
    QString caption() const override { return "范围迭代器"; }
    QString name() const override { return "RangeIterator"; }

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
    void onStartClicked();
    void onPauseClicked();
    void onResetClicked();
    void onStepClicked();

private:
    void updateIterator();
    void moveToNextRow();
    void updateDisplay();

private:
    // 输入数据
    std::shared_ptr<RangeData> m_rangeData;
    
    // 输出数据
    std::shared_ptr<RowData> m_currentRowData;
    std::shared_ptr<IntegerData> m_currentRowIndex;
    std::shared_ptr<BooleanData> m_hasMoreRows;
    
    // 迭代状态
    int m_currentRow = 0;
    int m_totalRows = 0;
    bool m_isRunning = false;
    
    // UI组件
    QWidget* m_widget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_progressLabel = nullptr;
    QPushButton* m_startButton = nullptr;
    QPushButton* m_pauseButton = nullptr;
    QPushButton* m_resetButton = nullptr;
    QPushButton* m_stepButton = nullptr;
};


