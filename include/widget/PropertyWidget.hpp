#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QFrame>
#include <QString>
#include <QVariant>
#include <functional>

/**
 * @brief 属性控件基类
 * 支持查看/编辑模式切换，避免重复创建控件
 */
class PropertyWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PropertyWidget(QWidget* parent = nullptr);
    
    /**
     * @brief 设置编辑模式
     * @param editable true=编辑模式，false=查看模式
     */
    void setEditMode(bool editable);
    
    /**
     * @brief 添加标题
     */
    void addTitle(const QString& title);
    
    /**
     * @brief 添加描述文本
     */
    void addDescription(const QString& description);

    /**
     * @brief 添加模式切换按钮（在描述后调用）
     */
    void addModeToggleButtons();
    
    /**
     * @brief 添加分隔线
     */
    void addSeparator();
    
    /**
     * @brief 添加文本属性
     * @param label 标签文本
     * @param value 当前值
     * @param propertyName 属性名称
     * @param placeholder 占位符文本
     * @param callback 值改变时的回调函数
     */
    void addTextProperty(const QString& label, const QString& value, 
                        const QString& propertyName, const QString& placeholder = "",
                        std::function<void(const QString&)> callback = nullptr);
    
    /**
     * @brief 添加下拉框属性
     * @param label 标签文本
     * @param options 选项列表
     * @param currentIndex 当前选中索引
     * @param propertyName 属性名称
     * @param callback 值改变时的回调函数
     */
    void addComboProperty(const QString& label, const QStringList& options,
                         int currentIndex, const QString& propertyName,
                         std::function<void(int)> callback = nullptr);
    
    /**
     * @brief 添加复选框属性
     * @param label 标签文本
     * @param checked 是否选中
     * @param propertyName 属性名称
     * @param callback 值改变时的回调函数
     */
    void addCheckBoxProperty(const QString& label, bool checked,
                            const QString& propertyName,
                            std::function<void(bool)> callback = nullptr);
    
    /**
     * @brief 添加文件路径属性
     * @param label 标签文本
     * @param path 当前路径
     * @param propertyName 属性名称
     * @param filter 文件过滤器
     * @param saveMode 是否为保存模式
     * @param callback 值改变时的回调函数
     */
    void addFilePathProperty(const QString& label, const QString& path,
                            const QString& propertyName, const QString& filter = "",
                            bool saveMode = false,
                            std::function<void(const QString&)> callback = nullptr);
    
    /**
     * @brief 添加只读信息
     * @param label 标签文本
     * @param value 显示值
     * @param style 样式（可选）
     */
    void addInfoProperty(const QString& label, const QString& value, 
                        const QString& style = "");
    
    /**
     * @brief 更新属性值（不触发回调）
     */
    void updatePropertyValue(const QString& propertyName, const QVariant& value);

    /**
     * @brief 清空所有属性（复用控件）
     */
    void clearAllProperties();

    /**
     * @brief 强制重置控件（彻底清理）
     */
    void forceReset();

    /**
     * @brief 检查是否有属性
     */
    bool hasProperties() const { return !m_properties.isEmpty(); }

    /**
     * @brief 调试布局状态
     */
    void debugLayoutState() const;

signals:
    void propertyChanged(const QString& propertyName, const QVariant& value);

private:
    struct PropertyItem {
        QString name;
        QLabel* label = nullptr;
        QLabel* valueLabel = nullptr;  // 查看模式显示
        QWidget* editWidget = nullptr; // 编辑模式控件
        QPushButton* browseButton = nullptr; // 文件浏览按钮
        std::function<void()> updateCallback; // 更新显示的回调
    };

    QVBoxLayout* m_layout;
    QList<PropertyItem> m_properties;
    bool m_editMode = false;

    // 模式切换按钮管理
    QWidget* m_buttonContainer = nullptr;
    QPushButton* m_viewButton = nullptr;
    QPushButton* m_editButton = nullptr;
    
    void updatePropertyVisibility();
    PropertyItem* findProperty(const QString& name);
};


