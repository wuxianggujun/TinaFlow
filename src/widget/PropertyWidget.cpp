#include "widget/PropertyWidget.hpp"
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

    qDebug() << "PropertyWidget: Switched to" << (editable ? "edit" : "view") << "mode";
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
        qDebug() << "PropertyWidget: 切换按钮已存在，跳过添加";
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

    qDebug() << "PropertyWidget: 模式切换按钮已添加";
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
    
    // 编辑模式：输入框
    auto* lineEdit = new QLineEdit(value);
    lineEdit->setPlaceholderText(placeholder);
    lineEdit->setStyleSheet("padding: 4px; border: 1px solid #ccc; border-radius: 3px; margin-bottom: 4px;");
    item.editWidget = lineEdit;
    m_layout->addWidget(item.editWidget);
    
    // 连接信号
    if (callback) {
        connect(lineEdit, &QLineEdit::textChanged, [this, propertyName, callback](const QString& text) {
            callback(text);
            emit propertyChanged(propertyName, text);
        });
    }
    
    // 更新回调
    item.updateCallback = [item, value]() {
        if (auto* edit = qobject_cast<QLineEdit*>(item.editWidget)) {
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

void PropertyWidget::addFilePathProperty(const QString& label, const QString& path,
                                        const QString& propertyName, const QString& filter,
                                        bool saveMode, std::function<void(const QString&)> callback)
{
    PropertyItem item;
    item.name = propertyName;

    // 标签
    item.label = new QLabel(label + ":");
    item.label->setStyleSheet("font-weight: bold; margin-top: 5px;");
    m_layout->addWidget(item.label);

    // 查看模式：显示路径
    item.valueLabel = new QLabel(path.isEmpty() ? "未设置" : path);
    item.valueLabel->setStyleSheet(path.isEmpty() ? "color: #999; font-style: italic;" : "color: #333;");
    item.valueLabel->setWordWrap(true);
    item.valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_layout->addWidget(item.valueLabel);

    // 编辑模式：输入框 + 浏览按钮
    auto* editContainer = new QWidget();
    auto* editLayout = new QHBoxLayout(editContainer);
    editLayout->setContentsMargins(0, 0, 0, 0);

    auto* lineEdit = new QLineEdit(path);
    lineEdit->setPlaceholderText("输入路径或点击浏览...");
    editLayout->addWidget(lineEdit);

    auto* browseButton = new QPushButton("浏览...");
    editLayout->addWidget(browseButton);
    item.browseButton = browseButton;

    item.editWidget = editContainer;
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

    // 连接输入框
    if (callback) {
        connect(lineEdit, &QLineEdit::textChanged, [this, propertyName, callback](const QString& text) {
            callback(text);
            emit propertyChanged(propertyName, text);
        });
    }

    // 更新回调
    item.updateCallback = [item]() {
        if (auto* container = item.editWidget) {
            auto* lineEdit = container->findChild<QLineEdit*>();
            if (lineEdit) {
                QString text = lineEdit->text();
                item.valueLabel->setText(text.isEmpty() ? "未设置" : text);
                item.valueLabel->setStyleSheet(text.isEmpty() ?
                    "color: #999; font-style: italic;" : "color: #333;");
            }
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

void PropertyWidget::updatePropertyValue(const QString& propertyName, const QVariant& value)
{
    PropertyItem* item = findProperty(propertyName);
    if (!item) return;

    // 更新编辑控件的值（不触发信号）
    if (auto* lineEdit = qobject_cast<QLineEdit*>(item->editWidget)) {
        lineEdit->blockSignals(true);
        lineEdit->setText(value.toString());
        lineEdit->blockSignals(false);
    } else if (auto* comboBox = qobject_cast<QComboBox*>(item->editWidget)) {
        comboBox->blockSignals(true);
        comboBox->setCurrentIndex(value.toInt());
        comboBox->blockSignals(false);
    } else if (auto* checkBox = qobject_cast<QCheckBox*>(item->editWidget)) {
        checkBox->blockSignals(true);
        checkBox->setChecked(value.toBool());
        checkBox->blockSignals(false);
    } else if (item->editWidget) {
        // 文件路径控件
        auto* lineEdit = item->editWidget->findChild<QLineEdit*>();
        if (lineEdit) {
            lineEdit->blockSignals(true);
            lineEdit->setText(value.toString());
            lineEdit->blockSignals(false);
        }
    }

    // 更新显示
    if (item->updateCallback) {
        item->updateCallback();
    }
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
    qDebug() << "PropertyWidget: 开始清理属性，当前属性数量:" << m_properties.size();

    // 第一步：清理模式切换按钮
    if (m_buttonContainer) {
        m_buttonContainer->disconnect();
        m_buttonContainer->hide();
        m_buttonContainer->deleteLater();
        m_buttonContainer = nullptr;
        m_viewButton = nullptr;
        m_editButton = nullptr;
        qDebug() << "PropertyWidget: 模式切换按钮已清理";
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

    qDebug() << "PropertyWidget: 属性清理完成，布局项目数量:" << m_layout->count();
}

void PropertyWidget::forceReset()
{
    qDebug() << "PropertyWidget: 开始强制重置";

    // 清理按钮引用
    m_buttonContainer = nullptr;
    m_viewButton = nullptr;
    m_editButton = nullptr;

    // 完全销毁当前布局
    if (m_layout) {
        // 移除所有子控件
        while (m_layout->count() > 0) {
            QLayoutItem* item = m_layout->takeAt(0);
            if (item) {
                if (item->widget()) {
                    item->widget()->setParent(nullptr);
                    item->widget()->deleteLater();
                }
                delete item;
            }
        }

        // 删除布局本身
        delete m_layout;
    }

    // 清空属性列表
    m_properties.clear();
    m_editMode = false;

    // 重新创建布局
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(8);

    // 强制更新
    this->update();

    qDebug() << "PropertyWidget: 强制重置完成";
}

void PropertyWidget::debugLayoutState() const
{
    qDebug() << "=== PropertyWidget 布局状态调试 ===";
    qDebug() << "布局项目数量:" << m_layout->count();
    qDebug() << "属性列表大小:" << m_properties.size();
    qDebug() << "编辑模式:" << m_editMode;
    qDebug() << "按钮容器存在:" << (m_buttonContainer != nullptr);
    qDebug() << "查看按钮存在:" << (m_viewButton != nullptr);
    qDebug() << "编辑按钮存在:" << (m_editButton != nullptr);

    for (int i = 0; i < m_layout->count(); ++i) {
        QLayoutItem* item = m_layout->itemAt(i);
        if (item && item->widget()) {
            QWidget* widget = item->widget();
            qDebug() << "布局项" << i << ":" << widget->metaObject()->className()
                     << "可见性:" << widget->isVisible()
                     << "大小:" << widget->size();
        }
    }

    for (int i = 0; i < m_properties.size(); ++i) {
        const auto& prop = m_properties[i];
        qDebug() << "属性" << i << "名称:" << prop.name;
        if (prop.label) qDebug() << "  - 标签可见:" << prop.label->isVisible();
        if (prop.valueLabel) qDebug() << "  - 值标签可见:" << prop.valueLabel->isVisible();
        if (prop.editWidget) qDebug() << "  - 编辑控件可见:" << prop.editWidget->isVisible();
    }
    qDebug() << "=== 调试结束 ===";
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


