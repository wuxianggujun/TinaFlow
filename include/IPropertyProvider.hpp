//
// Created by wuxianggujun on 25-6-30.
//

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QString>

/**
 * @brief 属性提供者接口
 * 
 * 节点模型可以实现这个接口来提供自己的属性编辑界面。
 * 这样就不需要在MainWindow中为每个节点类型手动添加属性编辑方法。
 */
class IPropertyProvider
{
public:
    virtual ~IPropertyProvider() = default;

    /**
     * @brief 创建属性编辑界面
     * @param parent 父布局，属性控件将被添加到这个布局中
     * @return 是否成功创建了属性界面
     */
    virtual bool createPropertyWidget(QVBoxLayout* parent) = 0;

    /**
     * @brief 获取节点的显示名称（用于属性面板标题）
     * @return 节点显示名称
     */
    virtual QString getDisplayName() const = 0;

    /**
     * @brief 获取节点的描述信息（可选）
     * @return 节点描述，如果不需要可以返回空字符串
     */
    virtual QString getDescription() const { return QString(); }

    /**
     * @brief 属性值改变时的回调（可选）
     * 当用户在属性面板中修改值时会调用此方法
     */
    virtual void onPropertyChanged() {}
};

/**
 * @brief 属性提供者的便利基类
 * 
 * 提供了一些常用的属性控件创建方法，简化节点的属性界面实现。
 */
class PropertyProviderBase : public IPropertyProvider
{
public:
    virtual ~PropertyProviderBase() = default;

protected:
    /**
     * @brief 添加标题标签
     */
    void addTitle(QVBoxLayout* layout, const QString& title)
    {
        auto* titleLabel = new QLabel(title);
        titleLabel->setStyleSheet("font-weight: bold; margin-top: 10px; margin-bottom: 5px;");
        layout->addWidget(titleLabel);
    }

    /**
     * @brief 添加描述标签
     */
    void addDescription(QVBoxLayout* layout, const QString& description)
    {
        auto* descLabel = new QLabel(description);
        descLabel->setStyleSheet("color: #666; font-size: 11px; margin-bottom: 10px;");
        descLabel->setWordWrap(true);
        layout->addWidget(descLabel);
    }

    /**
     * @brief 添加分隔线
     */
    void addSeparator(QVBoxLayout* layout)
    {
        auto* line = new QFrame();
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        line->setStyleSheet("color: #ddd;");
        layout->addWidget(line);
    }

    /**
     * @brief 添加带标签的控件
     */
    void addLabeledWidget(QVBoxLayout* layout, const QString& label, QWidget* widget)
    {
        auto* labelWidget = new QLabel(label);
        labelWidget->setStyleSheet("margin-bottom: 2px;");
        layout->addWidget(labelWidget);
        layout->addWidget(widget);
    }

    /**
     * @brief 添加水平布局的控件组
     */
    QHBoxLayout* addHorizontalGroup(QVBoxLayout* layout)
    {
        auto* hLayout = new QHBoxLayout();
        layout->addLayout(hLayout);
        return hLayout;
    }
};
