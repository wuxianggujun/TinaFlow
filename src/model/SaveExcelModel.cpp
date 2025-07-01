//
// Created by wuxianggujun on 25-6-30.
//

#include "model/SaveExcelModel.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>

void SaveExcelModel::saveDataToExcel(const QString& filePath, const QString& sheetName)
{
    qDebug() << "SaveExcelModel: Starting to save data to" << filePath << "sheet:" << sheetName;
    
    // 显示进度条
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, m_rangeData->rowCount());
    m_progressBar->setValue(0);
    m_saveButton->setText("保存中...");
    m_saveButton->setStyleSheet("QPushButton { background-color: #cce5ff; color: #004085; }");
    m_statusLabel->setText("正在保存...");
    
    QApplication::processEvents(); // 更新UI
    
    try {
        // 确保目录存在
        QFileInfo fileInfo(filePath);
        QDir dir = fileInfo.absoluteDir();
        if (!dir.exists()) {
            if (!dir.mkpath(".")) {
                throw std::runtime_error("无法创建目录: " + dir.absolutePath().toStdString());
            }
        }
        
        // 创建或打开Excel文档
        OpenXLSX::XLDocument doc;
        bool fileExists = QFileInfo::exists(filePath);
        
        if (fileExists) {
            qDebug() << "SaveExcelModel: Opening existing file";
            doc.open(filePath.toStdString());
        } else {
            qDebug() << "SaveExcelModel: Creating new file";
            doc.create(filePath.toStdString());
        }
        
        // 获取或创建工作表
        OpenXLSX::XLWorksheet worksheet;
        std::string sheetNameStd = sheetName.toStdString();
        
        if (doc.workbook().worksheetExists(sheetNameStd)) {
            qDebug() << "SaveExcelModel: Using existing worksheet:" << sheetName;
            worksheet = doc.workbook().worksheet(sheetNameStd);
            // 清空现有数据（可选）
            // worksheet.clear();
        } else {
            qDebug() << "SaveExcelModel: Creating new worksheet:" << sheetName;
            if (fileExists && doc.workbook().worksheetCount() > 0) {
                // 如果文件已存在且有工作表，添加新的工作表
                doc.workbook().addWorksheet(sheetNameStd);
                worksheet = doc.workbook().worksheet(sheetNameStd);
            } else {
                // 新文件或没有工作表，重命名默认工作表
                if (doc.workbook().worksheetCount() > 0) {
                    worksheet = doc.workbook().worksheet(1);
                    worksheet.setName(sheetNameStd);
                } else {
                    doc.workbook().addWorksheet(sheetNameStd);
                    worksheet = doc.workbook().worksheet(sheetNameStd);
                }
            }
        }
        
        // 写入数据
        int rows = m_rangeData->rowCount();
        int cols = m_rangeData->columnCount();
        
        qDebug() << "SaveExcelModel: Writing" << rows << "x" << cols << "data";
        
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < cols; ++col) {
                QVariant cellValue = m_rangeData->cellValue(row, col);
                
                // 计算Excel单元格地址 (1-based)
                auto cell = worksheet.cell(row + 1, col + 1);
                
                // 根据数据类型写入值
                if (cellValue.isNull() || !cellValue.isValid()) {
                    // 空值
                    cell.value() = "";
                } else if (cellValue.type() == QVariant::Bool) {
                    cell.value() = cellValue.toBool();
                } else if (cellValue.type() == QVariant::Int || 
                          cellValue.type() == QVariant::LongLong) {
                    cell.value() = cellValue.toLongLong();
                } else if (cellValue.type() == QVariant::Double) {
                    cell.value() = cellValue.toDouble();
                } else {
                    // 字符串或其他类型
                    cell.value() = cellValue.toString().toStdString();
                }
            }
            
            // 更新进度
            m_progressBar->setValue(row + 1);
            if (row % 10 == 0) { // 每10行更新一次UI
                QApplication::processEvents();
            }
        }
        
        // 保存文档
        m_statusLabel->setText("正在保存文件...");
        QApplication::processEvents();
        
        doc.save();
        doc.close();
        
        // 成功完成
        m_progressBar->setVisible(false);
        m_saveButton->setText("保存成功");
        m_saveButton->setStyleSheet("QPushButton { background-color: #d4edda; color: #155724; }");
        m_statusLabel->setText(QString("成功保存 %1行x%2列 数据到 %3")
            .arg(rows)
            .arg(cols)
            .arg(sheetName));
        
        // 更新输出数据
        m_saveResult = std::make_shared<BooleanData>(true, 
            QString("成功保存到 %1").arg(filePath));
        emit dataUpdated(0);
        
        qDebug() << "SaveExcelModel: Successfully saved data to" << filePath;
        
        // 不显示成功弹窗，通过状态标签和输出数据反馈结果
        
    } catch (const std::exception& e) {
        // 错误处理
        QString errorMsg = QString("保存失败: %1").arg(e.what());
        qDebug() << "SaveExcelModel: Error:" << errorMsg;
        
        m_progressBar->setVisible(false);
        m_saveButton->setText("保存失败");
        m_saveButton->setStyleSheet("QPushButton { background-color: #f8d7da; color: #721c24; }");
        m_statusLabel->setText(errorMsg);
        
        // 更新输出数据
        m_saveResult = std::make_shared<BooleanData>(false, errorMsg);
        emit dataUpdated(0);
        
        // 显示错误消息
        QMessageBox::critical(nullptr, "错误", errorMsg);
    }
}
