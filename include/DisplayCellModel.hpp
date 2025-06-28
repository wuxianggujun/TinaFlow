//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "data/CellData.hpp"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QtNodes/NodeDelegateModel>
#include <QDebug>

/**
 * @brief 显示Excel单元格数据的节点模型
 * 
 * 这个节点接收一个单元格数据(CellData)作为输入，
 * 然后在UI中显示单元格的地址、值和类型信息。
 * 这是数据流的终端节点，用于查看处理结果。
 */
class DisplayCellModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    DisplayCellModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        m_widget->setMinimumSize(200, 80);
        
        auto* mainLayout = new QVBoxLayout(m_widget);
        mainLayout->setContentsMargins(8, 8, 8, 8);
        mainLayout->setSpacing(4);

        // 创建显示框架
        auto* frame = new QFrame();
        frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
        frame->setLineWidth(1);
        mainLayout->addWidget(frame);

        auto* frameLayout = new QVBoxLayout(frame);
        frameLayout->setContentsMargins(6, 6, 6, 6);
        frameLayout->setSpacing(2);

        // 单元格地址标签
        m_addressLabel = new QLabel("地址: --");
        m_addressLabel->setStyleSheet("font-weight: bold; color: #2E86AB;");
        frameLayout->addWidget(m_addressLabel);

        // 单元格值标签
        m_valueLabel = new QLabel("值: --");
        m_valueLabel->setWordWrap(true);
        m_valueLabel->setStyleSheet("color: #333333;");
        frameLayout->addWidget(m_valueLabel);

        // 数据类型标签
        m_typeLabel = new QLabel("类型: --");
        m_typeLabel->setStyleSheet("font-size: 10px; color: #666666;");
        frameLayout->addWidget(m_typeLabel);

        // 初始显示
        updateDisplay();
    }

    QString caption() const override
    {
        return tr("显示单元格");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("DisplayCell");
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
            return CellData().type();
        }
        return {"", ""};
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return nullptr; // 显示节点没有输出
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "DisplayCellModel::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << "DisplayCellModel: Received null nodeData";
            m_cellData.reset();
            updateDisplay();
            return;
        }
        
        m_cellData = std::dynamic_pointer_cast<CellData>(nodeData);
        if (m_cellData) {
            qDebug() << "DisplayCellModel: Successfully received CellData";
        } else {
            qDebug() << "DisplayCellModel: Failed to cast to CellData";
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
        qDebug() << "DisplayCellModel::updateDisplay called";
        
        if (!m_cellData || !m_cellData->isValid()) {
            // 显示空状态
            m_addressLabel->setText("地址: --");
            m_valueLabel->setText("值: --");
            m_typeLabel->setText("类型: --");
            qDebug() << "DisplayCellModel: No valid cell data to display";
            return;
        }

        try {
            auto cell = m_cellData->cell();
            
            // 获取单元格地址
            QString address = QString::fromStdString(cell->cellReference().address());
            m_addressLabel->setText(QString("地址: %1").arg(address));
            
            // 获取单元格值
            QString value;
            QString type;
            
            if (cell->value().type() == OpenXLSX::XLValueType::Empty) {
                value = "(空)";
                type = "Empty";
            } else if (cell->value().type() == OpenXLSX::XLValueType::Boolean) {
                value = cell->value().get<bool>() ? "TRUE" : "FALSE";
                type = "Boolean";
            } else if (cell->value().type() == OpenXLSX::XLValueType::Integer) {
                value = QString::number(cell->value().get<int64_t>());
                type = "Integer";
            } else if (cell->value().type() == OpenXLSX::XLValueType::Float) {
                value = QString::number(cell->value().get<double>(), 'g', 10);
                type = "Float";
            } else if (cell->value().type() == OpenXLSX::XLValueType::String) {
                value = QString::fromStdString(cell->value().get<std::string>());
                type = "String";
            } else {
                value = "(未知类型)";
                type = "Unknown";
            }
            
            m_valueLabel->setText(QString("值: %1").arg(value));
            m_typeLabel->setText(QString("类型: %1").arg(type));
            
            qDebug() << "DisplayCellModel: Updated display -" 
                     << "Address:" << address 
                     << "Value:" << value 
                     << "Type:" << type;
            
        } catch (const std::exception& e) {
            qDebug() << "DisplayCellModel: Error updating display:" << e.what();
            m_addressLabel->setText("地址: 错误");
            m_valueLabel->setText(QString("错误: %1").arg(e.what()));
            m_typeLabel->setText("类型: Error");
        }
    }

    QWidget* m_widget;
    QLabel* m_addressLabel;
    QLabel* m_valueLabel;
    QLabel* m_typeLabel;
    
    std::shared_ptr<CellData> m_cellData;
};
