//
// Created by wuxianggujun on 25-6-26.
//

#pragma once
#include "BaseNodeModel.hpp"
#include <QtEvents>
#include <string>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>

#include "XLDocument.hpp"
#include "data/WorkbookData.hpp"
#include "widget/PropertyWidget.hpp"
#include "ErrorHandler.hpp"
#include "DataValidator.hpp"
#include "PerformanceProfiler.hpp"

class ClickableLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit ClickableLineEdit(QWidget* parent = nullptr): QLineEdit(parent)
    {
        setReadOnly(true);
        setPlaceholderText("未选择Excel文件");
        setFrame(true);
        setProperty("class", "node-path");
    }

protected:
    void mousePressEvent(QMouseEvent* event) override
    {
        if (event->button() == Qt::LeftButton)
        {
            emit clicked();
        }
        else if (event->button() == Qt::RightButton)
        {
            clear();
        }
        QLineEdit::mousePressEvent(event);
    }

signals:
    void clicked();
};


class OpenExcelModel : public BaseNodeModel
{
    
public:
    OpenExcelModel()
    {
        m_widget = new QWidget();
        auto* layout = new QHBoxLayout(m_widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(0);

        m_lineEdit = new ClickableLineEdit();
        m_lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        layout->addWidget(m_lineEdit);

        connect(m_lineEdit, &ClickableLineEdit::clicked, this, &OpenExcelModel::chooseFile);

        // 注册属性
        registerLineEdit("filePath", m_lineEdit, "Excel文件路径");
    }

    [[nodiscard]] QString caption() const override
    {
        return "打开Excel文件";
    }

    [[nodiscard]] QString name() const override
    {
        return {"OpenExcel"};
    }


    QWidget* embeddedWidget() override
    {
        return m_widget;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return portType == QtNodes::PortType::Out ? 1 : 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        return WorkbookData().type();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return m_workbookData;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save(); // 调用基类方法保存model-name
        modelJson["file"] = QString::fromStdString(m_filePath);
        return modelJson;
    }

    void load(QJsonObject const& json) override
    {
        m_filePath = json["file"].toString().toStdString();
        // 只显示文件名，不显示完整路径
        QString fileName = QFileInfo(QString::fromStdString(m_filePath)).fileName();
        m_lineEdit->setText(fileName);

        // 加载时不自动执行，等待用户点击运行按钮
        qDebug() << "OpenExcelModel: File path loaded, waiting for execution trigger";
    }

public:
    // 添加公共方法供外部触发执行
    void triggerExecution()
    {
        qDebug() << "OpenExcelModel: Execution triggered";
        compute();
    }

private:
    bool shouldExecute() const
    {
        // 检查全局执行状态（需要包含MainWindow头文件）
        // 暂时返回true，后续可以添加更复杂的逻辑
        return true;
    }

    void compute()
    {
        PROFILE_NODE("OpenExcelModel");

        if (m_filePath.empty())
        {
            m_workbookData = nullptr;
            return;
        }

        if (!shouldExecute()) {
            qDebug() << "OpenExcelModel: Execution not allowed, skipping";
            return;
        }

        SAFE_EXECUTE({
            // 验证文件路径
            QString filePath = QString::fromStdString(m_filePath);
            auto validation = DataValidator::validateExcelFile(filePath);
            if (!validation.isValid) {
                throw TinaFlowException::fileNotFound(filePath);
            }

            // 尝试打开Excel文件
            auto* doc = new OpenXLSX::XLDocument();
            try {
                doc->open(m_filePath);
            } catch (const std::exception& openError) {
                delete doc;
                TINAFLOW_THROW(ExcelFileInvalid, QString("无法打开Excel文件: %1 - %2").arg(filePath).arg(openError.what()));
            }

            // 获取工作簿
            OpenXLSX::XLWorkbook wb = doc->workbook();
            // 检查工作簿是否有效（通过检查是否有工作表）
            if (wb.worksheetCount() == 0) {
                delete doc;
                TINAFLOW_THROW(ExcelFileInvalid, QString("Excel工作簿无效或为空: %1").arg(filePath));
            }

            m_workbookData = std::make_shared<WorkbookData>(wb, doc);
            Q_EMIT dataUpdated(0);

            qDebug() << "OpenExcelModel: Successfully opened Excel file:" << filePath;

        }, m_widget, "OpenExcelModel", "打开Excel文件");
    }
    
    void chooseFile()
    {
        SAFE_EXECUTE({
            const QString path = QFileDialog::getOpenFileName(nullptr, "打开 Excel File", {}, "Excel Files (*.xlsx *.xls)");
            if (path.isEmpty()) {
                return; // 用户取消选择
            }

            // 验证选择的文件
            auto validation = DataValidator::validateExcelFile(path);
            if (!validation.isValid) {
                SHOW_WARNING(validation.errorMessage, validation.suggestions.join("\n"), m_widget);
                return;
            }

            m_filePath = path.toStdString();
            m_lineEdit->setToolTip(path);
            m_lineEdit->setText(QFileInfo(path).fileName());

            compute();

        }, m_widget, "OpenExcelModel", "选择Excel文件");
    }

protected:
    // 实现BaseNodeModel的虚函数
    QString getNodeTypeName() const override
    {
        return "OpenExcelModel";
    }



    QString getDisplayName() const override
    {
        return "打开Excel文件";
    }

    QString getDescription() const override
    {
        return "打开Excel文件并读取工作簿数据";
    }

    // 新的属性面板实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle("Excel文件设置");
        propertyWidget->addDescription("选择要打开的Excel文件，支持.xlsx格式");

        // 添加模式切换按钮
        propertyWidget->addModeToggleButtons();

        // 文件路径属性
        propertyWidget->addFilePathProperty("文件路径", QString::fromStdString(m_filePath),
            "filePath", "Excel文件 (*.xlsx);;所有文件 (*)", false,
            [this](const QString& newPath) {
                if (!newPath.isEmpty() && newPath != QString::fromStdString(m_filePath)) {
                    m_filePath = newPath.toStdString();
                    m_lineEdit->setText(QFileInfo(newPath).fileName());
                    m_lineEdit->setToolTip(newPath);
                    // 不在属性面板创建时调用compute()，避免循环调用导致卡死
                    qDebug() << "OpenExcelModel: File path changed to" << newPath << "(no auto execution)";
                }
            });

        // 文件信息
        if (!m_filePath.empty() && m_workbookData && m_workbookData->isValid()) {
            propertyWidget->addSeparator();
            propertyWidget->addTitle("文件信息");

            try {
                auto workbook = m_workbookData->workbook();
                if (workbook) {
                    int sheetCount = workbook->worksheetCount();
                    propertyWidget->addInfoProperty("工作表数量", QString::number(sheetCount), "color: #666;");
                } else {
                    propertyWidget->addInfoProperty("工作表信息", "无法获取", "color: #999;");
                }
            } catch (...) {
                propertyWidget->addInfoProperty("文件信息", "读取失败", "color: #999;");
            }
        }

        return true;
    }

    void onPropertyChanged(const QString& propertyName, const QVariant& value) override
    {
        if (propertyName == "filePath") {
            QString newPath = value.toString();
            if (!newPath.isEmpty() && newPath != QString::fromStdString(m_filePath)) {
                m_filePath = newPath.toStdString();
                m_lineEdit->setText(QFileInfo(newPath).fileName());
                m_lineEdit->setToolTip(newPath);

                // 不自动执行compute()，避免在属性面板操作时导致意外的文件加载
                qDebug() << "OpenExcelModel: File path changed to" << newPath << "(no auto execution)";
            }
        }
    }

private:
    QWidget* m_widget;
    ClickableLineEdit* m_lineEdit;
    std::string m_filePath;
    std::shared_ptr<WorkbookData> m_workbookData;
};
