#include "DataValidator.hpp"
#include <QFileInfo>
#include <QDir>
#include <QDebug>

// Excel文件扩展名
const QStringList DataValidator::EXCEL_EXTENSIONS = {".xlsx", ".xls", ".xlsm", ".xlsb"};

ValidationResult DataValidator::validateCellAddress(const QString& address)
{
    if (address.isEmpty()) {
        return ValidationResult::error("单元格地址不能为空", 
            {"请输入有效的单元格地址，如A1、B5等"});
    }
    
    // 使用正则表达式验证单元格地址格式
    QRegularExpression regex("^[A-Z]+[1-9][0-9]*$");
    if (!regex.match(address.toUpper()).hasMatch()) {
        return ValidationResult::error(
            QString("单元格地址格式不正确: %1").arg(address),
            {"单元格地址应由字母和数字组成，如A1、B5、AA100",
             "字母部分表示列，数字部分表示行",
             "行号必须大于0"});
    }
    
    // 解析单元格地址
    auto parsed = parseCellAddress(address.toUpper());
    QString column = parsed.first;
    int row = parsed.second;
    
    // 验证列引用
    if (!isValidColumnReference(column)) {
        return ValidationResult::error(
            QString("列引用超出Excel限制: %1").arg(column),
            {"Excel最大支持XFD列（第16384列）",
             "请使用有效的列引用"});
    }
    
    // 验证行号
    if (!isValidRowNumber(row)) {
        return ValidationResult::error(
            QString("行号超出Excel限制: %1").arg(row),
            {"Excel最大支持1048576行",
             "请使用有效的行号"});
    }
    
    return ValidationResult::success();
}

ValidationResult DataValidator::validateRange(const QString& range)
{
    if (range.isEmpty()) {
        return ValidationResult::error("范围地址不能为空",
            {"请输入有效的范围地址，如A1:C10"});
    }
    
    // 分割范围
    QStringList parts = range.split(":");
    if (parts.size() != 2) {
        return ValidationResult::error(
            QString("范围格式不正确: %1").arg(range),
            {"范围格式应为 起始单元格:结束单元格",
             "例如: A1:C10, B2:E20"});
    }
    
    QString startCell = parts[0].trimmed().toUpper();
    QString endCell = parts[1].trimmed().toUpper();
    
    // 验证起始单元格
    auto startResult = validateCellAddress(startCell);
    if (!startResult.isValid) {
        return ValidationResult::error(
            QString("起始单元格地址无效: %1").arg(startCell),
            startResult.suggestions);
    }
    
    // 验证结束单元格
    auto endResult = validateCellAddress(endCell);
    if (!endResult.isValid) {
        return ValidationResult::error(
            QString("结束单元格地址无效: %1").arg(endCell),
            endResult.suggestions);
    }
    
    // 解析单元格地址
    auto startParsed = parseCellAddress(startCell);
    auto endParsed = parseCellAddress(endCell);
    
    int startCol = columnToNumber(startParsed.first);
    int startRow = startParsed.second;
    int endCol = columnToNumber(endParsed.first);
    int endRow = endParsed.second;
    
    // 验证范围逻辑
    if (startCol > endCol || startRow > endRow) {
        return ValidationResult::error(
            QString("范围逻辑错误: %1").arg(range),
            {"起始单元格应在结束单元格的左上方",
             "请确保起始行列号都小于等于结束行列号"});
    }
    
    return ValidationResult::success();
}

ValidationResult DataValidator::validateFilePath(const QString& filePath, 
                                               bool checkExists,
                                               const QStringList& allowedExtensions)
{
    if (filePath.isEmpty()) {
        return ValidationResult::error("文件路径不能为空",
            {"请选择或输入有效的文件路径"});
    }
    
    QFileInfo fileInfo(filePath);
    
    // 检查文件是否存在
    if (checkExists && !fileInfo.exists()) {
        return ValidationResult::error(
            QString("文件不存在: %1").arg(filePath),
            {"请检查文件路径是否正确",
             "确认文件是否存在",
             "尝试重新选择文件"});
    }
    
    // 检查是否为文件（而不是目录）
    if (checkExists && fileInfo.exists() && !fileInfo.isFile()) {
        return ValidationResult::error(
            QString("路径不是文件: %1").arg(filePath),
            {"请选择文件而不是文件夹"});
    }
    
    // 检查文件扩展名
    if (!allowedExtensions.isEmpty()) {
        QString extension = fileInfo.suffix().toLower();
        bool validExtension = false;
        for (const QString& allowed : allowedExtensions) {
            if (allowed.toLower().endsWith(extension) || 
                allowed.toLower() == "." + extension) {
                validExtension = true;
                break;
            }
        }
        
        if (!validExtension) {
            return ValidationResult::error(
                QString("文件类型不支持: %1").arg(extension),
                {QString("支持的文件类型: %1").arg(allowedExtensions.join(", ")),
                 "请选择正确类型的文件"});
        }
    }
    
    // 检查文件权限
    if (checkExists && fileInfo.exists() && !fileInfo.isReadable()) {
        return ValidationResult::error(
            QString("文件无法读取: %1").arg(filePath),
            {"请检查文件权限",
             "确保文件未被其他程序占用",
             "尝试以管理员身份运行程序"});
    }
    
    return ValidationResult::success();
}

ValidationResult DataValidator::validateExcelFile(const QString& filePath)
{
    return validateFilePath(filePath, true, EXCEL_EXTENSIONS);
}

ValidationResult DataValidator::validateSheetName(const QString& sheetName)
{
    if (sheetName.isEmpty()) {
        return ValidationResult::error("工作表名称不能为空",
            {"请输入有效的工作表名称"});
    }
    
    // Excel工作表名称限制
    if (sheetName.length() > 31) {
        return ValidationResult::error("工作表名称过长",
            {"工作表名称不能超过31个字符"});
    }
    
    // 检查非法字符
    QStringList invalidChars = {"\\", "/", "?", "*", "[", "]", ":"};
    for (const QString& invalidChar : invalidChars) {
        if (sheetName.contains(invalidChar)) {
            return ValidationResult::error(
                QString("工作表名称包含非法字符: %1").arg(invalidChar),
                {"工作表名称不能包含以下字符: \\ / ? * [ ] :"});
        }
    }
    
    return ValidationResult::success();
}

ValidationResult DataValidator::validateNumber(const QString& value, 
                                             bool allowNegative,
                                             bool allowDecimal)
{
    if (value.isEmpty()) {
        return ValidationResult::error("数值不能为空");
    }
    
    bool ok;
    double number = value.toDouble(&ok);
    
    if (!ok) {
        return ValidationResult::error(
            QString("不是有效的数值: %1").arg(value),
            {"请输入有效的数字"});
    }
    
    if (!allowNegative && number < 0) {
        return ValidationResult::error("不允许负数",
            {"请输入非负数"});
    }
    
    if (!allowDecimal && value.contains('.')) {
        return ValidationResult::error("不允许小数",
            {"请输入整数"});
    }
    
    return ValidationResult::success();
}

ValidationResult DataValidator::validateInteger(const QString& value, int min, int max)
{
    if (value.isEmpty()) {
        return ValidationResult::error("整数不能为空");
    }
    
    bool ok;
    int number = value.toInt(&ok);
    
    if (!ok) {
        return ValidationResult::error(
            QString("不是有效的整数: %1").arg(value),
            {"请输入有效的整数"});
    }
    
    if (number < min || number > max) {
        return ValidationResult::error(
            QString("整数超出范围: %1 (范围: %2-%3)").arg(number).arg(min).arg(max),
            {QString("请输入%1到%2之间的整数").arg(min).arg(max)});
    }
    
    return ValidationResult::success();
}

ValidationResult DataValidator::validateStringLength(const QString& text, int minLength, int maxLength)
{
    int length = text.length();
    
    if (length < minLength) {
        return ValidationResult::error(
            QString("文本长度不足: %1 (最少需要%2个字符)").arg(length).arg(minLength),
            {QString("请输入至少%1个字符").arg(minLength)});
    }
    
    if (length > maxLength) {
        return ValidationResult::error(
            QString("文本长度超限: %1 (最多允许%2个字符)").arg(length).arg(maxLength),
            {QString("请输入不超过%1个字符").arg(maxLength)});
    }
    
    return ValidationResult::success();
}

ValidationResult DataValidator::validateRegex(const QString& text, 
                                             const QString& pattern,
                                             const QString& errorMessage)
{
    QRegularExpression regex(pattern);
    if (!regex.isValid()) {
        return ValidationResult::error("正则表达式无效",
            {"请检查正则表达式语法"});
    }
    
    if (!regex.match(text).hasMatch()) {
        return ValidationResult::error(errorMessage,
            {"请检查输入格式是否正确"});
    }
    
    return ValidationResult::success();
}

// 私有辅助方法实现
bool DataValidator::isValidColumnReference(const QString& column)
{
    int colNumber = columnToNumber(column);
    return colNumber > 0 && colNumber <= MAX_EXCEL_COLUMNS;
}

int DataValidator::columnToNumber(const QString& column)
{
    int result = 0;
    for (int i = 0; i < column.length(); ++i) {
        result = result * 26 + (column[i].toLatin1() - 'A' + 1);
    }
    return result;
}

QString DataValidator::numberToColumn(int number)
{
    QString result;
    while (number > 0) {
        number--; // 调整为0基索引
        result = QChar('A' + number % 26) + result;
        number /= 26;
    }
    return result;
}

bool DataValidator::isValidRowNumber(int row)
{
    return row > 0 && row <= MAX_EXCEL_ROWS;
}

QPair<QString, int> DataValidator::parseCellAddress(const QString& address)
{
    QRegularExpression regex("^([A-Z]+)([1-9][0-9]*)$");
    QRegularExpressionMatch match = regex.match(address);
    
    if (match.hasMatch()) {
        QString column = match.captured(1);
        int row = match.captured(2).toInt();
        return qMakePair(column, row);
    }
    
    return qMakePair(QString(), 0);
}
