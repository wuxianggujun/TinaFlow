//
// Created by wuxianggujun on 25-6-30.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/RangeData.hpp"
#include "data/BooleanData.hpp"
#include "PropertyWidget.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QGroupBox>
#include <QtNodes/NodeDelegateModel>
#include <QDebug>

#include <OpenXLSX.hpp>

/**
 * @brief 保存Excel文件的节点模型
 * 
 * 功能：
 * - 接收RangeData数据并保存到Excel文件
 * - 支持自定义文件路径和sheet名称
 * - 支持创建新文件或追加到现有文件
 * - 提供保存进度反馈
 * 
 * 输入端口：
 * - 0: RangeData - 要保存的数据
 * 
 * 输出端口：
 * - 0: BooleanData - 保存成功/失败状态
 */
class SaveExcelModel : public BaseNodeModel
{
    Q_OBJECT

public:
    SaveExcelModel()
    {
        // 创建UI组件
        m_widget = new QWidget();
        m_widget->setMinimumSize(280, 200);
        
        auto* mainLayout = new QVBoxLayout(m_widget);
        mainLayout->setContentsMargins(8, 8, 8, 8);
        mainLayout->setSpacing(6);

        // 文件路径组
        auto* fileGroup = new QGroupBox("文件设置");
        auto* fileLayout = new QVBoxLayout(fileGroup);
        
        // 文件路径
        auto* pathLayout = new QHBoxLayout();
        pathLayout->addWidget(new QLabel("文件路径:"));
        
        m_filePathEdit = new QLineEdit();
        m_filePathEdit->setPlaceholderText("选择Excel文件路径...");
        pathLayout->addWidget(m_filePathEdit);
        
        m_browseButton = new QPushButton("浏览");
        m_browseButton->setMaximumWidth(60);
        pathLayout->addWidget(m_browseButton);
        
        fileLayout->addLayout(pathLayout);
        
        // Sheet名称
        auto* sheetLayout = new QHBoxLayout();
        sheetLayout->addWidget(new QLabel("Sheet名称:"));
        
        m_sheetNameEdit = new QLineEdit();
        m_sheetNameEdit->setText("Sheet1");
        m_sheetNameEdit->setPlaceholderText("输入sheet名称...");
        sheetLayout->addWidget(m_sheetNameEdit);
        
        fileLayout->addLayout(sheetLayout);
        
        mainLayout->addWidget(fileGroup);

        // 操作组
        auto* actionGroup = new QGroupBox("操作");
        auto* actionLayout = new QVBoxLayout(actionGroup);
        
        // 保存按钮（改为状态显示）
        m_saveButton = new QPushButton("等待数据...");
        m_saveButton->setEnabled(false);
        m_saveButton->setStyleSheet("QPushButton { background-color: #f0f0f0; color: #666; }");
        actionLayout->addWidget(m_saveButton);
        
        // 进度条
        m_progressBar = new QProgressBar();
        m_progressBar->setVisible(false);
        actionLayout->addWidget(m_progressBar);
        
        // 状态标签
        m_statusLabel = new QLabel("等待数据输入...");
        m_statusLabel->setStyleSheet("color: #666; font-size: 11px;");
        m_statusLabel->setWordWrap(true);
        actionLayout->addWidget(m_statusLabel);
        
        mainLayout->addWidget(actionGroup);

        // 连接信号
        connect(m_browseButton, &QPushButton::clicked, this, &SaveExcelModel::onBrowseFile);
        // 移除保存按钮的点击信号，改为自动保存
        connect(m_filePathEdit, &QLineEdit::textChanged, this, &SaveExcelModel::onFilePathChanged);
        connect(m_sheetNameEdit, &QLineEdit::textChanged, this, &SaveExcelModel::onSheetNameChanged);

        // 初始化输出数据
        m_saveResult = std::make_shared<BooleanData>(false, "未开始保存");

        // 注册需要保存的属性
        registerLineEdit("filePath", m_filePathEdit, "文件路径");
        registerLineEdit("sheetName", m_sheetNameEdit, "Sheet名称");

        qDebug() << "SaveExcelModel: Created";
    }

    QString caption() const override
    {
        return "保存Excel";
    }
    
    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return "SaveExcel";
    }

    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        if (portType == QtNodes::PortType::In) {
            return 1; // RangeData输入
        } else {
            return 1; // BooleanData输出
        }
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In) {
            return RangeData().type();
        } else {
            return BooleanData().type();
        }
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return m_saveResult;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "SaveExcelModel::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << "SaveExcelModel: Received null nodeData";
            m_rangeData.reset();
            updateUI();
            return;
        }
        
        m_rangeData = std::dynamic_pointer_cast<RangeData>(nodeData);
        if (m_rangeData) {
            qDebug() << "SaveExcelModel: Successfully received RangeData with"
                     << m_rangeData->rowCount() << "rows and"
                     << m_rangeData->columnCount() << "columns";

            // 自动保存数据
            autoSaveData();
        } else {
            qDebug() << "SaveExcelModel: Failed to cast to RangeData";
        }

        updateUI();
    }

protected:
    // 实现基类的虚函数
    QString getNodeTypeName() const override
    {
        return "SaveExcelModel";
    }

    // 自定义加载后处理
    void onLoad(const QJsonObject& json) override
    {
        updateUI(); // 加载后更新UI状态
    }

private slots:
    void onBrowseFile()
    {
        // 使用nullptr作为父窗口，避免QGraphicsProxyWidget的问题
        QString fileName = QFileDialog::getSaveFileName(
            nullptr,  // 改为nullptr，避免在QGraphicsProxyWidget中的问题
            "保存Excel文件",
            "",
            "Excel文件 (*.xlsx);;所有文件 (*)"
        );

        if (!fileName.isEmpty()) {
            // 确保文件扩展名
            if (!fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
                fileName += ".xlsx";
            }
            m_filePathEdit->setText(fileName);
        }
    }

    void onFilePathChanged()
    {
        updateUI();
    }

    void onSheetNameChanged()
    {
        updateUI();
    }



private:
    void updateUI()
    {
        bool hasData = m_rangeData && !m_rangeData->isEmpty();
        bool hasPath = !m_filePathEdit->text().trimmed().isEmpty();
        bool hasSheetName = !m_sheetNameEdit->text().trimmed().isEmpty();

        // 更新按钮状态和文本
        if (!hasData) {
            m_saveButton->setText("等待数据...");
            m_saveButton->setStyleSheet("QPushButton { background-color: #f0f0f0; color: #666; }");
            m_statusLabel->setText("等待数据输入...");
        } else if (!hasPath) {
            m_saveButton->setText("需要路径");
            m_saveButton->setStyleSheet("QPushButton { background-color: #fff3cd; color: #856404; }");
            m_statusLabel->setText("请选择保存路径");
        } else if (!hasSheetName) {
            m_saveButton->setText("需要Sheet名");
            m_saveButton->setStyleSheet("QPushButton { background-color: #fff3cd; color: #856404; }");
            m_statusLabel->setText("请输入Sheet名称");
        } else {
            m_saveButton->setText("自动保存");
            m_saveButton->setStyleSheet("QPushButton { background-color: #d4edda; color: #155724; }");
            m_statusLabel->setText(QString("自动保存 %1行x%2列 数据到 %3")
                .arg(m_rangeData->rowCount())
                .arg(m_rangeData->columnCount())
                .arg(m_sheetNameEdit->text()));
        }
    }

    // 自动保存数据的方法
    void autoSaveData()
    {
        QString filePath = m_filePathEdit->text().trimmed();
        QString sheetName = m_sheetNameEdit->text().trimmed();

        // 检查是否有必要的信息
        if (filePath.isEmpty() || sheetName.isEmpty()) {
            qDebug() << "SaveExcelModel: Cannot auto-save, missing path or sheet name";
            return;
        }

        // 自动保存
        saveDataToExcel(filePath, sheetName);
    }

    void saveDataToExcel(const QString& filePath, const QString& sheetName);

    // 实现IPropertyProvider接口
    bool createPropertyWidget(QVBoxLayout* parent, bool editable = false) override
    {
        addTitle(parent, "保存设置");
        addDescription(parent, "配置Excel文件保存参数，数据将自动保存");

        if (editable) {
            // 可编辑模式
            auto* pathEdit = addEditableLineEdit(parent, "保存路径:",
                m_filePathEdit->text(), "filePath", this);
            pathEdit->setPlaceholderText("输入保存路径或点击浏览...");

            // 添加浏览按钮
            auto* browseLayout = addHorizontalGroup(parent);
            auto* browseButton = new QPushButton("浏览...");
            browseLayout->addWidget(browseButton);
            browseLayout->addStretch();

            connect(browseButton, &QPushButton::clicked, [this, pathEdit]() {
                QString fileName = QFileDialog::getSaveFileName(
                    nullptr, "保存Excel文件", "", "Excel文件 (*.xlsx);;所有文件 (*)");
                if (!fileName.isEmpty()) {
                    if (!fileName.endsWith(".xlsx", Qt::CaseInsensitive)) {
                        fileName += ".xlsx";
                    }
                    pathEdit->setText(fileName);
                    onPropertyChanged("filePath", fileName);
                }
            });

            // Sheet名称编辑
            addEditableLineEdit(parent, "Sheet名称:", m_sheetNameEdit->text(), "sheetName", this);

        } else {
            // 只读模式
            QString filePath = m_filePathEdit->text();
            if (filePath.isEmpty()) {
                auto* pathLabel = new QLabel("未设置");
                pathLabel->setStyleSheet("color: #999; font-style: italic;");
                addLabeledWidget(parent, "保存路径:", pathLabel);
            } else {
                auto* pathLabel = new QLabel(filePath);
                pathLabel->setStyleSheet("color: #333;");
                pathLabel->setWordWrap(true);
                pathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
                addLabeledWidget(parent, "保存路径:", pathLabel);
            }

            // Sheet名称
            QString sheetName = m_sheetNameEdit->text();
            auto* sheetLabel = new QLabel(sheetName.isEmpty() ? "未设置" : sheetName);
            sheetLabel->setStyleSheet(sheetName.isEmpty() ? "color: #999; font-style: italic;" : "color: #333; font-weight: bold;");
            addLabeledWidget(parent, "Sheet名称:", sheetLabel);
        }

        // 数据信息
        if (m_rangeData && !m_rangeData->isEmpty()) {
            addSeparator(parent);
            addTitle(parent, "数据信息");

            auto* dataInfo = new QLabel(QString("数据大小: %1行 x %2列")
                .arg(m_rangeData->rowCount())
                .arg(m_rangeData->columnCount()));
            dataInfo->setStyleSheet("color: #666;");
            parent->addWidget(dataInfo);
        }

        // 状态信息
        addSeparator(parent);
        addTitle(parent, "当前状态");

        // 按钮状态
        auto* buttonStatus = new QLabel(QString("状态: %1").arg(m_saveButton->text()));
        QString buttonStyle = m_saveButton->styleSheet();
        if (buttonStyle.contains("#d4edda")) {
            buttonStatus->setStyleSheet("color: #155724; font-weight: bold;"); // 成功
        } else if (buttonStyle.contains("#f8d7da")) {
            buttonStatus->setStyleSheet("color: #721c24; font-weight: bold;"); // 失败
        } else if (buttonStyle.contains("#cce5ff")) {
            buttonStatus->setStyleSheet("color: #004085; font-weight: bold;"); // 进行中
        } else if (buttonStyle.contains("#fff3cd")) {
            buttonStatus->setStyleSheet("color: #856404; font-weight: bold;"); // 警告
        } else {
            buttonStatus->setStyleSheet("color: #666;"); // 默认
        }
        parent->addWidget(buttonStatus);

        // 详细状态信息
        auto* statusInfo = new QLabel(m_statusLabel->text());
        statusInfo->setStyleSheet("color: #666; font-size: 11px;");
        statusInfo->setWordWrap(true);
        parent->addWidget(statusInfo);

        return true;
    }

    QString getDisplayName() const override
    {
        return "保存Excel";
    }

    QString getDescription() const override
    {
        return "将数据自动保存到Excel文件";
    }

    // 新的属性面板实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle("保存设置");
        propertyWidget->addDescription("配置Excel文件保存参数，数据将自动保存");

        // 添加模式切换按钮
        propertyWidget->addModeToggleButtons();

        // 文件路径属性
        propertyWidget->addFilePathProperty("保存路径", m_filePathEdit->text(),
            "filePath", "Excel文件 (*.xlsx);;所有文件 (*)", true,
            [this](const QString& newPath) {
                if (!newPath.isEmpty()) {
                    m_filePathEdit->setText(newPath);
                    qDebug() << "SaveExcelModel: File path changed to" << newPath;
                }
            });

        // Sheet名称属性
        propertyWidget->addTextProperty("Sheet名称", m_sheetNameEdit->text(),
            "sheetName", "输入工作表名称",
            [this](const QString& newName) {
                if (!newName.isEmpty()) {
                    m_sheetNameEdit->setText(newName);
                    qDebug() << "SaveExcelModel: Sheet name changed to" << newName;
                }
            });

        // 数据信息
        if (m_rangeData && !m_rangeData->isEmpty()) {
            propertyWidget->addSeparator();
            propertyWidget->addTitle("数据信息");

            propertyWidget->addInfoProperty("数据大小",
                QString("%1行 x %2列").arg(m_rangeData->rowCount()).arg(m_rangeData->columnCount()),
                "color: #666;");
        }

        // 状态信息
        propertyWidget->addSeparator();
        propertyWidget->addTitle("当前状态");

        // 按钮状态解析
        QString statusText = m_saveButton->text();
        QString statusColor = "#666;";
        QString buttonStyle = m_saveButton->styleSheet();

        if (buttonStyle.contains("#d4edda")) {
            statusColor = "color: #155724; font-weight: bold;"; // 成功
        } else if (buttonStyle.contains("#f8d7da")) {
            statusColor = "color: #721c24; font-weight: bold;"; // 失败
        } else if (buttonStyle.contains("#cce5ff")) {
            statusColor = "color: #004085; font-weight: bold;"; // 进行中
        } else if (buttonStyle.contains("#fff3cd")) {
            statusColor = "color: #856404; font-weight: bold;"; // 警告
        }

        propertyWidget->addInfoProperty("状态", statusText, statusColor);
        propertyWidget->addInfoProperty("详细信息", m_statusLabel->text(), "color: #666; font-size: 11px;");

        return true;
    }

    void onPropertyChanged(const QString& propertyName, const QVariant& value) override
    {
        if (propertyName == "filePath") {
            QString newPath = value.toString();
            if (!newPath.isEmpty()) {
                m_filePathEdit->setText(newPath);
                qDebug() << "SaveExcelModel: File path changed to" << newPath;
            }
        } else if (propertyName == "sheetName") {
            QString newSheetName = value.toString();
            if (!newSheetName.isEmpty()) {
                m_sheetNameEdit->setText(newSheetName);
                qDebug() << "SaveExcelModel: Sheet name changed to" << newSheetName;
            }
        }
    }

private:
    QWidget* m_widget;
    QLineEdit* m_filePathEdit;
    QLineEdit* m_sheetNameEdit;
    QPushButton* m_browseButton;
    QPushButton* m_saveButton;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
    
    std::shared_ptr<RangeData> m_rangeData;
    std::shared_ptr<BooleanData> m_saveResult;
};
