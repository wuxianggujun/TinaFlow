//
// Created by TinaFlow Team
//

#pragma once

#include "TinaFlowException.hpp"
#include <QObject>
#include <QMessageBox>
#include <QWidget>
#include <QTimer>
#include <QQueue>
#include <QMutex>
#include <QDateTime>
#include <functional>

/**
 * @brief 错误信息结构
 */
struct ErrorInfo {
    TinaFlowException::ErrorType type;
    TinaFlowException::Severity severity;
    QString message;
    QString details;
    QStringList suggestions;
    QDateTime timestamp;
    QString nodeId;  // 发生错误的节点ID
    QString context; // 错误上下文
};

/**
 * @brief 错误处理管理器
 * 
 * 提供统一的错误处理机制：
 * - 错误收集和记录
 * - 用户友好的错误显示
 * - 错误恢复建议
 * - 错误统计和分析
 */
class ErrorHandler : public QObject
{
    Q_OBJECT

public:
    static ErrorHandler& instance()
    {
        static ErrorHandler instance;
        return instance;
    }

    /**
     * @brief 处理异常
     * @param exception TinaFlow异常
     * @param parent 父窗口（用于显示对话框）
     * @param nodeId 发生错误的节点ID
     * @param context 错误上下文
     */
    void handleException(const TinaFlowException& exception, 
                        QWidget* parent = nullptr,
                        const QString& nodeId = "",
                        const QString& context = "");

    /**
     * @brief 处理标准异常
     */
    void handleStandardException(const std::exception& exception,
                               QWidget* parent = nullptr,
                               const QString& nodeId = "",
                               const QString& context = "");

    /**
     * @brief 显示错误对话框
     */
    void showErrorDialog(const ErrorInfo& error, QWidget* parent = nullptr);

    /**
     * @brief 显示警告对话框
     */
    void showWarningDialog(const QString& message, const QString& details = "", QWidget* parent = nullptr);

    /**
     * @brief 显示信息对话框
     */
    void showInfoDialog(const QString& message, const QString& details = "", QWidget* parent = nullptr);

    /**
     * @brief 记录错误（不显示对话框）
     */
    void logError(const TinaFlowException& exception, 
                  const QString& nodeId = "",
                  const QString& context = "");

    /**
     * @brief 获取错误历史
     */
    QList<ErrorInfo> getErrorHistory() const;

    /**
     * @brief 清除错误历史
     */
    void clearErrorHistory();

    /**
     * @brief 设置错误回调函数
     */
    void setErrorCallback(std::function<void(const ErrorInfo&)> callback);

    /**
     * @brief 启用/禁用自动错误对话框
     */
    void setAutoShowDialog(bool enabled) { m_autoShowDialog = enabled; }

    /**
     * @brief 获取错误统计
     */
    QMap<TinaFlowException::ErrorType, int> getErrorStatistics() const;

signals:
    void errorOccurred(const ErrorInfo& error);
    void warningOccurred(const QString& message);

private:
    explicit ErrorHandler(QObject* parent = nullptr);
    ~ErrorHandler() = default;

    // 禁用拷贝
    ErrorHandler(const ErrorHandler&) = delete;
    ErrorHandler& operator=(const ErrorHandler&) = delete;

    void addErrorToHistory(const ErrorInfo& error);
    QString formatErrorMessage(const ErrorInfo& error);
    QMessageBox::Icon getSeverityIcon(TinaFlowException::Severity severity);

private:
    mutable QMutex m_mutex;
    QQueue<ErrorInfo> m_errorHistory;
    std::function<void(const ErrorInfo&)> m_errorCallback;
    bool m_autoShowDialog = true;
    static const int MAX_ERROR_HISTORY = 100;
};

/**
 * @brief 错误处理宏定义
 */
#define HANDLE_EXCEPTION(exception, parent, nodeId, context) \
    ErrorHandler::instance().handleException(exception, parent, nodeId, context)

#define HANDLE_STD_EXCEPTION(exception, parent, nodeId, context) \
    ErrorHandler::instance().handleStandardException(exception, parent, nodeId, context)

#define LOG_ERROR(exception, nodeId, context) \
    ErrorHandler::instance().logError(exception, nodeId, context)

#define SHOW_WARNING(message, details, parent) \
    ErrorHandler::instance().showWarningDialog(message, details, parent)

#define SHOW_INFO(message, details, parent) \
    ErrorHandler::instance().showInfoDialog(message, details, parent)

/**
 * @brief 安全执行宏 - 自动捕获和处理异常
 */
#define SAFE_EXECUTE(code, parent, nodeId, context) \
    try { \
        code; \
    } catch (const TinaFlowException& e) { \
        HANDLE_EXCEPTION(e, parent, nodeId, context); \
    } catch (const std::exception& e) { \
        HANDLE_STD_EXCEPTION(e, parent, nodeId, context); \
    } catch (...) { \
        ErrorHandler::instance().handleStandardException( \
            std::runtime_error("未知错误"), parent, nodeId, context); \
    }

/**
 * @brief 安全执行宏 - 只记录错误，不显示对话框
 */
#define SAFE_EXECUTE_SILENT(code, nodeId, context) \
    try { \
        code; \
    } catch (const TinaFlowException& e) { \
        LOG_ERROR(e, nodeId, context); \
    } catch (const std::exception& e) { \
        ErrorHandler::instance().logError( \
            TinaFlowException(TinaFlowException::InternalError, \
                QString("标准异常: %1").arg(e.what())), nodeId, context); \
    } catch (...) { \
        ErrorHandler::instance().logError( \
            TinaFlowException(TinaFlowException::Unknown, "未知错误"), nodeId, context); \
    }
