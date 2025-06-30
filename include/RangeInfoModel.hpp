//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "BaseDisplayModel.hpp"
#include "data/RangeData.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QDebug>

/**
 * @brief 显示范围信息的节点模型
 *
 * 这个节点接收一个范围数据(RangeData)作为输入，
 * 然后显示范围的详细信息：
 * - 行数和列数
 * - 范围地址
 * - 数据预览
 *
 * 这个节点纯粹用于信息显示，没有输出端口。
 */
class RangeInfoModel : public BaseDisplayModel<RangeData>
{
    Q_OBJECT

public:
    RangeInfoModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        m_widget->setMinimumSize(200, 120);
        
        auto* layout = new QVBoxLayout(m_widget);
        layout->setContentsMargins(6, 6, 6, 6);
        layout->setSpacing(4);

        // 创建信息框架
        m_frame = new QFrame();
        m_frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        m_frame->setLineWidth(1);
        m_frame->setStyleSheet(
            "QFrame {"
            "    background-color: #f8f9fa;"
            "    border: 1px solid #dee2e6;"
            "    border-radius: 4px;"
            "}"
        );
        layout->addWidget(m_frame);

        auto* frameLayout = new QVBoxLayout(m_frame);
        frameLayout->setContentsMargins(8, 8, 8, 8);
        frameLayout->setSpacing(6);

        // 标题
        QLabel* titleLabel = new QLabel("范围信息");
        titleLabel->setStyleSheet(
            "font-weight: bold; "
            "font-size: 12px; "
            "color: #495057; "
            "padding-bottom: 4px;"
        );
        titleLabel->setAlignment(Qt::AlignCenter);
        frameLayout->addWidget(titleLabel);

        // 信息网格
        auto* gridLayout = new QGridLayout();
        gridLayout->setSpacing(4);

        // 行数
        gridLayout->addWidget(new QLabel("行数:"), 0, 0);
        m_rowCountLabel = new QLabel("--");
        m_rowCountLabel->setStyleSheet("font-weight: bold; color: #007bff;");
        gridLayout->addWidget(m_rowCountLabel, 0, 1);

        // 列数
        gridLayout->addWidget(new QLabel("列数:"), 1, 0);
        m_columnCountLabel = new QLabel("--");
        m_columnCountLabel->setStyleSheet("font-weight: bold; color: #28a745;");
        gridLayout->addWidget(m_columnCountLabel, 1, 1);

        // 范围
        gridLayout->addWidget(new QLabel("范围:"), 2, 0);
        m_rangeLabel = new QLabel("--");
        m_rangeLabel->setStyleSheet("font-weight: bold; color: #6f42c1;");
        gridLayout->addWidget(m_rangeLabel, 2, 1);

        // 总单元格数
        gridLayout->addWidget(new QLabel("单元格:"), 3, 0);
        m_cellCountLabel = new QLabel("--");
        m_cellCountLabel->setStyleSheet("font-weight: bold; color: #fd7e14;");
        gridLayout->addWidget(m_cellCountLabel, 3, 1);

        frameLayout->addLayout(gridLayout);

        // 状态标签
        m_statusLabel = new QLabel("等待数据输入");
        m_statusLabel->setAlignment(Qt::AlignCenter);
        m_statusLabel->setStyleSheet(
            "color: #6c757d; "
            "font-size: 10px; "
            "font-style: italic; "
            "padding-top: 4px;"
        );
        frameLayout->addWidget(m_statusLabel);

        // 初始显示
        updateDisplay();
    }

    QString caption() const override
    {
        return tr("范围信息");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("RangeInfo");
    }

    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

protected:
    // 实现基类的纯虚函数
    QString getNodeTypeName() const override
    {
        return "RangeInfoModel";
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
        qDebug() << "RangeInfoModel::updateDisplay called";

        auto rangeData = getData();
        if (!hasValidData()) {
            // 显示空状态
            m_rowCountLabel->setText("--");
            m_columnCountLabel->setText("--");
            m_rangeLabel->setText("--");
            m_cellCountLabel->setText("--");
            m_statusLabel->setText("等待数据输入");
            
            m_frame->setStyleSheet(
                "QFrame {"
                "    background-color: #f8f9fa;"
                "    border: 1px solid #dee2e6;"
                "    border-radius: 4px;"
                "}"
            );
            
            qDebug() << "RangeInfoModel: No range data to display";
            return;
        }

        try {
            int rows = rangeData->rowCount();
            int cols = rangeData->columnCount();
            int totalCells = rows * cols;
            
            // 更新信息
            m_rowCountLabel->setText(QString::number(rows));
            m_columnCountLabel->setText(QString::number(cols));
            m_cellCountLabel->setText(QString::number(totalCells));
            
            // 计算范围地址（假设从A1开始）
            QString endColumn;
            int temp = cols - 1;
            do {
                endColumn = QChar('A' + (temp % 26)) + endColumn;
                temp = temp / 26 - 1;
            } while (temp >= 0);
            
            QString rangeAddress = QString("A1:%1%2").arg(endColumn).arg(rows);
            m_rangeLabel->setText(rangeAddress);
            
            // 更新状态
            m_statusLabel->setText("数据已加载");
            
            // 设置成功状态的样式
            m_frame->setStyleSheet(
                "QFrame {"
                "    background-color: #d4edda;"
                "    border: 1px solid #c3e6cb;"
                "    border-radius: 4px;"
                "}"
            );
            
            qDebug() << "RangeInfoModel: Updated display -" 
                     << "Rows:" << rows 
                     << "Cols:" << cols
                     << "Range:" << rangeAddress;
            
        } catch (const std::exception& e) {
            qDebug() << "RangeInfoModel: Error updating display:" << e.what();
            m_statusLabel->setText(QString("错误: %1").arg(e.what()));
            
            m_frame->setStyleSheet(
                "QFrame {"
                "    background-color: #f8d7da;"
                "    border: 1px solid #f5c6cb;"
                "    border-radius: 4px;"
                "}"
            );
        }
    }

private:
    QWidget* m_widget;
    QFrame* m_frame;
    QLabel* m_rowCountLabel;
    QLabel* m_columnCountLabel;
    QLabel* m_rangeLabel;
    QLabel* m_cellCountLabel;
    QLabel* m_statusLabel;
};
