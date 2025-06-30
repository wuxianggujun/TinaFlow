//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/SheetData.hpp"
#include "data/RangeData.hpp"
#include "PropertyWidget.hpp"

#include <QLineEdit>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QDebug>

/**
 * @brief 读取Excel单元格范围数据的节点模型
 *
 * 这个节点接收一个工作表数据(SheetData)作为输入，
 * 允许用户指定单元格范围（如"A1:C10"），
 * 然后输出该范围的所有数据(RangeData)。
 */
class ReadRangeModel : public BaseNodeModel
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
        QJsonObject modelJson = NodeDelegateModel::save(); // 调用基类方法保存model-name
        modelJson["range"] = m_rangeEdit->text();
        return modelJson;
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

protected:
    // 实现BaseNodeModel的虚函数
    QString getNodeTypeName() const override
    {
        return "ReadRangeModel";
    }

    // 实现IPropertyProvider接口
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle("读取范围设置");
        propertyWidget->addDescription("从Excel工作表中读取指定范围的数据");

        // 添加模式切换按钮
        propertyWidget->addModeToggleButtons();

        // 范围地址设置
        propertyWidget->addTextProperty("范围地址", m_rangeEdit->text(),
            "rangeAddress", "输入范围地址，如A1:C10、B2:E20等",
            [this](const QString& newRange) {
                if (!newRange.isEmpty()) {
                    m_rangeEdit->setText(newRange.toUpper());
                    qDebug() << "ReadRangeModel: Range address changed to" << newRange;
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
        if (m_rangeData && !m_rangeData->isEmpty()) {
            propertyWidget->addSeparator();
            propertyWidget->addTitle("输出数据");

            try {
                int rows = m_rangeData->rowCount();
                int cols = m_rangeData->columnCount();

                propertyWidget->addInfoProperty("读取范围", m_rangeEdit->text(), "color: #2E86AB; font-weight: bold;");
                propertyWidget->addInfoProperty("数据大小", QString("%1行 x %2列").arg(rows).arg(cols), "color: #333; font-weight: bold;");
                propertyWidget->addInfoProperty("总单元格数", QString::number(rows * cols), "color: #666;");

                // 数据预览（显示前几个单元格的值）
                if (rows > 0 && cols > 0) {
                    propertyWidget->addSeparator();
                    propertyWidget->addTitle("数据预览");

                    int previewRows = qMin(3, rows);
                    int previewCols = qMin(3, cols);

                    for (int r = 0; r < previewRows; ++r) {
                        QStringList rowValues;
                        for (int c = 0; c < previewCols; ++c) {
                            QVariant value = m_rangeData->cellValue(r, c);
                            QString valueStr = value.toString();
                            if (valueStr.length() > 10) {
                                valueStr = valueStr.left(10) + "...";
                            }
                            rowValues << valueStr;
                        }
                        if (cols > previewCols) {
                            rowValues << "...";
                        }

                        QString rowText = QString("第%1行: %2").arg(r + 1).arg(rowValues.join(" | "));
                        propertyWidget->addInfoProperty("", rowText, "color: #666; font-family: monospace; font-size: 10px;");
                    }

                    if (rows > previewRows) {
                        propertyWidget->addInfoProperty("", "...", "color: #999; text-align: center;");
                    }
                }

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
        return "读取范围";
    }

    QString getDescription() const override
    {
        return "从Excel工作表中读取指定范围的数据";
    }

private:
    QWidget* m_widget;
    QLineEdit* m_rangeEdit;

    std::shared_ptr<SheetData> m_sheetData;
    std::shared_ptr<RangeData> m_rangeData;
};
