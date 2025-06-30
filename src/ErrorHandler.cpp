#include "ErrorHandler.hpp"
#include <QApplication>
#include <QDebug>
#include <QTextStream>
#include <QMutexLocker>

ErrorHandler::ErrorHandler(QObject* parent)
    : QObject(parent)
{
    qDebug() << "ErrorHandler: Initialized";
}

void ErrorHandler::handleException(const TinaFlowException& exception, 
                                 QWidget* parent,
                                 const QString& nodeId,
                                 const QString& context)
{
    QMutexLocker locker(&m_mutex);
    
    // 创建错误信息
    ErrorInfo error;
    error.type = exception.type();
    error.severity = exception.severity();
    error.message = exception.message();
    error.details = exception.details();
    error.suggestions = exception.recoverySuggestions();
    error.timestamp = QDateTime::currentDateTime();
    error.nodeId = nodeId;
    error.context = context;
    
    // 添加到历史记录
    addErrorToHistory(error);
    
    // 发出信号
    emit errorOccurred(error);
    
    // 调用回调函数
    if (m_errorCallback) {
        m_errorCallback(error);
    }
    
    // 记录到调试输出
    qDebug() << "ErrorHandler: Exception occurred -"
             << "Type:" << static_cast<int>(error.type)
             << "Message:" << error.message
             << "NodeId:" << nodeId
             << "Context:" << context;
    
    // 显示错误对话框
    if (m_autoShowDialog && error.severity >= TinaFlowException::Error) {
        showErrorDialog(error, parent);
    }
}

void ErrorHandler::handleStandardException(const std::exception& exception,
                                         QWidget* parent,
                                         const QString& nodeId,
                                         const QString& context)
{
    // 将标准异常转换为TinaFlow异常
    TinaFlowException tinaException(
        TinaFlowException::InternalError,
        QString("标准异常: %1").arg(exception.what()),
        QString("在%1中发生了标准异常").arg(context.isEmpty() ? "未知位置" : context),
        TinaFlowException::Error
    );
    
    handleException(tinaException, parent, nodeId, context);
}

void ErrorHandler::showErrorDialog(const ErrorInfo& error, QWidget* parent)
{
    if (!parent) {
        parent = qApp->activeWindow();
    }
    
    QMessageBox msgBox(parent);
    msgBox.setIcon(getSeverityIcon(error.severity));
    msgBox.setWindowTitle("TinaFlow - 错误");
    msgBox.setText(error.message);
    
    // 构建详细信息
    QString detailText;
    QTextStream stream(&detailText);
    
    if (!error.details.isEmpty()) {
        stream << "详细信息:\n" << error.details << "\n\n";
    }
    
    if (!error.nodeId.isEmpty()) {
        stream << "节点ID: " << error.nodeId << "\n";
    }
    
    if (!error.context.isEmpty()) {
        stream << "上下文: " << error.context << "\n";
    }
    
    stream << "时间: " << error.timestamp.toString("yyyy-MM-dd hh:mm:ss") << "\n\n";
    
    if (!error.suggestions.isEmpty()) {
        stream << "建议的解决方案:\n";
        for (int i = 0; i < error.suggestions.size(); ++i) {
            stream << QString("%1. %2\n").arg(i + 1).arg(error.suggestions[i]);
        }
    }
    
    msgBox.setDetailedText(detailText);
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void ErrorHandler::showWarningDialog(const QString& message, const QString& details, QWidget* parent)
{
    if (!parent) {
        parent = qApp->activeWindow();
    }
    
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("TinaFlow - 警告");
    msgBox.setText(message);
    
    if (!details.isEmpty()) {
        msgBox.setDetailedText(details);
    }
    
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
    
    emit warningOccurred(message);
}

void ErrorHandler::showInfoDialog(const QString& message, const QString& details, QWidget* parent)
{
    if (!parent) {
        parent = qApp->activeWindow();
    }
    
    QMessageBox msgBox(parent);
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setWindowTitle("TinaFlow - 信息");
    msgBox.setText(message);
    
    if (!details.isEmpty()) {
        msgBox.setDetailedText(details);
    }
    
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}

void ErrorHandler::logError(const TinaFlowException& exception, 
                          const QString& nodeId,
                          const QString& context)
{
    QMutexLocker locker(&m_mutex);
    
    // 创建错误信息
    ErrorInfo error;
    error.type = exception.type();
    error.severity = exception.severity();
    error.message = exception.message();
    error.details = exception.details();
    error.suggestions = exception.recoverySuggestions();
    error.timestamp = QDateTime::currentDateTime();
    error.nodeId = nodeId;
    error.context = context;
    
    // 添加到历史记录
    addErrorToHistory(error);
    
    // 发出信号
    emit errorOccurred(error);
    
    // 调用回调函数
    if (m_errorCallback) {
        m_errorCallback(error);
    }
    
    // 记录到调试输出
    qDebug() << "ErrorHandler: Error logged -"
             << "Type:" << static_cast<int>(error.type)
             << "Message:" << error.message
             << "NodeId:" << nodeId
             << "Context:" << context;
}

QList<ErrorInfo> ErrorHandler::getErrorHistory() const
{
    QMutexLocker locker(&m_mutex);
    return m_errorHistory.toList();
}

void ErrorHandler::clearErrorHistory()
{
    QMutexLocker locker(&m_mutex);
    m_errorHistory.clear();
    qDebug() << "ErrorHandler: Error history cleared";
}

void ErrorHandler::setErrorCallback(std::function<void(const ErrorInfo&)> callback)
{
    QMutexLocker locker(&m_mutex);
    m_errorCallback = callback;
}

QMap<TinaFlowException::ErrorType, int> ErrorHandler::getErrorStatistics() const
{
    QMutexLocker locker(&m_mutex);
    
    QMap<TinaFlowException::ErrorType, int> statistics;
    
    for (const auto& error : m_errorHistory) {
        statistics[error.type]++;
    }
    
    return statistics;
}

void ErrorHandler::addErrorToHistory(const ErrorInfo& error)
{
    m_errorHistory.enqueue(error);
    
    // 限制历史记录数量
    while (m_errorHistory.size() > MAX_ERROR_HISTORY) {
        m_errorHistory.dequeue();
    }
}

QString ErrorHandler::formatErrorMessage(const ErrorInfo& error)
{
    return QString("[%1] %2")
        .arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
        .arg(error.message);
}

QMessageBox::Icon ErrorHandler::getSeverityIcon(TinaFlowException::Severity severity)
{
    switch (severity) {
        case TinaFlowException::Info:
            return QMessageBox::Information;
        case TinaFlowException::Warning:
            return QMessageBox::Warning;
        case TinaFlowException::Error:
            return QMessageBox::Critical;
        case TinaFlowException::Critical:
            return QMessageBox::Critical;
        default:
            return QMessageBox::Warning;
    }
}
