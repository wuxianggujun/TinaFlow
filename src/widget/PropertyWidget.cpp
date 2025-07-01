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
    
    qDebug() << "PropertyWidget: Switched to" << (editable ? "edit" : "view") << "mode";
}

void PropertyWidget::addTitle(const QString& title)
{
    auto* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #333; margin-top: 10px;");
    m_layout->addWidget(titleLabel);
}

void PropertyWidget::addDescription(const QString& description)
{
    auto* descLabel = new QLabel(description);
    descLabel->setStyleSheet("color: #666; font-size: 11px; margin-bottom: 10px;");
    descLabel->setWordWrap(true);
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
    auto* buttonLayout = new QHBoxLayout();
    auto* viewButton = new QPushButton("查看模式");
    auto* editButton = new QPushButton("编辑模式");

    viewButton->setCheckable(true);
    editButton->setCheckable(true);
    viewButton->setChecked(true); // 默认查看模式

    buttonLayout->addWidget(viewButton);
    buttonLayout->addWidget(editButton);
    buttonLayout->addStretch();

    m_layout->addLayout(buttonLayout);

    // 添加分隔线
    addSeparator();

    // 连接信号
    connect(editButton, &QPushButton::clicked, [=]() {
        if (!editButton->isChecked()) return;
        viewButton->setChecked(false);
        setEditMode(true);
    });

    connect(viewButton, &QPushButton::clicked, [=]() {
        if (!viewButton->isChecked()) return;
        editButton->setChecked(false);
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
    item.label->setStyleSheet("font-weight: bold; margin-top: 5px;");
    m_layout->addWidget(item.label);
    
    // 查看模式：只读标签
    item.valueLabel = new QLabel(value.isEmpty() ? "未设置" : value);
    item.valueLabel->setStyleSheet(value.isEmpty() ? "color: #999; font-style: italic;" : "color: #333;");
    item.valueLabel->setWordWrap(true);
    item.valueLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_layout->addWidget(item.valueLabel);
    
    // 编辑模式：输入框
    auto* lineEdit = new QLineEdit(value);
    lineEdit->setPlaceholderText(placeholder);
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
        if (item.valueLabel) {
            item.valueLabel->setVisible(!m_editMode);
        }
        if (item.editWidget) {
            item.editWidget->setVisible(m_editMode);
        }

        // 更新显示内容
        if (!m_editMode && item.updateCallback) {
            item.updateCallback();
        }
    }
}

void PropertyWidget::clearAllProperties()
{
    // 清除所有属性控件
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    m_properties.clear();
    m_editMode = false;  // 重置为查看模式

    qDebug() << "PropertyWidget: Cleared all properties";
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

// PropertyPanelManager 实现
PropertyWidget* PropertyPanelManager::createPropertyPanel(QVBoxLayout* parent)
{
    auto* propertyWidget = new PropertyWidget();
    parent->addWidget(propertyWidget);
    return propertyWidget;
}
