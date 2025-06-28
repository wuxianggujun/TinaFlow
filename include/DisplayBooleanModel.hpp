//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "data/BooleanData.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QFrame>
#include <QtNodes/NodeDelegateModel>
#include <QDebug>

/**
 * @brief 显示布尔值的节点模型
 * 
 * 这个节点接收一个布尔数据(BooleanData)作为输入，
 * 然后在UI中显示布尔值和相关描述信息。
 * 使用不同的颜色和图标来区分True和False。
 */
class DisplayBooleanModel : public QtNodes::NodeDelegateModel
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

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return (portType == QtNodes::PortType::In) ? 1 : 0; // 只有输入端口，没有输出
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In)
        {
            return BooleanData().type();
        }
        return {"", ""};
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return nullptr; // 显示节点没有输出
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "DisplayBooleanModel::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << "DisplayBooleanModel: Received null nodeData";
            m_booleanData.reset();
            updateDisplay();
            return;
        }
        
        m_booleanData = std::dynamic_pointer_cast<BooleanData>(nodeData);
        if (m_booleanData) {
            qDebug() << "DisplayBooleanModel: Successfully received BooleanData:" 
                     << m_booleanData->valueAsString();
        } else {
            qDebug() << "DisplayBooleanModel: Failed to cast to BooleanData";
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
        qDebug() << "DisplayBooleanModel::updateDisplay called";
        
        if (!m_booleanData) {
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
            bool value = m_booleanData->value();
            QString description = m_booleanData->description();
            
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
                m_descriptionLabel->setText(QString("结果: %1").arg(m_booleanData->localizedString()));
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

    QWidget* m_widget;
    QFrame* m_frame;
    QLabel* m_resultLabel;
    QLabel* m_descriptionLabel;
    
    std::shared_ptr<BooleanData> m_booleanData;
};
