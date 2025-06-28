//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "data/RowData.hpp"

#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QtNodes/NodeDelegateModel>
#include <QDebug>

/**
 * @brief 显示单行数据的节点模型
 * 
 * 这个节点接收一个行数据(RowData)作为输入，
 * 然后在表格中显示该行的所有列数据。
 * 适用于循环中查看当前处理的行数据。
 */
class DisplayRowModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    DisplayRowModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        m_widget->setMinimumSize(300, 150);
        
        auto* layout = new QVBoxLayout(m_widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);

        // 信息标签
        m_infoLabel = new QLabel("行: --");
        m_infoLabel->setStyleSheet("font-weight: bold; color: #2E86AB;");
        layout->addWidget(m_infoLabel);

        // 创建表格（单行显示）
        m_tableWidget = new QTableWidget();
        m_tableWidget->setRowCount(1); // 只显示一行
        m_tableWidget->setAlternatingRowColors(false);
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        
        // 隐藏行标题
        m_tableWidget->verticalHeader()->setVisible(false);
        
        // 设置表格样式
        m_tableWidget->setStyleSheet(
            "QTableWidget {"
            "    gridline-color: #d0d0d0;"
            "    background-color: white;"
            "    border: 1px solid #d0d0d0;"
            "}"
            "QTableWidget::item {"
            "    padding: 6px;"
            "    border: none;"
            "}"
            "QTableWidget::item:selected {"
            "    background-color: #3daee9;"
            "    color: white;"
            "}"
            "QHeaderView::section {"
            "    background-color: #f0f0f0;"
            "    padding: 4px;"
            "    border: 1px solid #d0d0d0;"
            "    font-weight: bold;"
            "}"
        );
        
        layout->addWidget(m_tableWidget);

        // 初始显示
        updateDisplay();
    }

    QString caption() const override
    {
        return tr("显示行");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("DisplayRow");
    }

    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return (portType == QtNodes::PortType::In) ? 1 : 0; // 只有输入端口，没有输出
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In)
        {
            return RowData().type();
        }
        return {"", ""};
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return nullptr; // 显示节点没有输出
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "DisplayRowModel::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << "DisplayRowModel: Received null nodeData";
            m_rowData.reset();
            updateDisplay();
            return;
        }
        
        m_rowData = std::dynamic_pointer_cast<RowData>(nodeData);
        if (m_rowData) {
            qDebug() << "DisplayRowModel: Successfully received RowData for row" 
                     << (m_rowData->rowIndex() + 1);
        } else {
            qDebug() << "DisplayRowModel: Failed to cast to RowData";
        }
        
        updateDisplay();
    }

    QJsonObject save() const override
    {
        return NodeDelegateModel::save(); // 调用基类方法保存model-name
    }

    void load(QJsonObject const& json) override
    {
        // 显示节点不需要加载状态
    }

private:
    void updateDisplay()
    {
        qDebug() << "DisplayRowModel::updateDisplay called";
        
        if (!m_rowData) {
            // 显示空状态
            m_infoLabel->setText("行: --");
            m_tableWidget->setColumnCount(0);
            qDebug() << "DisplayRowModel: No row data to display";
            return;
        }

        try {
            // 更新信息标签
            QString info;
            if (m_rowData->totalRows() > 0) {
                double progress = m_rowData->progressPercentage();
                info = QString("行: %1/%2 (%.1f%%)")
                    .arg(m_rowData->rowIndex() + 1)
                    .arg(m_rowData->totalRows())
                    .arg(progress);
            } else {
                info = QString("行: %1").arg(m_rowData->rowIndex() + 1);
            }
            m_infoLabel->setText(info);
            
            // 设置表格列数
            int cols = m_rowData->columnCount();
            m_tableWidget->setColumnCount(cols);
            
            // 设置列标题（A, B, C, ...）
            QStringList columnHeaders;
            for (int col = 0; col < cols; ++col) {
                QString header;
                int temp = col;
                do {
                    header = QChar('A' + (temp % 26)) + header;
                    temp = temp / 26 - 1;
                } while (temp >= 0);
                columnHeaders << header;
            }
            m_tableWidget->setHorizontalHeaderLabels(columnHeaders);
            
            // 填充数据
            for (int col = 0; col < cols; ++col) {
                QVariant cellValue = m_rowData->cellValue(col);
                QString displayText = cellValue.toString();
                
                auto* item = new QTableWidgetItem(displayText);
                
                // 根据数据类型设置不同的对齐方式
                if (cellValue.type() == QVariant::Int || 
                    cellValue.type() == QVariant::Double ||
                    cellValue.type() == QVariant::LongLong) {
                    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                } else {
                    item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                }
                
                // 如果是空值，设置特殊样式
                if (cellValue.isNull() || displayText.isEmpty()) {
                    item->setBackground(QColor(245, 245, 245));
                    item->setText("(空)");
                    item->setForeground(QColor(150, 150, 150));
                }
                
                m_tableWidget->setItem(0, col, item);
            }
            
            // 调整列宽
            m_tableWidget->resizeColumnsToContents();
            
            // 限制最大列宽
            for (int col = 0; col < cols; ++col) {
                int width = m_tableWidget->columnWidth(col);
                if (width > 120) {
                    m_tableWidget->setColumnWidth(col, 120);
                } else if (width < 60) {
                    m_tableWidget->setColumnWidth(col, 60);
                }
            }
            
            qDebug() << "DisplayRowModel: Updated display for row" 
                     << (m_rowData->rowIndex() + 1) << "with" << cols << "columns";
            
        } catch (const std::exception& e) {
            qDebug() << "DisplayRowModel: Error updating display:" << e.what();
            m_infoLabel->setText("错误: 无法显示数据");
            m_tableWidget->setColumnCount(0);
        }
    }

    QWidget* m_widget;
    QLabel* m_infoLabel;
    QTableWidget* m_tableWidget;
    
    std::shared_ptr<RowData> m_rowData;
};
