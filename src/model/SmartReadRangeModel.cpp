//
// Created by wuxianggujun on 25-7-3.
//

#include "model/SmartReadRangeModel.hpp"
#include <QDebug>

#include "ErrorHandler.hpp"

SmartReadRangeModel::SmartReadRangeModel()
{
    qDebug() << "SmartReadRangeModel created";
}

void SmartReadRangeModel::processData()
{
    if (!m_sheetData) {
        m_rangeData.reset();
        emit dataUpdated(0);
        return;
    }

    QString rangeAddress;

    // 根据读取模式确定范围
    switch (m_readMode) {
        case ReadMode::ManualRange:
            rangeAddress = m_manualRangeEdit ? m_manualRangeEdit->text().trimmed() : "A1:C10";
            break;

        case ReadMode::EntireSheet:
        case ReadMode::UsedRange: {
            // 使用 OpenXLSX 的 API 获取实际数据范围
            auto& worksheet = m_sheetData->worksheet();
            auto lastCell = worksheet.lastCell();
            if (lastCell.address() != "A1" || !worksheet.cell("A1").value().get<std::string>().empty()) {
                rangeAddress = "A1:" + QString::fromStdString(lastCell.address());
            } else {
                rangeAddress = "A1:A1"; // 空工作表
            }
            break;
        }

        case ReadMode::FromCell: {
            QString startCell = m_startCellEdit ? m_startCellEdit->text().trimmed() : "A1";
            if (startCell.isEmpty()) startCell = "A1";
            auto& worksheet = m_sheetData->worksheet();
            auto lastCell = worksheet.lastCell();
            rangeAddress = startCell + ":" + QString::fromStdString(lastCell.address());
            break;
        }

        case ReadMode::SpecificRows: {
            QString startRowStr = m_rowStartEdit ? m_rowStartEdit->text().trimmed() : "1";
            QString endRowStr = m_rowEndEdit ? m_rowEndEdit->text().trimmed() : "";
            uint32_t startRow = startRowStr.toUInt();
            if (startRow == 0) startRow = 1;

            uint32_t endRow;
            if (endRowStr.isEmpty()) {
                auto& worksheet = m_sheetData->worksheet();
                endRow = worksheet.rowCount();
            } else {
                endRow = endRowStr.toUInt();
                if (endRow == 0) endRow = startRow;
            }

            auto& worksheet = m_sheetData->worksheet();
            uint16_t lastCol = worksheet.columnCount();
            QString lastColLetter = QString::fromStdString(OpenXLSX::XLCellReference::columnAsString(lastCol));
            rangeAddress = QString("A%1:%2%3").arg(startRow).arg(lastColLetter).arg(endRow);
            break;
        }

        case ReadMode::SpecificColumns: {
            QString startCol = m_colStartEdit ? m_colStartEdit->text().trimmed().toUpper() : "A";
            QString endCol = m_colEndEdit ? m_colEndEdit->text().trimmed().toUpper() : "";
            if (startCol.isEmpty()) startCol = "A";

            if (endCol.isEmpty()) {
                auto& worksheet = m_sheetData->worksheet();
                uint16_t lastColNum = worksheet.columnCount();
                endCol = QString::fromStdString(OpenXLSX::XLCellReference::columnAsString(lastColNum));
            }

            auto& worksheet = m_sheetData->worksheet();
            uint32_t lastRow = worksheet.rowCount();
            rangeAddress = QString("%11:%2%3").arg(startCol).arg(endCol).arg(lastRow);
            break;
        }
    }

    if (rangeAddress.isEmpty()) {
        qWarning() << "SmartReadRangeModel: Empty range address";
        m_rangeData.reset();
        emit dataUpdated(0);
        return;
    }

    qDebug() << "SmartReadRangeModel: Reading range:" << rangeAddress;

    // 使用 SAFE_EXECUTE 宏进行安全执行
    SAFE_EXECUTE({
        auto& worksheet = m_sheetData->worksheet();
        auto range = worksheet.range(rangeAddress.toStdString());

        std::vector<std::vector<QVariant>> data;

        // 遍历范围中的所有单元格
        for (auto& cell : range) {
            // 计算当前单元格的行列位置
            uint32_t row = cell.cellReference().row();
            uint16_t col = cell.cellReference().column();

            // 确保数据向量有足够的行
            while (data.size() < static_cast<size_t>(row)) {
                data.push_back(std::vector<QVariant>());
            }

            // 确保当前行有足够的列
            while (data[row - 1].size() < static_cast<size_t>(col)) {
                data[row - 1].push_back(QVariant());
            }

            // 读取单元格值
            QVariant cellValue;
            if (cell.value().type() == OpenXLSX::XLValueType::Empty) {
                cellValue = QString("");
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

            data[row - 1][col - 1] = cellValue;
        }

        // 创建结果
        m_rangeData = std::make_shared<RangeData>(rangeAddress, data);

        qDebug() << "SmartReadRangeModel: Successfully read" << data.size()
                 << "rows x" << (data.empty() ? 0 : data[0].size()) << "cols";

        emit dataUpdated(0);

    }, m_widget, "SmartReadRangeModel", QString("智能读取范围 %1").arg(rangeAddress));
}
