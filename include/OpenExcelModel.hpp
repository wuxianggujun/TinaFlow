//
// Created by wuxianggujun on 25-6-26.
//

#pragma once
#include <QtEvents>
#include <string>
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QMessageBox>
#include <QtNodes/NodeDelegateModel>

#include "XLDocument.hpp"
#include "data/WorkbookData.hpp"

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


class OpenExcelModel : public QtNodes::NodeDelegateModel
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
        m_lineEdit->setText(QString::fromStdString(m_filePath));
        compute();
    }

private:
    void compute()
    {
        if (m_filePath.empty())
        {
            m_workbookData = nullptr;
            return;
        }

        try
        {
            OpenXLSX::XLDocument* doc = new OpenXLSX::XLDocument();
            doc->open(m_filePath);

            OpenXLSX::XLWorkbook wb = doc->workbook();
            m_workbookData = std::make_shared<WorkbookData>(wb, doc);
            Q_EMIT dataUpdated(0);
        }
        catch (const std::exception& e)
        {
            m_workbookData = nullptr;
            QMessageBox::warning(
                nullptr,
                "打开文件失败",
                QString("错误信息: %1").arg(e.what())
            );
        }
    }
    
    void chooseFile()
    {
        const QString path = QFileDialog::getOpenFileName(nullptr, "打开 Excel File", {}, "Excel Files (*.xlsx)");
        if (path.isEmpty())
            return;
        m_filePath = path.toStdString();
        m_lineEdit->setToolTip(path);
        m_lineEdit->setText(QFileInfo(path).fileName());

        compute();
    }
    
    QWidget* m_widget;
    ClickableLineEdit* m_lineEdit;
    std::string m_filePath;
    std::shared_ptr<WorkbookData> m_workbookData;
};
