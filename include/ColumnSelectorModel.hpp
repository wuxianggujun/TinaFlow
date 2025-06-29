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
#include <QDebug>

#include "data/RowData.hpp"
#include "data/CellData.hpp"

/**
 * @brief 列选择器节点 - 从行数据中选择特定列
 * 
 * 功能：
 * - 从行数据中提取指定列的单元格数据
 * - 支持列索引选择（0-based）
 * - 提供列名显示
 * 
 * 输入端口：
 * - 0: RowData - 行数据
 * 
 * 输出端口：
 * - 0: CellData - 选中列的单元格数据
 */
class ColumnSelectorModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    ColumnSelectorModel();
    ~ColumnSelectorModel() override = default;

public:
    QString caption() const override { return "列选择器"; }
    QString name() const override { return "ColumnSelector"; }

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
    void onColumnIndexChanged(int index);

private:
    void updateCellData();
    void updateDisplay();

private:
    // 输入数据
    std::shared_ptr<RowData> m_rowData;
    
    // 输出数据
    std::shared_ptr<CellData> m_cellData;
    
    // 配置
    int m_columnIndex = 0;
    
    // UI组件
    QWidget* m_widget = nullptr;
    QLabel* m_infoLabel = nullptr;
    QSpinBox* m_columnSpinBox = nullptr;
    QLabel* m_previewLabel = nullptr;
};


