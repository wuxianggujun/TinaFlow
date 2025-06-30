//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/SheetData.hpp"
#include "data/CellData.hpp"
#include "PropertyWidget.hpp"
#include "ErrorHandler.hpp"
#include "DataValidator.hpp"

#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QDebug>

/**
 * @brief 读取Excel单元格数据的节点模型
 *
 * 这个节点接收一个工作表数据(SheetData)作为输入，
 * 允许用户指定单元格地址（如"A1", "B5"），
 * 然后输出该单元格的数据(CellData)。
 */
class ReadCellModel : public BaseNodeModel
{
    Q_OBJECT

public:
    ReadCellModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        auto* layout = new QHBoxLayout(m_widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(6);

        // 添加标签
        auto* label = new QLabel("单元格:");
        layout->addWidget(label);

        // 添加单元格地址输入框
        m_cellAddressEdit = new QLineEdit();
        m_cellAddressEdit->setPlaceholderText("A1");
        m_cellAddressEdit->setMaximumWidth(60);
        m_cellAddressEdit->setText("A1"); // 默认值
        layout->addWidget(m_cellAddressEdit);

        // 连接信号
        connect(m_cellAddressEdit, &QLineEdit::textChanged,
                this, &ReadCellModel::onCellAddressChanged);
    }

    QString caption() const override
    {
        return tr("读取单元格");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("ReadCell");
    }

    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return (portType == QtNodes::PortType::In) ? 1 : (portType == QtNodes::PortType::Out) ? 1 : 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In)
        {
            return SheetData().type();
        }
        return CellData().type();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return m_cellData;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "ReadCellModel::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << "ReadCellModel: Received null nodeData";
            m_sheetData.reset();
            updateCellData();
            return;
        }
        
        m_sheetData = std::dynamic_pointer_cast<SheetData>(nodeData);
        if (m_sheetData) {
            qDebug() << "ReadCellModel: Successfully received SheetData for sheet:" 
                     << QString::fromStdString(m_sheetData->sheetName());
        } else {
            qDebug() << "ReadCellModel: Failed to cast to SheetData";
        }
        
        updateCellData();
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save(); // 调用基类方法保存model-name
        modelJson["cellAddress"] = m_cellAddressEdit->text();
        return modelJson;
    }

    void load(QJsonObject const& json) override
    {
        if (json.contains("cellAddress")) {
            m_cellAddressEdit->setText(json["cellAddress"].toString());
        }
    }

private slots:
    void onCellAddressChanged()
    {
        qDebug() << "ReadCellModel: Cell address changed to:" << m_cellAddressEdit->text();
        updateCellData();
    }

private:
    void updateCellData()
    {
        qDebug() << "ReadCellModel::updateCellData called";

        if (!m_sheetData) {
            qDebug() << "ReadCellModel: No sheet data available";
            m_cellData.reset();
            emit dataUpdated(0);
            return;
        }

        const QString cellAddress = m_cellAddressEdit->text().trimmed().toUpper();
        if (cellAddress.isEmpty()) {
            qDebug() << "ReadCellModel: Empty cell address";
            m_cellData.reset();
            emit dataUpdated(0);
            return;
        }

        SAFE_EXECUTE({
            // 验证单元格地址
            auto validation = DataValidator::validateCellAddress(cellAddress);
            if (!validation.isValid) {
                throw TinaFlowException::invalidCellAddress(cellAddress);
            }

            // 使用OpenXLSX读取单元格数据
            auto& worksheet = m_sheetData->worksheet();
            auto cell = worksheet.cell(cellAddress.toStdString());

            qDebug() << "ReadCellModel: Reading cell" << cellAddress;

            // 创建CellData
            m_cellData = std::make_shared<CellData>(cell);

            qDebug() << "ReadCellModel: Successfully read cell data";
            emit dataUpdated(0);

        }, m_widget, "ReadCellModel", QString("读取单元格 %1").arg(cellAddress));

        // 如果发生错误，清除数据
        if (!m_cellData) {
            emit dataUpdated(0);
        }
    }

protected:
    // 实现BaseNodeModel的虚函数
    QString getNodeTypeName() const override
    {
        return "ReadCellModel";
    }

    // 实现IPropertyProvider接口
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle("读取单元格设置");
        propertyWidget->addDescription("从Excel工作表中读取指定单元格的数据");

        // 添加模式切换按钮
        propertyWidget->addModeToggleButtons();

        // 单元格地址设置
        propertyWidget->addTextProperty("单元格地址", m_cellAddressEdit->text(),
            "cellAddress", "输入单元格地址，如A1、B5等",
            [this](const QString& newAddress) {
                if (!newAddress.isEmpty()) {
                    m_cellAddressEdit->setText(newAddress.toUpper());
                    qDebug() << "ReadCellModel: Cell address changed to" << newAddress;
                }
            });

        // 工作表连接状态
        propertyWidget->addSeparator();
        propertyWidget->addTitle("连接状态");

        if (m_sheetData) {
            propertyWidget->addInfoProperty("工作表状态", "已连接", "color: #28a745; font-weight: bold;");
            propertyWidget->addInfoProperty("工作表名称", QString::fromStdString(m_sheetData->sheetName()), "color: #666;");
        } else {
            propertyWidget->addInfoProperty("工作表状态", "未连接", "color: #999; font-style: italic;");
        }

        // 输出数据状态
        if (m_cellData && m_cellData->isValid()) {
            propertyWidget->addSeparator();
            propertyWidget->addTitle("输出数据");

            try {
                QString address = m_cellData->address();
                QVariant value = m_cellData->value();
                QString type = m_cellData->type().name;

                propertyWidget->addInfoProperty("读取地址", address, "color: #2E86AB; font-weight: bold;");
                propertyWidget->addInfoProperty("单元格值", value.toString(), "color: #333; font-weight: bold;");
                propertyWidget->addInfoProperty("数据类型", type, "color: #666;");

            } catch (const std::exception& e) {
                propertyWidget->addInfoProperty("数据状态", QString("读取失败: %1").arg(e.what()), "color: #dc3545;");
            }
        } else {
            propertyWidget->addSeparator();
            propertyWidget->addInfoProperty("输出数据", "无数据", "color: #999; font-style: italic;");
        }

        return true;
    }

    QString getDisplayName() const override
    {
        return "读取单元格";
    }

    QString getDescription() const override
    {
        return "从Excel工作表中读取指定单元格的数据";
    }

private:
    QWidget* m_widget;
    QLineEdit* m_cellAddressEdit;

    std::shared_ptr<SheetData> m_sheetData;
    std::shared_ptr<CellData> m_cellData;
};
