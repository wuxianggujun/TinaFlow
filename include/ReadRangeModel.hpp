//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "data/SheetData.hpp"
#include "data/RangeData.hpp"

#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QtNodes/NodeDelegateModel>
#include <QDebug>

/**
 * @brief 读取Excel单元格范围数据的节点模型
 * 
 * 这个节点接收一个工作表数据(SheetData)作为输入，
 * 允许用户指定单元格范围（如"A1:C10"），
 * 然后输出该范围的所有数据(RangeData)。
 */
class ReadRangeModel : public QtNodes::NodeDelegateModel
{
    Q_OBJECT

public:
    ReadRangeModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        auto* layout = new QHBoxLayout(m_widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(6);

        // 添加标签
        auto* label = new QLabel("范围:");
        layout->addWidget(label);

        // 添加范围地址输入框
        m_rangeEdit = new QLineEdit();
        m_rangeEdit->setPlaceholderText("A1:C10");
        m_rangeEdit->setMinimumWidth(80);
        m_rangeEdit->setText("A1:C10"); // 默认值
        layout->addWidget(m_rangeEdit);

        // 连接信号
        connect(m_rangeEdit, &QLineEdit::textChanged,
                this, &ReadRangeModel::onRangeChanged);
    }

    QString caption() const override
    {
        return tr("读取范围");
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("ReadRange");
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
        return RangeData().type();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return m_rangeData;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "ReadRangeModel::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << "ReadRangeModel: Received null nodeData";
            m_sheetData.reset();
            updateRangeData();
            return;
        }
        
        m_sheetData = std::dynamic_pointer_cast<SheetData>(nodeData);
        if (m_sheetData) {
            qDebug() << "ReadRangeModel: Successfully received SheetData for sheet:" 
                     << QString::fromStdString(m_sheetData->sheetName());
        } else {
            qDebug() << "ReadRangeModel: Failed to cast to SheetData";
        }
        
        updateRangeData();
    }

    QJsonObject save() const override
    {
        return {
            {"range", m_rangeEdit->text()}
        };
    }

    void load(QJsonObject const& json) override
    {
        if (json.contains("range")) {
            m_rangeEdit->setText(json["range"].toString());
        }
    }

private slots:
    void onRangeChanged()
    {
        qDebug() << "ReadRangeModel: Range changed to:" << m_rangeEdit->text();
        updateRangeData();
    }

private:
    void updateRangeData()
    {
        qDebug() << "ReadRangeModel::updateRangeData called";
        
        if (!m_sheetData) {
            qDebug() << "ReadRangeModel: No sheet data available";
            m_rangeData.reset();
            emit dataUpdated(0);
            return;
        }

        const QString rangeAddress = m_rangeEdit->text().trimmed().toUpper();
        if (rangeAddress.isEmpty()) {
            qDebug() << "ReadRangeModel: Empty range address";
            m_rangeData.reset();
            emit dataUpdated(0);
            return;
        }

        try {
            // 使用OpenXLSX读取范围数据
            auto& worksheet = m_sheetData->worksheet();
            auto range = worksheet.range(rangeAddress.toStdString());
            
            qDebug() << "ReadRangeModel: Reading range" << rangeAddress;
            
            // 获取范围的行列数
            int rowCount = range.numRows();
            int colCount = range.numColumns();

            qDebug() << "ReadRangeModel: Range size:" << rowCount << "x" << colCount;

            // 读取数据到二维数组
            std::vector<std::vector<QVariant>> data;
            data.reserve(rowCount);

            // 使用迭代器遍历范围中的所有单元格
            auto topLeft = range.topLeft();
            int startRow = topLeft.row();
            int startCol = topLeft.column();

            for (int row = 0; row < rowCount; ++row) {
                std::vector<QVariant> rowData;
                rowData.reserve(colCount);

                for (int col = 0; col < colCount; ++col) {
                    // 计算实际的单元格引用
                    OpenXLSX::XLCellReference cellRef(startRow + row, startCol + col);
                    auto cell = worksheet.cell(cellRef);

                    QVariant cellValue;
                    if (cell.value().type() == OpenXLSX::XLValueType::Empty) {
                        cellValue = QVariant();
                    } else if (cell.value().type() == OpenXLSX::XLValueType::Boolean) {
                        cellValue = cell.value().get<bool>();
                    } else if (cell.value().type() == OpenXLSX::XLValueType::Integer) {
                        cellValue = static_cast<qint64>(cell.value().get<int64_t>());
                    } else if (cell.value().type() == OpenXLSX::XLValueType::Float) {
                        cellValue = cell.value().get<double>();
                    } else if (cell.value().type() == OpenXLSX::XLValueType::String) {
                        cellValue = QString::fromUtf8(cell.value().get<std::string>().c_str());
                    } else {
                        cellValue = QString("(未知类型)");
                    }

                    rowData.push_back(cellValue);
                }

                data.push_back(rowData);
            }
            
            // 创建RangeData
            m_rangeData = std::make_shared<RangeData>(rangeAddress, data);
            
            qDebug() << "ReadRangeModel: Successfully read range data:" 
                     << rowCount << "rows x" << colCount << "cols";
            emit dataUpdated(0);
            
        } catch (const std::exception& e) {
            qDebug() << "ReadRangeModel: Error reading range:" << e.what();
            m_rangeData.reset();
            emit dataUpdated(0);
        }
    }

    QWidget* m_widget;
    QLineEdit* m_rangeEdit;
    
    std::shared_ptr<SheetData> m_sheetData;
    std::shared_ptr<RangeData> m_rangeData;
};
