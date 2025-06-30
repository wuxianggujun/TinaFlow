//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "BaseDisplayModel.hpp"
#include "data/RangeData.hpp"

#include <QTableWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QDebug>

/**
 * @brief 显示Excel范围数据的节点模型
 *
 * 这个节点接收一个范围数据(RangeData)作为输入，
 * 然后在表格中显示所有数据。
 * 这是数据流的终端节点，用于查看批量处理结果。
 */
class DisplayRangeModel : public BaseDisplayModel<RangeData>
{
    Q_OBJECT

public:
    DisplayRangeModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        m_widget->setMinimumSize(300, 200);
        
        auto* layout = new QVBoxLayout(m_widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(4);

        // 信息标签
        m_infoLabel = new QLabel("范围: --");
        m_infoLabel->setStyleSheet("font-weight: bold; color: #2E86AB;");
        layout->addWidget(m_infoLabel);

        // 创建表格
        m_tableWidget = new QTableWidget();
        m_tableWidget->setAlternatingRowColors(true);
        m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);
        m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
        
        // 设置表格样式
        m_tableWidget->setStyleSheet(
            "QTableWidget {"
            "    gridline-color: #d0d0d0;"
            "    background-color: white;"
            "}"
            "QTableWidget::item {"
            "    padding: 4px;"
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
        return tr("显示范围");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("DisplayRange");
    }

    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

protected:
    // 实现基类的纯虚函数
    QString getNodeTypeName() const override
    {
        return "DisplayRangeModel";
    }

    QString getDataTypeName() const override
    {
        return "RangeData";
    }

    bool isDataValid(std::shared_ptr<RangeData> data) const override
    {
        return data && !data->isEmpty();
    }

    void updateDisplay() override
    {
        qDebug() << "DisplayRangeModel::updateDisplay called";

        auto rangeData = getData();
        if (!hasValidData()) {
            // 显示空状态
            m_infoLabel->setText("范围: --");
            m_tableWidget->clear();
            m_tableWidget->setRowCount(0);
            m_tableWidget->setColumnCount(0);
            qDebug() << "DisplayRangeModel: No valid range data to display";
            return;
        }

        try {
            // 更新信息标签
            QString info = QString("范围: %1 (%2行 x %3列)")
                .arg(rangeData->rangeAddress())
                .arg(rangeData->rowCount())
                .arg(rangeData->columnCount());
            m_infoLabel->setText(info);
            
            // 设置表格大小
            int rows = rangeData->rowCount();
            int cols = rangeData->columnCount();
            
            m_tableWidget->setRowCount(rows);
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
            
            // 设置行标题（1, 2, 3, ...）
            QStringList rowHeaders;
            for (int row = 0; row < rows; ++row) {
                rowHeaders << QString::number(row + 1);
            }
            m_tableWidget->setVerticalHeaderLabels(rowHeaders);
            
            // 填充数据
            for (int row = 0; row < rows; ++row) {
                for (int col = 0; col < cols; ++col) {
                    QVariant cellValue = rangeData->cellValue(row, col);
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
                    
                    m_tableWidget->setItem(row, col, item);
                }
            }
            
            // 调整列宽
            m_tableWidget->resizeColumnsToContents();
            
            // 限制最大列宽
            for (int col = 0; col < cols; ++col) {
                int width = m_tableWidget->columnWidth(col);
                if (width > 150) {
                    m_tableWidget->setColumnWidth(col, 150);
                }
            }
            
            qDebug() << "DisplayRangeModel: Updated display with" 
                     << rows << "rows x" << cols << "cols";
            
        } catch (const std::exception& e) {
            qDebug() << "DisplayRangeModel: Error updating display:" << e.what();
            m_infoLabel->setText("错误: 无法显示数据");
            m_tableWidget->clear();
        }
    }

private:
    QWidget* m_widget;
    QLabel* m_infoLabel;
    QTableWidget* m_tableWidget;
};
