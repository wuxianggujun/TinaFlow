//
// Created by wuxianggujun on 25-7-3.
//

#include "model/BlockScriptModel.hpp"
#include "data/SheetData.hpp"
#include "data/ValueData.hpp"
#include "widget/PropertyWidget.hpp"
#include "widget/BlockProgrammingView.hpp"
#include "ErrorHandler.hpp"
#include <QDebug>
#include <QMessageBox>
#include <QJsonArray>

BlockScriptModel::BlockScriptModel()
    : m_scriptName("未命名脚本")
{
    qDebug() << "BlockScriptModel created";
}

unsigned int BlockScriptModel::nPorts(QtNodes::PortType const portType) const
{
    if (portType == QtNodes::PortType::In) {
        return 1; // 一个输入端口
    } else if (portType == QtNodes::PortType::Out) {
        return 1; // 一个输出端口
    }
    return 0;
}

QtNodes::NodeDataType BlockScriptModel::dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::In && portIndex == 0) {
        return SheetData().type(); // 输入工作表数据
    } else if (portType == QtNodes::PortType::Out && portIndex == 0) {
        return ValueData().type(); // 输出处理结果
    }
    return QtNodes::NodeDataType();
}

std::shared_ptr<QtNodes::NodeData> BlockScriptModel::outData(QtNodes::PortIndex const port)
{
    if (port == 0) {
        return m_outputData;
    }
    return nullptr;
}

void BlockScriptModel::setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex)
{
    if (portIndex == 0) {
        m_inputData = nodeData;
        
        // 更新状态显示
        if (m_statusLabel) {
            if (nodeData) {
                m_statusLabel->setText("已连接数据源");
                m_statusLabel->setStyleSheet("QLabel { color: green; }");
            } else {
                m_statusLabel->setText("未连接数据源");
                m_statusLabel->setStyleSheet("QLabel { color: red; }");
            }
        }
    }
}

QWidget* BlockScriptModel::embeddedWidget()
{
    if (!m_widget) {
        createEmbeddedWidget();
    }
    return m_widget;
}

void BlockScriptModel::createEmbeddedWidget()
{
    m_widget = new QWidget();
    m_widget->setMinimumSize(200, 100);
    
    // 创建主布局
    auto* layout = new QVBoxLayout(m_widget);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(5);
    
    // 脚本名称显示
    auto* nameLabel = new QLabel(QString("脚本: %1").arg(m_scriptName));
    nameLabel->setStyleSheet("QLabel { font-weight: bold; color: #333; }");
    layout->addWidget(nameLabel);
    
    // 状态显示
    m_statusLabel = new QLabel("未连接数据源");
    m_statusLabel->setStyleSheet("QLabel { color: red; font-size: 11px; }");
    layout->addWidget(m_statusLabel);
    
    // 按钮布局
    auto* buttonLayout = new QHBoxLayout();
    
    // 编辑按钮
    m_editButton = new QPushButton("编辑积木");
    m_editButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; border: none; padding: 5px 10px; border-radius: 3px; }");
    connect(m_editButton, &QPushButton::clicked, this, &BlockScriptModel::onEditBlockScript);
    
    // 执行按钮
    m_executeButton = new QPushButton("执行");
    m_executeButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border: none; padding: 5px 10px; border-radius: 3px; }");
    connect(m_executeButton, &QPushButton::clicked, this, &BlockScriptModel::onExecuteScript);
    
    buttonLayout->addWidget(m_editButton);
    buttonLayout->addWidget(m_executeButton);
    
    layout->addLayout(buttonLayout);
}

QJsonObject BlockScriptModel::save() const
{
    QJsonObject json = BaseNodeModel::save();
    
    // 保存脚本配置
    json["scriptName"] = m_scriptName;
    json["blockConfiguration"] = m_blockConfiguration;
    
    return json;
}

void BlockScriptModel::load(QJsonObject const& json)
{
    BaseNodeModel::load(json);
    
    // 加载脚本配置
    if (json.contains("scriptName")) {
        m_scriptName = json["scriptName"].toString();
    }
    
    if (json.contains("blockConfiguration")) {
        m_blockConfiguration = json["blockConfiguration"].toObject();
    }
}

bool BlockScriptModel::createPropertyPanel(PropertyWidget* propertyWidget)
{
    propertyWidget->addTitle("积木脚本节点设置");
    propertyWidget->addDescription("使用积木编程处理Excel数据");
    propertyWidget->addModeToggleButtons();
    
    // 脚本配置
    propertyWidget->addSeparator();

    propertyWidget->addTextProperty("脚本名称", m_scriptName, "scriptName",
        "为积木脚本指定一个名称", [this](const QString& name) {
            m_scriptName = name;
            // 更新UI显示
            if (m_widget) {
                // 重新创建widget以更新显示
                createEmbeddedWidget();
            }
        });

    // 积木配置信息
    propertyWidget->addSeparator();

    int blockCount = m_blockConfiguration.contains("blocks") ?
        m_blockConfiguration["blocks"].toArray().size() : 0;

    propertyWidget->addTextProperty("积木块数量", QString::number(blockCount), "blockCount",
        "当前脚本中的积木块数量（只读）");
    
    return true;
}

void BlockScriptModel::onEditBlockScript()
{
    openBlockEditor();
}

void BlockScriptModel::onExecuteScript()
{
    executeBlockScript();
}

void BlockScriptModel::openBlockEditor()
{
    // 如果已经有视图，重用它而不是创建新的
    if (m_blockProgrammingView) {
        // 更新现有视图的配置
        m_blockProgrammingView->setScriptName(m_scriptName);
        m_blockProgrammingView->setBlockConfiguration(m_blockConfiguration);

        // 显示并激活现有视图
        m_blockProgrammingView->show();
        m_blockProgrammingView->raise();
        m_blockProgrammingView->activateWindow();

        qDebug() << "BlockScriptModel: Reused existing block programming view for script:" << m_scriptName;
        return;
    }

    // 创建新的积木编程视图（仅在没有现有视图时）
    m_blockProgrammingView = new BlockProgrammingView();
    m_blockProgrammingView->setScriptName(m_scriptName);
    m_blockProgrammingView->setBlockConfiguration(m_blockConfiguration);

    // 连接信号
    connect(m_blockProgrammingView, &BlockProgrammingView::scriptSaved,
            this, &BlockScriptModel::onScriptSaved);
    connect(m_blockProgrammingView, &BlockProgrammingView::viewClosed,
            this, &BlockScriptModel::onBlockProgrammingViewClosed);

    // 显示视图
    m_blockProgrammingView->show();
    m_blockProgrammingView->raise();
    m_blockProgrammingView->activateWindow();

    qDebug() << "BlockScriptModel: Created new block programming view for script:" << m_scriptName;
}

void BlockScriptModel::executeBlockScript()
{
    if (!m_inputData) {
        QMessageBox::warning(m_widget, "执行错误", "请先连接数据源");
        return;
    }
    
    // 简化的执行逻辑 - 目前只是演示
    SAFE_EXECUTE({
        // 这里将来会执行积木脚本生成的逻辑
        // 目前只是创建一个示例输出
        
        auto sheetData = std::dynamic_pointer_cast<SheetData>(m_inputData);
        if (sheetData) {
            // 创建一个示例输出
            m_outputData = std::make_shared<ValueData>("积木脚本执行完成");
            
            qDebug() << "BlockScriptModel: Script executed successfully";
            emit dataUpdated(0);
            
            if (m_statusLabel) {
                m_statusLabel->setText("执行成功");
                m_statusLabel->setStyleSheet("QLabel { color: green; }");
            }
        } else {
            qWarning() << "BlockScriptModel: Invalid input data type";
            if (m_statusLabel) {
                m_statusLabel->setText("数据类型错误");
                m_statusLabel->setStyleSheet("QLabel { color: red; }");
            }
        }
        
    }, m_widget, "BlockScriptModel", "执行积木脚本");
}

void BlockScriptModel::onBlockProgrammingViewClosed()
{
    // 积木编程视图关闭时清理引用
    if (m_blockProgrammingView) {
        m_blockProgrammingView = nullptr;
    }
    qDebug() << "BlockScriptModel: Block programming view closed";
}

void BlockScriptModel::onScriptSaved(const QString& scriptName, const QJsonObject& configuration)
{
    // 保存脚本配置
    m_scriptName = scriptName;
    m_blockConfiguration = configuration;

    // 更新UI显示
    if (m_widget) {
        createEmbeddedWidget();
    }

    qDebug() << "BlockScriptModel: Script saved:" << scriptName;
}


