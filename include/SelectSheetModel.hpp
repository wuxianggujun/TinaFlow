//
// Created by wuxianggujun on 25-6-27.
//

#pragma once


#include "BaseNodeModel.hpp"
#include "data/SheetData.hpp"
#include "data/WorkbookData.hpp"
#include "PropertyWidget.hpp"

#include <QComboBox>
#include <qgraphicsproxywidget.h>
#include <QHBoxLayout>

#include "QtNodes/internal/ConnectionGraphicsObject.hpp"

class SelectSheetModel : public BaseNodeModel
{
    Q_OBJECT

public:
    SelectSheetModel()
    {
        // 使用QComboBox，依赖Qt样式修复解决显示问题
        m_comboBox = new QComboBox();
        m_comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_comboBox->setMinimumWidth(150);

        connect(m_comboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SelectSheetModel::onIndexChanged);
    }

    QString caption() const override
    {
        return tr("选择工作表");
    }

    bool captionVisible() const override
    {
        return true;
    }

    QString name() const override
    {
        return tr("SelectSheet");
    }

    QWidget* embeddedWidget() override
    {
        return m_comboBox;
    }

    unsigned int nPorts(QtNodes::PortType portType) const override
    {
        return (portType == QtNodes::PortType::In) ? 1 : (portType == QtNodes::PortType::Out) ? 1 : 0;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In)
        {
            return WorkbookData().type();
        }
        return SheetData().type();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return m_sheetData;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << "SelectSheetModel::setInData called, portIndex:" << portIndex;

        if (!nodeData) {
            qDebug() << "SelectSheetModel: Received null nodeData";
            m_workbook.reset();
            m_dataAlreadyCreated = false; // 重置标记
            refreshCombo();
            return;
        }

        m_workbook = std::dynamic_pointer_cast<WorkbookData>(nodeData);
        if (m_workbook) {
            qDebug() << "SelectSheetModel: Successfully received WorkbookData";
            qDebug() << "SelectSheetModel: WorkbookData is valid:" << m_workbook->isValid();
            m_dataAlreadyCreated = false; // 重置标记，允许重新创建数据
        } else {
            qDebug() << "SelectSheetModel: Failed to cast to WorkbookData";
        }

        refreshCombo();
    }

    [[nodiscard]] QJsonObject save() const override
    {
        QJsonObject modelJson = NodeDelegateModel::save(); // 调用基类方法保存model-name
        modelJson["sheet"] = QString::fromStdString(m_selectedSheet);
        return modelJson;
    }

    void load(QJsonObject const& json) override
    {
        m_selectedSheet = json["sheet"].toString().toStdString();
    }



private:
    void onIndexChanged(int index)
    {
        qDebug() << "SelectSheetModel::onIndexChanged called, index:" << index;

        if (!m_workbook || index < 0)
        {
            qDebug() << "SelectSheetModel: No workbook or invalid index";
            m_sheetData.reset();
            return;
        }

        const QString sheetName = m_comboBox->itemText(index);
        qDebug() << "SelectSheetModel: Selected sheet:" << sheetName;

        try {
            // 确保使用UTF-8编码转换回std::string
            std::string sheetNameUtf8 = sheetName.toUtf8().toStdString();
            auto ws = m_workbook->workbook()->worksheet(sheetNameUtf8);
            m_selectedSheet = sheetNameUtf8;
            m_sheetData = std::make_shared<SheetData>(m_selectedSheet, ws);
            qDebug() << "SelectSheetModel: Created SheetData for:" << sheetName;
            emit dataUpdated(0);
        } catch (const std::exception& e) {
            qDebug() << "SelectSheetModel: Error creating worksheet:" << e.what();
        }
    }

    void refreshCombo()
    {
        qDebug() << "SelectSheetModel::refreshCombo called";

        m_comboBox->blockSignals(true);
        m_comboBox->clear();
        m_sheetData.reset();

        if (m_workbook && m_workbook->isValid())
        {
            try {
                const auto sheetNames = m_workbook->workbook()->sheetNames();
                qDebug() << "SelectSheetModel: sheet count" << sheetNames.size();

                for (const auto& name : sheetNames)
                {
                    // OpenXLSX使用UTF-8编码，确保正确转换为QString
                    QString sheetName = QString::fromUtf8(name.c_str());
                    qDebug() << "SelectSheetModel: Adding sheet:" << sheetName;
                    qDebug() << "SelectSheetModel: Raw name bytes:" << QByteArray(name.c_str(), name.length()).toHex();
                    m_comboBox->addItem(sheetName);
                }

                // 如果有之前选择的sheet，设置为当前选择
                if (!m_selectedSheet.empty())
                {
                    QString selectedSheetName = QString::fromUtf8(m_selectedSheet.c_str());
                    int index = m_comboBox->findText(selectedSheetName);
                    if (index != -1) {
                        m_comboBox->setCurrentIndex(index);
                        qDebug() << "SelectSheetModel: Restored selected sheet:" << selectedSheetName;
                    }
                }
                else if (sheetNames.size() > 0)
                {
                    // 如果没有之前的选择，自动选择第一个工作表
                    m_comboBox->setCurrentIndex(0);
                    qDebug() << "SelectSheetModel: Auto-selected first sheet";
                    // 手动触发选择事件
                    onIndexChanged(0);
                }

                // 重要：如果有恢复的选择，也要手动触发数据创建
                if (!m_selectedSheet.empty() && !m_dataAlreadyCreated) {
                    int currentIndex = m_comboBox->currentIndex();
                    if (currentIndex >= 0) {
                        qDebug() << "SelectSheetModel: Manually triggering data creation for restored sheet";
                        m_dataAlreadyCreated = true; // 标记已经创建过数据
                        onIndexChanged(currentIndex);
                    }
                }

                // 启用ComboBox
                m_comboBox->setEnabled(true);
                qDebug() << "SelectSheetModel: ComboBox enabled with" << m_comboBox->count() << "items";
            } catch (const std::exception& e) {
                qDebug() << "SelectSheetModel: Error in refreshCombo:" << e.what();
                m_comboBox->addItem("错误：无法读取工作表");
                m_comboBox->setEnabled(false);
            }
        }
        else
        {
            qDebug() << "SelectSheetModel: No valid workbook";
            m_comboBox->addItem("请选择工作表");
            m_comboBox->setEnabled(false);
        }

        m_comboBox->blockSignals(false);
    }

protected:
    // 实现BaseNodeModel的虚函数
    QString getNodeTypeName() const override
    {
        return "SelectSheetModel";
    }

    // 实现IPropertyProvider接口
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle("工作表选择");
        propertyWidget->addDescription("从Excel工作簿中选择要操作的工作表");

        // 工作表选择节点只需要显示信息，不需要编辑模式

        // 当前选择的工作表
        QString currentSheet = m_comboBox->currentText();
        if (currentSheet.isEmpty() || currentSheet == "请选择工作表") {
            currentSheet = "未选择";
        }

        propertyWidget->addInfoProperty("当前工作表", currentSheet,
            currentSheet == "未选择" ? "color: #999; font-style: italic;" : "color: #333; font-weight: bold;");

        // 可用工作表列表
        if (m_comboBox->count() > 0 && m_comboBox->isEnabled()) {
            propertyWidget->addSeparator();
            propertyWidget->addTitle("可用工作表");

            for (int i = 0; i < m_comboBox->count(); ++i) {
                QString sheetName = m_comboBox->itemText(i);
                QString style = (i == m_comboBox->currentIndex()) ?
                    "color: #007acc; font-weight: bold;" : "color: #666;";
                propertyWidget->addInfoProperty(QString("工作表 %1").arg(i + 1), sheetName, style);
            }
        }

        // 工作簿信息
        if (m_workbook && m_workbook->isValid()) {
            propertyWidget->addSeparator();
            propertyWidget->addTitle("工作簿信息");

            try {
                auto workbook = m_workbook->workbook();
                if (workbook) {
                    int totalSheets = workbook->worksheetCount();
                    propertyWidget->addInfoProperty("总工作表数", QString::number(totalSheets), "color: #666;");
                }
            } catch (...) {
                propertyWidget->addInfoProperty("工作簿状态", "读取失败", "color: #999;");
            }
        } else {
            propertyWidget->addSeparator();
            propertyWidget->addInfoProperty("工作簿状态", "未连接Excel文件", "color: #999; font-style: italic;");
        }

        return true;
    }

    QString getDisplayName() const override
    {
        return "选择工作表";
    }

    QString getDescription() const override
    {
        return "从Excel工作簿中选择要操作的工作表";
    }



private:
    QComboBox* m_comboBox;

    std::shared_ptr<WorkbookData> m_workbook;
    std::shared_ptr<SheetData> m_sheetData;
    std::string m_selectedSheet;
    bool m_dataAlreadyCreated = false;
};
