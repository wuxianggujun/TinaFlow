#include "widget/PropertyWidget.hpp"
#include "widget/StyledLineEdit.hpp"
#include <QFileDialog>
#include <QDebug>

PropertyWidget::PropertyWidget(QWidget* parent)
    : QWidget(parent)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);
}

void PropertyWidget::setEditMode(bool editable)
{
    if (m_editMode == editable) return;

    m_editMode = editable;
    updatePropertyVisibility();

    // 更新按钮状态
    if (m_viewButton && m_editButton) {
        m_viewButton->setChecked(!editable);
        m_editButton->setChecked(editable);
    }


}

void PropertyWidget::addTitle(const QString& title)
{
    auto* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #333; margin-top: 10px; margin-bottom: 5px;");
    titleLabel->setWordWrap(true);
    m_layout->addWidget(titleLabel);
}

void PropertyWidget::addDescription(const QString& description)
{
    auto* descLabel = new QLabel(description);
    descLabel->setStyleSheet("color: #666; font-size: 11px; margin-bottom: 10px; padding: 5px;");
    descLabel->setWordWrap(true);
    descLabel->setAlignment(Qt::AlignCenter);
    m_layout->addWidget(descLabel);
}

void PropertyWidget::addSeparator()
{
    auto* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    line->setStyleSheet("color: #ddd;");
    m_layout->addWidget(line);
}

void PropertyWidget::addModeToggleButtons()
{
    // 防止重复添加按钮
    if (m_buttonContainer && m_viewButton && m_editButton) {

        return;
    }

    // 清理可能存在的旧按钮
    if (m_buttonContainer) {
        m_buttonContainer->deleteLater();
        m_buttonContainer = nullptr;
        m_viewButton = nullptr;
        m_editButton = nullptr;
    }

    // 添加分隔线
    addSeparator();

    // 创建按钮容器
    m_buttonContainer = new QWidget();
    auto* buttonLayout = new QHBoxLayout(m_buttonContainer);
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    m_viewButton = new QPushButton("查看模式");
    m_editButton = new QPushButton("编辑模式");

    m_viewButton->setCheckable(true);
    m_editButton->setCheckable(true);
    m_viewButton->setChecked(!m_editMode); // 根据当前模式设置
    m_editButton->setChecked(m_editMode);

    buttonLayout->addWidget(m_viewButton);
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addStretch();

    m_layout->addWidget(m_buttonContainer);

    // 连接信号
    connect(m_editButton, &QPushButton::clicked, [this]() {
        if (!m_editButton->isChecked()) return;
        m_viewButton->setChecked(false);
        setEditMode(true);
    });

    connect(m_viewButton, &QPushButton::clicked, [this]() {
        if (!m_viewButton->isChecked()) return;
        m_editButton->setChecked(false);
        setEditMode(false);
    });


}

void PropertyWidget::addTextProperty(const QString& label, const QString& value,
                                    const QString& propertyName, const QString& placeholder,
                                    std::function<void(const QString&)> callback)
{
    PropertyItem item;
    item.name = propertyName;
    
    // 标签
    item.label = new QLabel(label + ":");
    item.label->setStyleSheet("font-weight: bold; margin-top: 8px; margin-bottom: 2px; color: #333;");
    m_layout->addWidget(item.label);

    // 查看模式：只读标签
    item.valueLabel = new QLabel(value.isEmpty() ? "未设置" : value);
    item.valueLabel->setStyleSheet(value.isEmpty() ?
        "color: #999; font-style: italic; padding: 4px; border: 1px solid #ddd; border-radius: 3px; background: #f9f9f9; margin-bottom: 4px;" :
        "color: #333; padding: 4px; border: 1px solid #ddd; border-radius: 3px; background: #f9f9f9; margin-bottom: 4px;");
    item.valueLabel->setWordWrap(true);
    item.valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_layout->addWidget(item.valueLabel);
    
    // 编辑模式：使用自定义样式输入框
    auto* lineEdit = new StyledLineEdit(value);
    lineEdit->setPlaceholderText(placeholder);
    lineEdit->setDoubleClickEnabled(false); // 属性面板不需要双击功能
    item.editWidget = lineEdit;
    m_layout->addWidget(item.editWidget);
    
    // 连接信号 - 使用防抖动的文本变化信号
    if (callback) {
        connect(lineEdit, &StyledLineEdit::textChangedDebounced, [this, propertyName, callback](const QString& text) {
            callback(text);
            emit propertyChanged(propertyName, text);
        });
    }
    
    // 更新回调
    item.updateCallback = [item, value]() {
        if (auto* edit = qobject_cast<StyledLineEdit*>(item.editWidget)) {
            item.valueLabel->setText(edit->text().isEmpty() ? "未设置" : edit->text());
            item.valueLabel->setStyleSheet(edit->text().isEmpty() ?
                "color: #999; font-style: italic;" : "color: #333;");
        }
    };
    
    m_properties.append(item);
    updatePropertyVisibility();
}

void PropertyWidget::addComboProperty(const QString& label, const QStringList& options,
                                     int currentIndex, const QString& propertyName,
                                     std::function<void(int)> callback)
{
    PropertyItem item;
    item.name = propertyName;
    
    // 标签
    item.label = new QLabel(label + ":");
    item.label->setStyleSheet("font-weight: bold; margin-top: 5px;");
    m_layout->addWidget(item.label);
    
    // 查看模式：显示当前选项
    QString currentText = (currentIndex >= 0 && currentIndex < options.size()) ? 
                         options[currentIndex] : "未设置";
    item.valueLabel = new QLabel(currentText);
    item.valueLabel->setStyleSheet("color: #333; font-weight: bold;");
    m_layout->addWidget(item.valueLabel);
    
    // 编辑模式：下拉框
    auto* comboBox = new QComboBox();
    comboBox->addItems(options);
    comboBox->setCurrentIndex(currentIndex);
    item.editWidget = comboBox;
    m_layout->addWidget(item.editWidget);
    
    // 连接信号
    if (callback) {
        connect(comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, propertyName, callback](int index) {
            callback(index);
            emit propertyChanged(propertyName, index);
        });
    }
    
    // 更新回调
    item.updateCallback = [item, options]() {
        if (auto* combo = qobject_cast<QComboBox*>(item.editWidget)) {
            int index = combo->currentIndex();
            QString text = (index >= 0 && index < options.size()) ? options[index] : "未设置";
            item.valueLabel->setText(text);
        }
    };
    
    m_properties.append(item);
    updatePropertyVisibility();
}

void PropertyWidget::addFilePathProperty(const QString& label, const QString& path,
                                        const QString& propertyName, const QString& filter,
                                        bool saveMode, std::function<void(const QString&)> callback)
{
    PropertyItem item;
    item.name = propertyName;

    // 标签
    item.label = new QLabel(label + ":");
    item.label->setStyleSheet("font-weight: bold; margin-top: 8px; margin-bottom: 2px; color: #333;");
    m_layout->addWidget(item.label);

    // 查看模式：显示路径
    item.valueLabel = new QLabel(path.isEmpty() ? "未设置" : path);
    item.valueLabel->setStyleSheet(path.isEmpty() ?
        "color: #999; font-style: italic; padding: 4px; border: 1px solid #ddd; border-radius: 3px; background: #f9f9f9; margin-bottom: 4px;" :
        "color: #333; padding: 4px; border: 1px solid #ddd; border-radius: 3px; background: #f9f9f9; margin-bottom: 4px;");
    item.valueLabel->setWordWrap(true);
    item.valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_layout->addWidget(item.valueLabel);

    // 编辑模式：输入框 + 浏览按钮
    auto* container = new QWidget();
    auto* layout = new QHBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* lineEdit = new StyledLineEdit(path);
    lineEdit->setPlaceholderText("选择文件路径");
    lineEdit->setDoubleClickEnabled(false); // 文件路径不需要双击功能
    layout->addWidget(lineEdit);

    auto* browseButton = new QPushButton("浏览...");
    browseButton->setMaximumWidth(80);
    layout->addWidget(browseButton);
    item.browseButton = browseButton;

    item.editWidget = container;
    m_layout->addWidget(item.editWidget);

    // 连接浏览按钮
    connect(browseButton, &QPushButton::clicked, [this, lineEdit, filter, saveMode, callback, propertyName]() {
        QString fileName;
        if (saveMode) {
            fileName = QFileDialog::getSaveFileName(this, "保存文件", "", filter);
            if (!fileName.isEmpty() && !fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
                fileName += ".xlsx";
            }
        } else {
            fileName = QFileDialog::getOpenFileName(this, "选择文件", "", filter);
        }

        if (!fileName.isEmpty()) {
            lineEdit->setText(fileName);
            if (callback) {
                callback(fileName);
            }
            emit propertyChanged(propertyName, fileName);
        }
    });

    // 连接输入框 - 使用防抖动信号
    if (callback) {
        connect(lineEdit, &StyledLineEdit::textChangedDebounced, [this, propertyName, callback](const QString& text) {
            callback(text);
            emit propertyChanged(propertyName, text);
        });
    }

    // 更新回调
    item.updateCallback = [item]() {
        if (auto* container = item.editWidget) {
            auto* lineEdit = container->findChild<StyledLineEdit*>();
            if (lineEdit) {
                QString text = lineEdit->text();
                item.valueLabel->setText(text.isEmpty() ? "未设置" : text);
                item.valueLabel->setStyleSheet(text.isEmpty() ?
                    "color: #999; font-style: italic; padding: 4px; border: 1px solid #ddd; border-radius: 3px; background: #f9f9f9; margin-bottom: 4px;" :
                    "color: #333; padding: 4px; border: 1px solid #ddd; border-radius: 3px; background: #f9f9f9; margin-bottom: 4px;");
            }
        }
    };

    m_properties.append(item);
    updatePropertyVisibility();
}

void PropertyWidget::addCheckBoxProperty(const QString& label, bool checked,
                                        const QString& propertyName,
                                        std::function<void(bool)> callback)
{
    PropertyItem item;
    item.name = propertyName;
    
    // 查看模式：标签显示
    item.valueLabel = new QLabel(QString("%1: %2").arg(label).arg(checked ? "是" : "否"));
    item.valueLabel->setStyleSheet("color: #333; margin-top: 5px;");
    m_layout->addWidget(item.valueLabel);
    
    // 编辑模式：复选框
    auto* checkBox = new QCheckBox(label);
    checkBox->setChecked(checked);
    item.editWidget = checkBox;
    m_layout->addWidget(item.editWidget);
    
    // 连接信号
    if (callback) {
        connect(checkBox, &QCheckBox::toggled, [this, propertyName, callback](bool checked) {
            callback(checked);
            emit propertyChanged(propertyName, checked);
        });
    }
    
    // 更新回调
    item.updateCallback = [item, label]() {
        if (auto* check = qobject_cast<QCheckBox*>(item.editWidget)) {
            bool checked = check->isChecked();
            item.valueLabel->setText(QString("%1: %2").arg(label).arg(checked ? "是" : "否"));
        }
    };
    
    m_properties.append(item);
    updatePropertyVisibility();
}



void PropertyWidget::addInfoProperty(const QString& label, const QString& value, const QString& style)
{
    auto* infoLabel = new QLabel(QString("%1: %2").arg(label).arg(value));
    infoLabel->setStyleSheet(style.isEmpty() ? "color: #666; margin-top: 5px;" : style);
    infoLabel->setWordWrap(true);
    m_layout->addWidget(infoLabel);
}



void PropertyWidget::updatePropertyVisibility()
{
    for (auto& item : m_properties) {
        // 标签始终显示
        if (item.label) {
            item.label->setVisible(true);
        }

        // 根据模式显示不同的控件
        if (item.valueLabel) {
            item.valueLabel->setVisible(!m_editMode);
        }
        if (item.editWidget) {
            item.editWidget->setVisible(m_editMode);
        }
        if (item.browseButton) {
            item.browseButton->setVisible(m_editMode);
        }

        // 更新显示内容
        if (!m_editMode && item.updateCallback) {
            item.updateCallback();
        }
    }

    // 强制更新布局
    m_layout->update();
}

void PropertyWidget::clearAllProperties()
{


    // 第一步：清理模式切换按钮
    if (m_buttonContainer) {
        m_buttonContainer->disconnect();
        m_buttonContainer->hide();
        m_buttonContainer->deleteLater();
        m_buttonContainer = nullptr;
        m_viewButton = nullptr;
        m_editButton = nullptr;

    }

    // 第二步：清理属性控件
    for (auto& item : m_properties) {
        if (item.label) {
            item.label->disconnect();
            item.label->hide();
            item.label->deleteLater();
            item.label = nullptr;
        }
        if (item.valueLabel) {
            item.valueLabel->disconnect();
            item.valueLabel->hide();
            item.valueLabel->deleteLater();
            item.valueLabel = nullptr;
        }
        if (item.editWidget) {
            item.editWidget->disconnect();
            item.editWidget->hide();
            item.editWidget->deleteLater();
            item.editWidget = nullptr;
        }
        if (item.browseButton) {
            item.browseButton->disconnect();
            item.browseButton->hide();
            item.browseButton->deleteLater();
            item.browseButton = nullptr;
        }
    }

    // 第三步：清除布局中剩余的项目
    while (m_layout->count() > 0) {
        QLayoutItem* layoutItem = m_layout->takeAt(0);
        if (layoutItem) {
            if (layoutItem->widget()) {
                QWidget* widget = layoutItem->widget();
                widget->setParent(nullptr);
                widget->hide();
                widget->deleteLater();
            }
            delete layoutItem;
        }
    }

    // 第四步：重置状态
    m_properties.clear();
    m_editMode = false;

    // 第五步：强制更新布局
    m_layout->invalidate();
    m_layout->update();
    this->update();


}





PropertyWidget::PropertyItem* PropertyWidget::findProperty(const QString& name)
{
    for (auto& item : m_properties) {
        if (item.name == name) {
            return &item;
        }
    }
    return nullptr;
}

void PropertyWidget::finishLayout()
{
    // 添加弹性空间，防止控件被拉伸
    m_layout->addStretch();

    // 设置布局的对齐方式
    m_layout->setAlignment(Qt::AlignTop);

    // 设置控件的大小策略
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
}


