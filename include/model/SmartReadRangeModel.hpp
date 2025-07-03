//
// Created by wuxianggujun on 25-7-3.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "IPropertyProvider.hpp"
#include "data/SheetData.hpp"
#include "data/RangeData.hpp"
#include "widget/PropertyWidget.hpp"
#include <QComboBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * @brief 智能范围读取模式
 */
enum class ReadMode {
    ManualRange,        // 手动指定范围
    EntireSheet,        // 整张工作表
    UsedRange,          // 已使用的数据范围（自动检测）
    FromCell,           // 从指定单元格开始到数据末尾
    SpecificRows,       // 指定行范围
    SpecificColumns     // 指定列范围
};

/**
 * @brief 智能读取Excel范围数据的节点模型
 * 
 * 支持多种读取模式：
 * - 手动指定范围（如A1:C10）
 * - 读取整张工作表
 * - 自动检测数据范围
 * - 从指定位置读取到末尾
 * - 指定行/列范围
 */
class SmartReadRangeModel : public BaseNodeModel
{
    Q_OBJECT

public:
    SmartReadRangeModel();
    ~SmartReadRangeModel() override = default;

    // 基本节点信息
    QString caption() const override { return tr("智能范围读取"); }
    QString name() const override { return tr("SmartReadRange"); }

    // 端口配置
    unsigned int nPorts(QtNodes::PortType const portType) const override
    {
        return (portType == QtNodes::PortType::In) ? 1 : 1;
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In && portIndex == 0) {
            return SheetData().type();
        } else if (portType == QtNodes::PortType::Out && portIndex == 0) {
            return RangeData().type();
        }
        return QtNodes::NodeDataType();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        if (port == 0) {
            return m_rangeData;
        }
        return nullptr;
    }

    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        if (portIndex != 0) return;

        auto sheetData = std::dynamic_pointer_cast<SheetData>(nodeData);
        if (!sheetData) {
            m_sheetData.reset();
            m_rangeData.reset();
            emit dataUpdated(0);
            return;
        }

        m_sheetData = sheetData;
        processData();
    }

    // 嵌入式控件
    QWidget* embeddedWidget() override
    {
        if (!m_widget) {
            createEmbeddedWidget();
        }
        return m_widget;
    }

    // 保存和加载
    QJsonObject save() const override
    {
        QJsonObject json = BaseNodeModel::save();
        json["readMode"] = static_cast<int>(m_readMode);
        json["manualRange"] = m_manualRangeEdit->text();
        json["startCell"] = m_startCellEdit->text();
        json["rowStart"] = m_rowStartEdit->text();
        json["rowEnd"] = m_rowEndEdit->text();
        json["colStart"] = m_colStartEdit->text();
        json["colEnd"] = m_colEndEdit->text();
        return json;
    }

    void load(QJsonObject const& json) override
    {
        BaseNodeModel::load(json);
        
        if (json.contains("readMode")) {
            m_readMode = static_cast<ReadMode>(json["readMode"].toInt());
            if (m_modeCombo) {
                m_modeCombo->setCurrentIndex(static_cast<int>(m_readMode));
            }
        }
        
        if (json.contains("manualRange") && m_manualRangeEdit) {
            m_manualRangeEdit->setText(json["manualRange"].toString());
        }
        
        if (json.contains("startCell") && m_startCellEdit) {
            m_startCellEdit->setText(json["startCell"].toString());
        }
        
        // 加载其他参数...
        updateUIVisibility();
    }

    // IPropertyProvider 接口
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle("智能范围读取设置");
        propertyWidget->addDescription("支持多种方式读取Excel数据范围");
        propertyWidget->addModeToggleButtons();

        // 读取模式选择
        QStringList modeOptions = {
            "手动指定范围", "整张工作表", "自动检测范围", 
            "从指定单元格开始", "指定行范围", "指定列范围"
        };
        int currentMode = static_cast<int>(m_readMode);
        propertyWidget->addComboProperty("读取模式", modeOptions, currentMode, "readMode",
            [this](int index) {
                m_readMode = static_cast<ReadMode>(index);
                if (m_modeCombo) {
                    m_modeCombo->setCurrentIndex(index);
                }
                updateUIVisibility();
                processData();
            });

        // 根据模式显示不同的参数
        switch (m_readMode) {
            case ReadMode::ManualRange:
                propertyWidget->addTextProperty("范围地址", m_manualRangeEdit->text(), "manualRange", 
                    "如：A1:C10, B2:E20", [this](const QString& range) {
                        m_manualRangeEdit->setText(range);
                        processData();
                    });
                break;
                
            case ReadMode::FromCell:
                propertyWidget->addTextProperty("起始单元格", m_startCellEdit->text(), "startCell",
                    "如：A1, B2", [this](const QString& cell) {
                        m_startCellEdit->setText(cell);
                        processData();
                    });
                break;
                
            case ReadMode::SpecificRows:
                propertyWidget->addTextProperty("起始行", m_rowStartEdit->text(), "rowStart",
                    "起始行号", [this](const QString& row) {
                        m_rowStartEdit->setText(row);
                        processData();
                    });
                propertyWidget->addTextProperty("结束行", m_rowEndEdit->text(), "rowEnd",
                    "结束行号（空表示到最后）", [this](const QString& row) {
                        m_rowEndEdit->setText(row);
                        processData();
                    });
                break;
                
            case ReadMode::SpecificColumns:
                propertyWidget->addTextProperty("起始列", m_colStartEdit->text(), "colStart",
                    "起始列（如A, B）", [this](const QString& col) {
                        m_colStartEdit->setText(col);
                        processData();
                    });
                propertyWidget->addTextProperty("结束列", m_colEndEdit->text(), "colEnd",
                    "结束列（空表示到最后）", [this](const QString& col) {
                        m_colEndEdit->setText(col);
                        processData();
                    });
                break;
                
            default:
                // EntireSheet 和 UsedRange 不需要额外参数
                break;
        }

        return true;
    }

    QString getDisplayName() const override { return "智能范围读取"; }
    QString getDescription() const override { return "支持多种方式读取Excel数据范围"; }

protected:
    QString getNodeTypeName() const override { return "SmartReadRangeModel"; }

private slots:
    void onModeChanged(int index)
    {
        m_readMode = static_cast<ReadMode>(index);
        updateUIVisibility();
        processData();
    }

    void onParameterChanged()
    {
        processData();
    }

private:
    void createEmbeddedWidget()
    {
        m_widget = new QWidget();
        auto* layout = new QVBoxLayout(m_widget);
        layout->setContentsMargins(4, 4, 4, 4);
        layout->setSpacing(2);

        // 模式选择
        auto* modeLayout = new QHBoxLayout();
        modeLayout->addWidget(new QLabel("模式:"));
        
        m_modeCombo = new QComboBox();
        m_modeCombo->addItems({
            "手动范围", "整张表", "自动检测", "从单元格", "指定行", "指定列"
        });
        m_modeCombo->setCurrentIndex(static_cast<int>(m_readMode));
        connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &SmartReadRangeModel::onModeChanged);
        modeLayout->addWidget(m_modeCombo);
        layout->addLayout(modeLayout);

        // 参数输入区域
        m_parameterWidget = new QWidget();
        m_parameterLayout = new QVBoxLayout(m_parameterWidget);
        m_parameterLayout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(m_parameterWidget);

        // 创建所有输入控件
        createParameterControls();
        updateUIVisibility();
    }

    void createParameterControls()
    {
        // 手动范围输入
        m_manualRangeEdit = new QLineEdit();
        m_manualRangeEdit->setPlaceholderText("A1:C10");
        m_manualRangeEdit->setText("A1:C10");
        connect(m_manualRangeEdit, &QLineEdit::textChanged, this, &SmartReadRangeModel::onParameterChanged);

        // 起始单元格输入
        m_startCellEdit = new QLineEdit();
        m_startCellEdit->setPlaceholderText("A1");
        m_startCellEdit->setText("A1");
        connect(m_startCellEdit, &QLineEdit::textChanged, this, &SmartReadRangeModel::onParameterChanged);

        // 行范围输入
        m_rowStartEdit = new QLineEdit();
        m_rowStartEdit->setPlaceholderText("1");
        m_rowStartEdit->setText("1");
        connect(m_rowStartEdit, &QLineEdit::textChanged, this, &SmartReadRangeModel::onParameterChanged);

        m_rowEndEdit = new QLineEdit();
        m_rowEndEdit->setPlaceholderText("10");
        connect(m_rowEndEdit, &QLineEdit::textChanged, this, &SmartReadRangeModel::onParameterChanged);

        // 列范围输入
        m_colStartEdit = new QLineEdit();
        m_colStartEdit->setPlaceholderText("A");
        m_colStartEdit->setText("A");
        connect(m_colStartEdit, &QLineEdit::textChanged, this, &SmartReadRangeModel::onParameterChanged);

        m_colEndEdit = new QLineEdit();
        m_colEndEdit->setPlaceholderText("C");
        connect(m_colEndEdit, &QLineEdit::textChanged, this, &SmartReadRangeModel::onParameterChanged);
    }

    void updateUIVisibility()
    {
        // 清除当前布局
        QLayoutItem* item;
        while ((item = m_parameterLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->setParent(nullptr);
            }
            delete item;
        }

        // 根据模式添加相应的控件
        switch (m_readMode) {
            case ReadMode::ManualRange: {
                auto* layout = new QHBoxLayout();
                layout->addWidget(new QLabel("范围:"));
                layout->addWidget(m_manualRangeEdit);
                m_parameterLayout->addLayout(layout);
                break;
            }
            case ReadMode::FromCell: {
                auto* layout = new QHBoxLayout();
                layout->addWidget(new QLabel("起始:"));
                layout->addWidget(m_startCellEdit);
                m_parameterLayout->addLayout(layout);
                break;
            }
            case ReadMode::SpecificRows: {
                auto* startLayout = new QHBoxLayout();
                startLayout->addWidget(new QLabel("起始行:"));
                startLayout->addWidget(m_rowStartEdit);
                m_parameterLayout->addLayout(startLayout);
                
                auto* endLayout = new QHBoxLayout();
                endLayout->addWidget(new QLabel("结束行:"));
                endLayout->addWidget(m_rowEndEdit);
                m_parameterLayout->addLayout(endLayout);
                break;
            }
            case ReadMode::SpecificColumns: {
                auto* startLayout = new QHBoxLayout();
                startLayout->addWidget(new QLabel("起始列:"));
                startLayout->addWidget(m_colStartEdit);
                m_parameterLayout->addLayout(startLayout);
                
                auto* endLayout = new QHBoxLayout();
                endLayout->addWidget(new QLabel("结束列:"));
                endLayout->addWidget(m_colEndEdit);
                m_parameterLayout->addLayout(endLayout);
                break;
            }
            default:
                // EntireSheet 和 UsedRange 不需要参数
                m_parameterLayout->addWidget(new QLabel("无需额外参数"));
                break;
        }
    }

    void processData();

private:
    // UI 组件
    QWidget* m_widget = nullptr;
    QComboBox* m_modeCombo = nullptr;
    QWidget* m_parameterWidget = nullptr;
    QVBoxLayout* m_parameterLayout = nullptr;
    
    // 参数输入控件
    QLineEdit* m_manualRangeEdit = nullptr;
    QLineEdit* m_startCellEdit = nullptr;
    QLineEdit* m_rowStartEdit = nullptr;
    QLineEdit* m_rowEndEdit = nullptr;
    QLineEdit* m_colStartEdit = nullptr;
    QLineEdit* m_colEndEdit = nullptr;

    // 数据
    std::shared_ptr<SheetData> m_sheetData;
    std::shared_ptr<RangeData> m_rangeData;
    
    // 配置
    ReadMode m_readMode = ReadMode::UsedRange; // 默认自动检测
};
