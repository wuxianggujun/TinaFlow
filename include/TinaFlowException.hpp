//
// Created by TinaFlow Team
//

#pragma once

#include <exception>
#include <QString>
#include <QStringList>
#include <QDebug>

/**
 * @brief TinaFlow异常基类
 * 
 * 提供统一的异常处理机制，包括：
 * - 错误类型分类
 * - 详细错误信息
 * - 用户友好的错误描述
 * - 错误恢复建议
 */
class TinaFlowException : public std::exception
{
public:
    enum ErrorType {
        // 文件相关错误
        FileNotFound,           // 文件不存在
        FileAccessDenied,       // 文件访问被拒绝
        FileCorrupted,          // 文件损坏
        FileFormatUnsupported,  // 文件格式不支持
        
        // Excel相关错误
        ExcelFileInvalid,       // Excel文件无效
        WorksheetNotFound,      // 工作表不存在
        CellAddressInvalid,     // 单元格地址无效
        RangeInvalid,           // 范围无效
        
        // 数据相关错误
        DataTypeIncompatible,   // 数据类型不兼容
        DataEmpty,              // 数据为空
        DataOutOfRange,         // 数据超出范围
        DataValidationFailed,   // 数据验证失败
        
        // 网络相关错误
        NetworkTimeout,         // 网络超时
        NetworkConnectionFailed,// 网络连接失败
        
        // 系统相关错误
        InsufficientMemory,     // 内存不足
        PermissionDenied,       // 权限不足
        SystemResourceBusy,     // 系统资源忙
        
        // 用户操作错误
        InvalidUserInput,       // 用户输入无效
        OperationCancelled,     // 操作被取消
        
        // 内部错误
        InternalError,          // 内部错误
        NotImplemented,         // 功能未实现
        Unknown                 // 未知错误
    };

    enum Severity {
        Info,       // 信息
        Warning,    // 警告
        Error,      // 错误
        Critical    // 严重错误
    };

public:
    TinaFlowException(ErrorType type, const QString& message, 
                     const QString& details = "", Severity severity = Error)
        : m_type(type), m_message(message), m_details(details), m_severity(severity)
    {
        m_whatMessage = QString("[%1] %2").arg(errorTypeToString(type)).arg(message).toStdString();
    }

    // 标准异常接口
    const char* what() const noexcept override
    {
        return m_whatMessage.c_str();
    }

    // 获取错误信息
    ErrorType type() const { return m_type; }
    QString message() const { return m_message; }
    QString details() const { return m_details; }
    Severity severity() const { return m_severity; }

    // 获取用户友好的错误描述
    QString userFriendlyMessage() const
    {
        return getUserFriendlyMessage(m_type, m_message);
    }

    // 获取错误恢复建议
    QStringList recoverySuggestions() const
    {
        return getRecoverySuggestions(m_type);
    }

    // 静态工厂方法
    static TinaFlowException fileNotFound(const QString& filePath)
    {
        return TinaFlowException(FileNotFound, 
            QString("文件未找到: %1").arg(filePath),
            QString("请检查文件路径是否正确，文件是否存在"));
    }

    static TinaFlowException invalidCellAddress(const QString& address)
    {
        return TinaFlowException(CellAddressInvalid,
            QString("无效的单元格地址: %1").arg(address),
            QString("单元格地址格式应为字母+数字，如A1、B5、C10等"));
    }

    static TinaFlowException invalidRange(const QString& range)
    {
        return TinaFlowException(RangeInvalid,
            QString("无效的范围: %1").arg(range),
            QString("范围格式应为起始单元格:结束单元格，如A1:C10"));
    }

    static TinaFlowException dataTypeIncompatible(const QString& expected, const QString& actual)
    {
        return TinaFlowException(DataTypeIncompatible,
            QString("数据类型不兼容，期望: %1，实际: %2").arg(expected).arg(actual),
            QString("请检查节点连接是否正确，确保数据类型匹配"));
    }

private:
    ErrorType m_type;
    QString m_message;
    QString m_details;
    Severity m_severity;
    std::string m_whatMessage;

    static QString errorTypeToString(ErrorType type)
    {
        switch (type) {
            case FileNotFound: return "文件未找到";
            case FileAccessDenied: return "文件访问被拒绝";
            case FileCorrupted: return "文件损坏";
            case FileFormatUnsupported: return "文件格式不支持";
            case ExcelFileInvalid: return "Excel文件无效";
            case WorksheetNotFound: return "工作表不存在";
            case CellAddressInvalid: return "单元格地址无效";
            case RangeInvalid: return "范围无效";
            case DataTypeIncompatible: return "数据类型不兼容";
            case DataEmpty: return "数据为空";
            case DataOutOfRange: return "数据超出范围";
            case DataValidationFailed: return "数据验证失败";
            case NetworkTimeout: return "网络超时";
            case NetworkConnectionFailed: return "网络连接失败";
            case InsufficientMemory: return "内存不足";
            case PermissionDenied: return "权限不足";
            case SystemResourceBusy: return "系统资源忙";
            case InvalidUserInput: return "用户输入无效";
            case OperationCancelled: return "操作被取消";
            case InternalError: return "内部错误";
            case NotImplemented: return "功能未实现";
            default: return "未知错误";
        }
    }

    static QString getUserFriendlyMessage(ErrorType type, const QString& message)
    {
        switch (type) {
            case FileNotFound:
                return "找不到指定的文件，请检查文件路径是否正确。";
            case FileAccessDenied:
                return "无法访问文件，请检查文件权限或确保文件未被其他程序占用。";
            case ExcelFileInvalid:
                return "Excel文件格式有误或文件已损坏，请尝试使用其他Excel文件。";
            case CellAddressInvalid:
                return "单元格地址格式不正确，请使用正确的格式（如A1、B5）。";
            case RangeInvalid:
                return "数据范围格式不正确，请使用正确的格式（如A1:C10）。";
            case DataTypeIncompatible:
                return "数据类型不匹配，请检查节点之间的连接是否正确。";
            default:
                return message;
        }
    }

    static QStringList getRecoverySuggestions(ErrorType type)
    {
        switch (type) {
            case FileNotFound:
                return {"检查文件路径是否正确", "确认文件是否存在", "尝试重新选择文件"};
            case FileAccessDenied:
                return {"检查文件权限", "关闭可能占用文件的其他程序", "以管理员身份运行程序"};
            case ExcelFileInvalid:
                return {"尝试用Excel打开文件检查", "使用其他Excel文件", "重新创建Excel文件"};
            case CellAddressInvalid:
                return {"使用正确的单元格地址格式（如A1）", "检查地址中是否包含特殊字符", "确认列号不超过Excel限制"};
            case RangeInvalid:
                return {"使用正确的范围格式（如A1:C10）", "确认起始单元格在结束单元格之前", "检查范围是否在工作表范围内"};
            case DataTypeIncompatible:
                return {"检查节点连接", "确认数据类型匹配", "添加数据转换节点"};
            default:
                return {"重试操作", "检查输入参数", "联系技术支持"};
        }
    }
};

/**
 * @brief 错误处理宏定义
 */
#define TINAFLOW_THROW(type, message) \
    throw TinaFlowException(TinaFlowException::type, message, "", TinaFlowException::Error)

#define TINAFLOW_THROW_WITH_DETAILS(type, message, details) \
    throw TinaFlowException(TinaFlowException::type, message, details, TinaFlowException::Error)

#define TINAFLOW_ASSERT(condition, type, message) \
    if (!(condition)) { \
        TINAFLOW_THROW(type, message); \
    }
