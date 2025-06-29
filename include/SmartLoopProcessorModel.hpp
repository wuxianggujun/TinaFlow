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
class SmartLoopProcessorModel : public QtNodes::NodeDelegateModel
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
};
