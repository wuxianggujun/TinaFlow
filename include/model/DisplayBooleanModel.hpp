//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "BaseDisplayModel.hpp"
#include "data/BooleanData.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QDebug>

/**
 * @brief 显示布尔值的节点模型
 *
 * 这个节点接收一个布尔数据(BooleanData)作为输入，
 * 然后在UI中显示布尔值和相关描述信息。
 * 使用不同的颜色和图标来区分True和False。
 */
class DisplayBooleanModel : public BaseDisplayModel<BooleanData>
{
    Q_OBJECT

public:
    DisplayBooleanModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        m_widget->setMinimumSize(150, 80);
        
        auto* layout = new QVBoxLayout(m_widget);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(4);

        // 创建显示框架
        m_frame = new QFrame();
        m_frame->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
        m_frame->setLineWidth(2);
        layout->addWidget(m_frame);

        auto* frameLayout = new QVBoxLayout(m_frame);
        frameLayout->setContentsMargins(6, 6, 6, 6);
        frameLayout->setSpacing(2);

        // 结果标签（显示True/False）
        m_resultLabel = new QLabel("--");
        m_resultLabel->setAlignment(Qt::AlignCenter);
        m_resultLabel->setStyleSheet(
            "font-size: 16px; "
            "font-weight: bold; "
            "padding: 4px;"
        );
        frameLayout->addWidget(m_resultLabel);

        // 描述标签
        m_descriptionLabel = new QLabel("等待输入");
        m_descriptionLabel->setAlignment(Qt::AlignCenter);
        m_descriptionLabel->setWordWrap(true);
        m_descriptionLabel->setStyleSheet(
            "font-size: 10px; "
            "color: #666666;"
        );
        frameLayout->addWidget(m_descriptionLabel);

        // 初始显示
        updateDisplay();
    }

    QString caption() const override
    {
        return tr("显示布尔值");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("DisplayBoolean");
    }

    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

protected:
    // 实现基类的纯虚函数
    QString getNodeTypeName() const override
    {
        return "DisplayBooleanModel";
    }

    QString getDataTypeName() const override
    {
        return "BooleanData";
    }

    void updateDisplay() override
    {
        qDebug() << "DisplayBooleanModel::updateDisplay called";

        auto booleanData = getData();
        if (!hasValidData()) {
            // 显示空状态
            m_resultLabel->setText("--");
            m_descriptionLabel->setText("等待输入");
            m_frame->setStyleSheet(
                "QFrame {"
                "    background-color: #f0f0f0;"
                "    border: 2px solid #cccccc;"
                "    border-radius: 4px;"
                "}"
            );
            qDebug() << "DisplayBooleanModel: No boolean data to display";
            return;
        }

        try {
            bool value = booleanData->value();
            QString description = booleanData->description();
            
            if (value) {
                // True状态 - 绿色
                m_resultLabel->setText("✓ TRUE");
                m_resultLabel->setStyleSheet(
                    "font-size: 16px; "
                    "font-weight: bold; "
                    "color: white; "
                    "background-color: #4CAF50; "
                    "border-radius: 4px; "
                    "padding: 4px;"
                );
                m_frame->setStyleSheet(
                    "QFrame {"
                    "    background-color: #E8F5E8;"
                    "    border: 2px solid #4CAF50;"
                    "    border-radius: 4px;"
                    "}"
                );
            } else {
                // False状态 - 红色
                m_resultLabel->setText("✗ FALSE");
                m_resultLabel->setStyleSheet(
                    "font-size: 16px; "
                    "font-weight: bold; "
                    "color: white; "
                    "background-color: #F44336; "
                    "border-radius: 4px; "
                    "padding: 4px;"
                );
                m_frame->setStyleSheet(
                    "QFrame {"
                    "    background-color: #FFF0F0;"
                    "    border: 2px solid #F44336;"
                    "    border-radius: 4px;"
                    "}"
                );
            }
            
            // 设置描述
            if (description.isEmpty()) {
                m_descriptionLabel->setText(QString("结果: %1").arg(booleanData->localizedString()));
            } else {
                m_descriptionLabel->setText(description);
            }
            
            qDebug() << "DisplayBooleanModel: Updated display -" 
                     << "Value:" << value 
                     << "Description:" << description;
            
        } catch (const std::exception& e) {
            qDebug() << "DisplayBooleanModel: Error updating display:" << e.what();
            m_resultLabel->setText("错误");
            m_descriptionLabel->setText(QString("错误: %1").arg(e.what()));
            m_frame->setStyleSheet(
                "QFrame {"
                "    background-color: #FFF8E1;"
                "    border: 2px solid #FF9800;"
                "    border-radius: 4px;"
                "}"
            );
        }
    }

protected:
    // 重写基类方法，添加布尔值特定的属性
    void addDataSpecificProperties(PropertyWidget* propertyWidget) override
    {
        if (!hasValidData()) return;

        try {
            auto booleanData = getData();
            bool value = booleanData->value();
            QString description = booleanData->description();

            propertyWidget->addSeparator();
            propertyWidget->addTitle("布尔值信息");

            // 布尔值显示（带颜色）
            QString valueColor = value ? "color: #28a745; font-weight: bold;" : "color: #dc3545; font-weight: bold;";
            QString valueText = value ? "✓ TRUE" : "✗ FALSE";
            propertyWidget->addInfoProperty("布尔值", valueText, valueColor);

            // 本地化字符串
            propertyWidget->addInfoProperty("本地化显示", booleanData->localizedString(), "color: #666;");

            // 描述信息
            if (!description.isEmpty()) {
                propertyWidget->addInfoProperty("描述", description, "color: #333;");
            }

            // 统计信息
            propertyWidget->addSeparator();
            propertyWidget->addTitle("统计信息");

            QString statusText = value ? "条件满足" : "条件不满足";
            QString statusColor = value ? "color: #28a745;" : "color: #dc3545;";
            propertyWidget->addInfoProperty("状态", statusText, statusColor);

        } catch (const std::exception& e) {
            propertyWidget->addInfoProperty("错误", QString("无法读取数据: %1").arg(e.what()), "color: #dc3545;");
        }
    }

    QString getDisplayName() const override
    {
        return "显示布尔值";
    }

    QString getDescription() const override
    {
        return "显示布尔值结果，用于条件判断和逻辑运算的可视化";
    }

private:
    QWidget* m_widget;
    QFrame* m_frame;
    QLabel* m_resultLabel;
    QLabel* m_descriptionLabel;
};
