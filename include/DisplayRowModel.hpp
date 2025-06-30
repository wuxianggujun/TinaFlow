//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "BaseDisplayModel.hpp"
#include "data/RowData.hpp"

#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QDebug>

/**
 * @brief 显示单行数据的节点模型
 *
 * 这个节点接收一个行数据(RowData)作为输入，
 * 然后在表格中显示该行的所有列数据。
 * 适用于循环中查看当前处理的行数据。
 */
class DisplayRowModel : public BaseDisplayModel<RowData>
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

protected:
    // 实现基类的纯虚函数
    QString getNodeTypeName() const override
    {
        return "DisplayRowModel";
    }

    QString getDataTypeName() const override
    {
        return "RowData";
    }

    void updateDisplay() override
    {
        qDebug() << "DisplayRowModel::updateDisplay called";

        auto rowData = getData();
        if (!hasValidData()) {
            // 显示空状态
            m_infoLabel->setText("行: --");
            m_tableWidget->setColumnCount(0);
            qDebug() << "DisplayRowModel: No row data to display";
            return;
        }

        try {
            // 更新信息标签
            QString info;
            if (rowData->totalRows() > 0) {
                double progress = rowData->progressPercentage();
                info = QString("行: %1/%2 (%.1f%%)")
                    .arg(rowData->rowIndex() + 1)
                    .arg(rowData->totalRows())
                    .arg(progress, 0, 'f', 1);
            } else {
                info = QString("行: %1").arg(rowData->rowIndex() + 1);
            }
            m_infoLabel->setText(info);
            
            // 设置表格列数
            int cols = rowData->columnCount();
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
                QVariant cellValue = rowData->cellValue(col);
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
                     << (rowData->rowIndex() + 1) << "with" << cols << "columns";
            
        } catch (const std::exception& e) {
            qDebug() << "DisplayRowModel: Error updating display:" << e.what();
            m_infoLabel->setText("错误: 无法显示数据");
            m_tableWidget->setColumnCount(0);
        }
    }

private:
    QWidget* m_widget;
    QLabel* m_infoLabel;
    QTableWidget* m_tableWidget;
};
