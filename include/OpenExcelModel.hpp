//
// Created by wuxianggujun on 25-6-26.
//

#pragma once
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

class OpenExcelModel : public QtNodes::NodeDelegateModel
{
public:
    OpenExcelModel(): m_button(new QPushButton("Open")), m_lineEdit(new QLineEdit())
    {
        QHBoxLayout* layout = new QHBoxLayout();
        layout->addWidget(m_lineEdit);
        layout->addWidget(m_button);

        m_widget = new QWidget();
        layout->addWidget(m_widget);

        connect(m_button, &QPushButton::clicked, [=]
        {
            QString path = QFileDialog::getOpenFileName(nullptr, "Open Excel File", "", "Excel Files (*.xlsx)");
            if (!path.isEmpty())
            {
                m_lineEdit->setText(path);
                m_filePath = path.toStdString();
                compute();
            }
        });

        connect(m_lineEdit, &QLineEdit::textChanged, [=](const QString& text)
        {
            m_filePath = text.toStdString();
            compute();
        });
    }

    QString caption() const override
    {
        return "打开Excel文件";
    }

    QString name() const override
    {
        return QString("OpenExcel");
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
            m_workbookData = std::make_shared<WorkbookData>(wb,doc);
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

    QJsonObject save() const override
    {
        QJsonObject modelJson;
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
    QWidget* m_widget;
    QPushButton* m_button;
    QLineEdit* m_lineEdit;
    std::string m_filePath;
    std::shared_ptr<WorkbookData> m_workbookData;
};
