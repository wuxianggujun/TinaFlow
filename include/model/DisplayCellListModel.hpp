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
#include <QListWidget>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QDebug>

#include "data/CellListData.hpp"
#include "data/CellData.hpp"

/**
 * @brief 显示单元格列表节点
 * 
 * 功能：
 * - 显示CellListData中的所有单元格
 * - 支持选择特定单元格
 * - 输出选中的单元格数据
 * 
 * 输入端口：
 * - 0: CellListData - 单元格列表
 * 
 * 输出端口：
 * - 0: CellData - 选中的单元格
 */
class DisplayCellListModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    DisplayCellListModel();
    ~DisplayCellListModel() override = default;

public:
    QString caption() const override { return "显示单元格列表"; }
    QString name() const override { return "DisplayCellList"; }

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
    void onCellSelectionChanged();
    void onDisplayModeChanged();

private:
    void updateDisplay();
    void updateSelectedCell();

private:
    // 输入数据
    std::shared_ptr<CellListData> m_cellListData;
    
    // 输出数据
    std::shared_ptr<CellData> m_selectedCellData;
    
    // 配置
    int m_selectedIndex = 0;
    enum DisplayMode { ShowValues = 0, ShowAddresses = 1, ShowBoth = 2 };
    DisplayMode m_displayMode = ShowBoth;
    
    // UI组件
    QWidget* m_widget = nullptr;
    QLabel* m_statusLabel = nullptr;
    QListWidget* m_cellListWidget = nullptr;
    QComboBox* m_displayModeCombo = nullptr;
    QLabel* m_selectedCellLabel = nullptr;
};
