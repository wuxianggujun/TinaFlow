//
// Created by wuxianggujun on 25-7-3.
//

#include "widget/StyledLineEdit.hpp"
#include <QDebug>
#include <QSizePolicy>

StyledLineEdit::StyledLineEdit(QWidget* parent)
    : QLineEdit(parent)
{
    setupUI();
}

StyledLineEdit::StyledLineEdit(const QString& text, QWidget* parent)
    : QLineEdit(text, parent)
{
    setupUI();
}

void StyledLineEdit::setupUI()
{
    // 设置大小策略
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(22);
    setMaximumHeight(26);
    setMinimumWidth(80);  // 设置最小宽度

    // 设置内容边距
    setContentsMargins(0, 0, 0, 0);

    // 设置默认样式
    updateStyleSheet();

    // 设置防抖动定时器
    m_debounceTimer = new QTimer(this);
    m_debounceTimer->setSingleShot(true);
    connect(m_debounceTimer, &QTimer::timeout, this, &StyledLineEdit::onDebounceTimeout);

    // 连接文本变化信号
    connect(this, &QLineEdit::textChanged, this, &StyledLineEdit::onTextChanged);
}

void StyledLineEdit::setTheme(Theme theme)
{
    m_theme = theme;

    // 根据主题设置颜色
    switch (m_theme) {
        case Theme::Primary:
            m_normalBorderColor = "#0066cc";
            m_focusBorderColor = "#0052a3";
            break;
        case Theme::Success:
            m_normalBorderColor = "#28a745";
            m_focusBorderColor = "#1e7e34";
            break;
        case Theme::Warning:
            m_normalBorderColor = "#ffc107";
            m_focusBorderColor = "#e0a800";
            break;
        case Theme::Error:
            m_normalBorderColor = "#dc3545";
            m_focusBorderColor = "#c82333";
            break;
        case Theme::Default:
        default:
            m_normalBorderColor = "#cccccc";
            m_focusBorderColor = "#0066cc";
            break;
    }

    updateStyleSheet();
}

void StyledLineEdit::setCustomColors(const QString& normalBorder, const QString& focusBorder, 
                                   const QString& background, const QString& textColor)
{
    m_normalBorderColor = normalBorder;
    m_focusBorderColor = focusBorder;
    m_backgroundColor = background;
    m_textColor = textColor;
    updateStyleSheet();
}

void StyledLineEdit::setPlaceholderTextWithStyle(const QString& text, const QString& color)
{
    setPlaceholderText(text);
    // 占位符颜色会在 updateStyleSheet 中设置
    updateStyleSheet();
}

void StyledLineEdit::updateStyleSheet()
{
    QString styleSheet = QString(
        "QLineEdit {"
        "    font-size: 10px;"
        "    border: 1px solid %1;"
        "    border-radius: 0px;"  // 统一使用直角设计
        "    padding: 3px 6px;"
        "    background-color: %2;"
        "    color: %3;"
        "    selection-background-color: #b3d9ff;"
        "    selection-color: #000000;"
        "    margin: 0px;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid %4;"
        "    background-color: %5;"
        "    padding: 2px 5px;"
        "}"
        "QLineEdit:disabled {"
        "    background-color: #f5f5f5;"
        "    color: #999999;"
        "    border-color: #e0e0e0;"
        "}"
    ).arg(m_normalBorderColor)
     .arg(m_backgroundColor)
     .arg(m_textColor)
     .arg(m_focusBorderColor)
     .arg(m_backgroundColor == "white" ? "#f8f8ff" : m_backgroundColor);
    
    setStyleSheet(styleSheet);
}

QString StyledLineEdit::getThemeColors() const
{
    // 这个方法现在不需要返回任何内容，主题颜色在 setTheme 中设置
    return "";
}

void StyledLineEdit::focusInEvent(QFocusEvent* event)
{
    QLineEdit::focusInEvent(event);
    updateStyleSheet(); // 更新焦点样式
}

void StyledLineEdit::focusOutEvent(QFocusEvent* event)
{
    QLineEdit::focusOutEvent(event);
    updateStyleSheet(); // 更新失焦样式
}

void StyledLineEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
    QLineEdit::mouseDoubleClickEvent(event);
    
    if (m_doubleClickEnabled) {
        emit doubleClicked();
    }
}

void StyledLineEdit::onTextChanged(const QString& text)
{
    // 重启防抖动定时器
    m_debounceTimer->stop();
    m_debounceTimer->start(m_debounceDelay);
}

void StyledLineEdit::onDebounceTimeout()
{
    QString currentText = text();
    if (currentText != m_lastEmittedText) {
        m_lastEmittedText = currentText;
        emit textChangedDebounced(currentText);
    }
}

// ConstantValueLineEdit 实现

ConstantValueLineEdit::ConstantValueLineEdit(QWidget* parent)
    : StyledLineEdit(parent)
{
    setupConstantValueStyle();
}

void ConstantValueLineEdit::setupConstantValueStyle()
{
    // 设置适合常量值的样式
    setTheme(Theme::Default);
    setDebounceDelay(200); // 较短的防抖动延迟
    setDoubleClickEnabled(true); // 启用双击切换类型

    // 设置固定宽度，恢复之前的节点大小
    setMinimumWidth(120);
    setMaximumWidth(200);
    setMinimumHeight(22);
    setMaximumHeight(26);

    // 为节点使用直角样式，完全填充容器
    setStyleSheet(
        "QLineEdit {"
        "    font-size: 10px;"
        "    border: 1px solid #cccccc;"
        "    border-radius: 0px;"  // 直角设计
        "    padding: 3px 6px;"
        "    background-color: white;"
        "    color: #333333;"
        "    margin: 0px;"
        "}"
        "QLineEdit:focus {"
        "    border: 2px solid #0066cc;"
        "    background-color: #f8f8ff;"
        "    padding: 2px 5px;"
        "}"
        "QLineEdit:disabled {"
        "    background-color: #f5f5f5;"
        "    color: #999999;"
        "    border-color: #e0e0e0;"
        "}"
    );

    // 连接双击信号
    connect(this, &StyledLineEdit::doubleClicked, this, &ConstantValueLineEdit::typeChangeRequested);

    // 设置默认占位符
    setPlaceholderTextWithStyle("输入值 (双击切换类型)", "#999999");
}

void ConstantValueLineEdit::setValueType(const QString& typeName, const QString& placeholder)
{
    QString fullPlaceholder = QString("[%1] %2 (双击切换类型)").arg(typeName, placeholder);
    setPlaceholderTextWithStyle(fullPlaceholder, "#999999");
    
    // 根据类型设置不同的边框颜色提示
    if (typeName == "字符串") {
        setCustomColors("#4CAF50", "#2E7D32", "white"); // 绿色
    } else if (typeName == "数值") {
        setCustomColors("#2196F3", "#1565C0", "white"); // 蓝色
    } else if (typeName == "布尔值") {
        setCustomColors("#FF9800", "#E65100", "white"); // 橙色
    } else {
        setCustomColors("#cccccc", "#0066cc", "white"); // 默认
    }
    
    setToolTip(QString("当前类型: %1\n双击可切换类型").arg(typeName));
}

void ConstantValueLineEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
    // 先调用基类处理
    StyledLineEdit::mouseDoubleClickEvent(event);
    
    // 发射类型切换信号
    emit typeChangeRequested();
}
