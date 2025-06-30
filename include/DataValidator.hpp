//
// Created by TinaFlow Team
//

#pragma once

#include "TinaFlowException.hpp"
#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>

/**
 * @brief 验证结果结构
 */
struct ValidationResult {
    bool isValid = true;
    QString errorMessage;
    QStringList warnings;
    QStringList suggestions;
    
    // 便利构造函数
    static ValidationResult success() {
        return ValidationResult{true, "", {}, {}};
    }
    
    static ValidationResult error(const QString& message, const QStringList& suggestions = {}) {
        return ValidationResult{false, message, {}, suggestions};
    }
    
    static ValidationResult warning(const QString& message, const QStringList& warnings = {}) {
        return ValidationResult{true, "", warnings, {}};
    }
};

/**
 * @brief 数据验证器
 * 
 * 提供各种数据验证功能：
 * - Excel单元格地址验证
 * - 范围地址验证
 * - 文件路径验证
 * - 数据类型验证
 */
class DataValidator
{
public:
    /**
     * @brief 验证Excel单元格地址
     * @param address 单元格地址（如A1、B5、AA100）
     * @return 验证结果
     */
    static ValidationResult validateCellAddress(const QString& address);
    
    /**
     * @brief 验证Excel范围地址
     * @param range 范围地址（如A1:C10、B2:E20）
     * @return 验证结果
     */
    static ValidationResult validateRange(const QString& range);
    
    /**
     * @brief 验证文件路径
     * @param filePath 文件路径
     * @param checkExists 是否检查文件是否存在
     * @param allowedExtensions 允许的文件扩展名
     * @return 验证结果
     */
    static ValidationResult validateFilePath(const QString& filePath, 
                                           bool checkExists = true,
                                           const QStringList& allowedExtensions = {});
    
    /**
     * @brief 验证Excel文件
     * @param filePath Excel文件路径
     * @return 验证结果
     */
    static ValidationResult validateExcelFile(const QString& filePath);
    
    /**
     * @brief 验证工作表名称
     * @param sheetName 工作表名称
     * @return 验证结果
     */
    static ValidationResult validateSheetName(const QString& sheetName);
    
    /**
     * @brief 验证数值
     * @param value 数值字符串
     * @param allowNegative 是否允许负数
     * @param allowDecimal 是否允许小数
     * @return 验证结果
     */
    static ValidationResult validateNumber(const QString& value, 
                                         bool allowNegative = true,
                                         bool allowDecimal = true);
    
    /**
     * @brief 验证整数
     * @param value 整数字符串
     * @param min 最小值
     * @param max 最大值
     * @return 验证结果
     */
    static ValidationResult validateInteger(const QString& value, 
                                          int min = INT_MIN, 
                                          int max = INT_MAX);
    
    /**
     * @brief 验证字符串长度
     * @param text 文本
     * @param minLength 最小长度
     * @param maxLength 最大长度
     * @return 验证结果
     */
    static ValidationResult validateStringLength(const QString& text, 
                                                int minLength = 0, 
                                                int maxLength = INT_MAX);
    
    /**
     * @brief 验证正则表达式
     * @param text 要验证的文本
     * @param pattern 正则表达式模式
     * @param errorMessage 错误消息
     * @return 验证结果
     */
    static ValidationResult validateRegex(const QString& text, 
                                        const QString& pattern,
                                        const QString& errorMessage = "格式不正确");

private:
    // Excel相关常量
    static const int MAX_EXCEL_ROWS = 1048576;      // Excel最大行数
    static const int MAX_EXCEL_COLUMNS = 16384;     // Excel最大列数（XFD列）
    static const QStringList EXCEL_EXTENSIONS;      // Excel文件扩展名
    
    // 辅助方法
    static bool isValidColumnReference(const QString& column);
    static int columnToNumber(const QString& column);
    static QString numberToColumn(int number);
    static bool isValidRowNumber(int row);
    static QPair<QString, int> parseCellAddress(const QString& address);
};

/**
 * @brief 验证宏定义
 */
#define VALIDATE_AND_THROW(validation, exceptionType) \
    do { \
        auto result = validation; \
        if (!result.isValid) { \
            TINAFLOW_THROW(exceptionType, result.errorMessage); \
        } \
    } while(0)

#define VALIDATE_CELL_ADDRESS(address) \
    VALIDATE_AND_THROW(DataValidator::validateCellAddress(address), CellAddressInvalid)

#define VALIDATE_RANGE(range) \
    VALIDATE_AND_THROW(DataValidator::validateRange(range), RangeInvalid)

#define VALIDATE_FILE_PATH(path) \
    VALIDATE_AND_THROW(DataValidator::validateFilePath(path), FileNotFound)

#define VALIDATE_EXCEL_FILE(path) \
    VALIDATE_AND_THROW(DataValidator::validateExcelFile(path), ExcelFileInvalid)
