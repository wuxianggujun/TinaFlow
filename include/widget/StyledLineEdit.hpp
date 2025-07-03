//
// Created by wuxianggujun on 25-7-3.
//

#pragma once

#include <QLineEdit>
#include <QEvent>
#include <QFocusEvent>
#include <QMouseEvent>
#include <QString>
#include <QTimer>

/**
 * @brief 自定义样式的输入框
 * 
 * 特性：
 * - 无下划线的现代化样式
 * - 支持占位符文本
 * - 焦点状态的视觉反馈
 * - 可自定义的颜色主题
 * - 支持双击事件
 * - 防抖动的文本变化事件
 */
class StyledLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    /**
     * @brief 预定义的样式主题
     */
    enum class Theme {
        Default,    // 默认主题（浅色）
        Primary,    // 主要主题（蓝色）
        Success,    // 成功主题（绿色）
        Warning,    // 警告主题（橙色）
        Error       // 错误主题（红色）
    };

    explicit StyledLineEdit(QWidget* parent = nullptr);
    explicit StyledLineEdit(const QString& text, QWidget* parent = nullptr);

    // 主题设置
    void setTheme(Theme theme);
    Theme currentTheme() const { return m_theme; }

    // 自定义样式设置
    void setCustomColors(const QString& normalBorder, const QString& focusBorder, 
                        const QString& background, const QString& textColor = "#333333");

    // 占位符增强
    void setPlaceholderTextWithStyle(const QString& text, const QString& color = "#999999");

    // 防抖动设置
    void setDebounceDelay(int milliseconds) { m_debounceDelay = milliseconds; }
    int debounceDelay() const { return m_debounceDelay; }

    // 启用/禁用双击事件
    void setDoubleClickEnabled(bool enabled) { m_doubleClickEnabled = enabled; }
    bool isDoubleClickEnabled() const { return m_doubleClickEnabled; }

signals:
    /**
     * @brief 防抖动的文本变化信号
     * @param text 新的文本内容
     * 
     * 只有在用户停止输入指定时间后才会发射
     */
    void textChangedDebounced(const QString& text);

    /**
     * @brief 双击事件信号
     */
    void doubleClicked();

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void onTextChanged(const QString& text);
    void onDebounceTimeout();

private:
    void setupUI();
    void updateStyleSheet();
    QString getThemeColors() const;

private:
    Theme m_theme = Theme::Default;
    
    // 自定义颜色
    QString m_normalBorderColor = "#cccccc";
    QString m_focusBorderColor = "#0066cc";
    QString m_backgroundColor = "white";
    QString m_textColor = "#333333";
    
    // 防抖动
    QTimer* m_debounceTimer = nullptr;
    int m_debounceDelay = 300; // 毫秒
    QString m_lastEmittedText;
    
    // 双击支持
    bool m_doubleClickEnabled = false;
};

/**
 * @brief 专门用于常量值输入的样式化输入框
 * 
 * 预配置了适合常量值输入的样式和行为
 */
class ConstantValueLineEdit : public StyledLineEdit
{
    Q_OBJECT

public:
    explicit ConstantValueLineEdit(QWidget* parent = nullptr);

    // 设置数据类型相关的样式和占位符
    void setValueType(const QString& typeName, const QString& placeholder);

signals:
    /**
     * @brief 类型切换请求信号
     */
    void typeChangeRequested();

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
    void setupConstantValueStyle();
};
