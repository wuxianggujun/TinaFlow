//
// Created by wuxianggujun on 25-6-30.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "data/RangeData.hpp"
#include "data/BooleanData.hpp"

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
