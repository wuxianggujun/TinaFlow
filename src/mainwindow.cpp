#include "mainwindow.hpp"
#include "ui_mainwindow.h"

// Qt Core
#include <QApplication>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QStandardPaths>
#include <QSettings>
#include <QShortcut>
#include <QInputDialog>
#include <QDateTime>
#include <QTimer>
#include <QElapsedTimer>
#include <limits>

// Qt Widgets
#include <QMenu>
#include <QMenuBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>

// QtNodes
#include <QtNodes/DataFlowGraphicsScene>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/ConnectionStyle>
#include <QtNodes/NodeStyle>
#include <QtNodes/internal/NodeGraphicsObject.hpp>

// Model includes
#include "model/OpenExcelModel.hpp"
#include "model/SelectSheetModel.hpp"
#include "model/ReadCellModel.hpp"
#include "model/DisplayCellModel.hpp"
#include "model/ReadRangeModel.hpp"
#include "model/DisplayRangeModel.hpp"
#include "model/StringCompareModel.hpp"
#include "model/DisplayBooleanModel.hpp"
#include "model/SmartLoopProcessorModel.hpp"
#include "model/DisplayCellListModel.hpp"
#include "model/DisplayRowModel.hpp"
#include "model/RangeInfoModel.hpp"
#include "model/SaveExcelModel.hpp"

// 核心节点模型
#include "model/OpenExcelModel.hpp"
#include "model/SelectSheetModel.hpp"
#include "model/ReadCellModel.hpp"
#include "model/ReadRangeModel.hpp"
#include "model/SaveExcelModel.hpp"
#include "model/SmartLoopProcessorModel.hpp"
#include "model/StringCompareModel.hpp"

// 显示节点模型
#include "model/DisplayCellModel.hpp"
#include "model/DisplayRangeModel.hpp"
#include "model/DisplayBooleanModel.hpp"
#include "model/DisplayRowModel.hpp"
#include "model/DisplayCellListModel.hpp"
#include "model/RangeInfoModel.hpp"

// TinaFlow Components
#include "CommandManager.hpp"
#include "NodeCatalog.hpp"
#include "NodePalette.hpp"
#include "NodeCommands.hpp"
#include "ErrorHandler.hpp"
#include "DataValidator.hpp"
#include "IPropertyProvider.hpp"
#include "widget/PropertyWidget.hpp"
#include "widget/ModernToolBar.hpp"
#include "widget/ADSPanelManager.hpp"
#include "widget/ADSPropertyPanel.hpp"

// 静态成员变量定义
bool MainWindow::s_globalExecutionEnabled = false;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
      , m_modernToolBar(nullptr)
      , m_adsPanelManager(nullptr)
      , m_selectedNodeId(QtNodes::NodeId{})
      , m_selectedConnectionId(QtNodes::ConnectionId{})
      , m_autoSaveTimer(new QTimer(this))
      , m_hasUnsavedChanges(false)
      , m_nodeCountLabel(new QLabel(this))
      , m_connectionCountLabel(new QLabel(this))
      , m_statusLabel(new QLabel(this))
{
    ui->setupUi(this);

    setupNodeEditor();
    setupModernToolbar();
    setupKeyboardShortcuts();

    // 先设置窗口属性，但不显示
    setMinimumSize(Constants::MIN_WINDOW_WIDTH, Constants::MIN_WINDOW_HEIGHT);

    // 然后初始化ADS系统
    setupAdvancedPanels();
    setupLayoutMenu(); // 只调用一次，包含ADS和通用菜单

    // 设置自动保存
    setupAutoSave();

    // 设置状态栏
    setupStatusBar();

    // 最后显示窗口
    setupWindowDisplay();
}

MainWindow::~MainWindow()
{


    // 清理ADS面板管理器（必须在UI清理之前）
    if (m_adsPanelManager)
    {
        m_adsPanelManager->disconnect();
        m_adsPanelManager->shutdown();
        delete m_adsPanelManager; // 直接删除，不用deleteLater
        m_adsPanelManager = nullptr;
    }

    // 自动保存窗口布局
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    delete ui;


}

void MainWindow::setupNodeEditor()
{
    // 注册自定义节点
    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> modelRegistry = registerDataModels();

    m_graphModel = std::make_unique<QtNodes::DataFlowGraphModel>(modelRegistry);
    m_graphicsScene = new QtNodes::DataFlowGraphicsScene(*m_graphModel, this);
    m_graphicsView = new TinaFlowGraphicsView(m_graphicsScene, this);

    // 应用自定义样式
    setupCustomStyles();

    // 重新启用节点选择事件，使用队列连接避免循环调用
    connect(m_graphicsScene, &QtNodes::DataFlowGraphicsScene::nodeSelected,
            this, &MainWindow::onNodeSelected, Qt::QueuedConnection);
    connect(m_graphicsScene, &QtNodes::DataFlowGraphicsScene::nodeClicked,
            this, &MainWindow::onNodeSelected, Qt::QueuedConnection);

    // 连接右键菜单事件
    connect(m_graphicsView, &TinaFlowGraphicsView::nodeContextMenuRequested,
            this, &MainWindow::showNodeContextMenu);
    connect(m_graphicsView, &TinaFlowGraphicsView::connectionContextMenuRequested,
            this, &MainWindow::showConnectionContextMenu);
    connect(m_graphicsView, &TinaFlowGraphicsView::sceneContextMenuRequested,
            this, &MainWindow::showImprovedSceneContextMenu);

    // 连接拖拽事件
    connect(m_graphicsView, &TinaFlowGraphicsView::nodeCreationFromDragRequested,
            this, &MainWindow::createNodeFromPalette);

    // 重新启用数据更新事件，使用队列连接并添加防护逻辑
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::inPortDataWasSet,
            this, [this](QtNodes::NodeId nodeId, QtNodes::PortType, QtNodes::PortIndex)
            {
                // 如果更新的节点是当前选中的节点，延迟刷新属性面板
                if (nodeId == m_selectedNodeId)
                {
                    QMetaObject::invokeMethod(this, [this, nodeId]()
                    {
                        // 再次检查节点是否仍然选中，避免无效刷新
                        if (nodeId == m_selectedNodeId)
                        {
                            updateADSPropertyPanel(nodeId);
                        }
                    }, Qt::QueuedConnection);
                }
            }, Qt::QueuedConnection);

    // 只在必要时更新属性面板 - 事件驱动而非定时器驱动
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeUpdated,
            this, [this](QtNodes::NodeId nodeId)
            {
                // 只有当节点是选中状态且是显示类型节点时才更新属性面板
                if (nodeId == m_selectedNodeId && m_selectedNodeId != QtNodes::NodeId{})
                {
                    auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
                    if (nodeDelegate)
                    {
                        QString nodeName = nodeDelegate->name();
                        // 只有显示类型的节点需要实时更新（因为它们显示计算结果）
                        if (nodeName.startsWith("Display") || nodeName.contains("Info"))
                        {
                            updateADSPropertyPanel(nodeId);
                        }
                    }
                }
            }, Qt::QueuedConnection);

    // 核心视图将直接由ADS系统管理，不再添加到传统容器
    // m_graphicsView会在setupADSCentralWidget()中被设置为ADS中央部件
}

void MainWindow::setupNodeUpdateConnections()
{
    if (!m_graphModel) return;

    // 事件驱动的属性面板更新 - 只在真正需要时更新
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeUpdated,
            this, [this](QtNodes::NodeId nodeId)
            {
                // 只有当节点是选中状态且是显示类型节点时才更新属性面板
                if (nodeId == m_selectedNodeId && m_selectedNodeId != QtNodes::NodeId{})
                {
                    auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
                    if (nodeDelegate)
                    {
                        QString nodeName = nodeDelegate->name();
                        // 只有显示类型的节点需要实时更新（因为它们显示计算结果）
                        if (nodeName.startsWith("Display") || nodeName.contains("Info"))
                        {
                            updateADSPropertyPanel(nodeId);
                        }
                    }
                }
            }, Qt::QueuedConnection);
}

void MainWindow::reinitializeNodeEditor()
{
    // 安全地清理旧的组件
    cleanupGraphicsComponents();

    // 重新创建所有组件
    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> modelRegistry = registerDataModels();
    m_graphModel = std::make_unique<QtNodes::DataFlowGraphModel>(modelRegistry);
    m_graphicsScene = new QtNodes::DataFlowGraphicsScene(*m_graphModel, this);
    m_graphicsView = new TinaFlowGraphicsView(m_graphicsScene, this);

    // 应用自定义样式
    setupCustomStyles();

    // 重新连接节点选择事件
    connect(m_graphicsScene, &QtNodes::DataFlowGraphicsScene::nodeSelected,
            this, &MainWindow::onNodeSelected, Qt::QueuedConnection);
    connect(m_graphicsScene, &QtNodes::DataFlowGraphicsScene::nodeClicked,
            this, &MainWindow::onNodeSelected, Qt::QueuedConnection);

    // 重新连接右键菜单事件
    connect(m_graphicsView, &TinaFlowGraphicsView::nodeContextMenuRequested,
            this, &MainWindow::showNodeContextMenu);
    connect(m_graphicsView, &TinaFlowGraphicsView::connectionContextMenuRequested,
            this, &MainWindow::showConnectionContextMenu);
    connect(m_graphicsView, &TinaFlowGraphicsView::sceneContextMenuRequested,
            this, &MainWindow::showImprovedSceneContextMenu);

    // 重新连接拖拽事件
    connect(m_graphicsView, &TinaFlowGraphicsView::nodeCreationFromDragRequested,
            this, &MainWindow::createNodeFromPalette);

    // 重新连接数据更新事件
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::inPortDataWasSet,
            this, [this](QtNodes::NodeId nodeId, QtNodes::PortType, QtNodes::PortIndex)
            {
                if (nodeId == m_selectedNodeId)
                {
                    QMetaObject::invokeMethod(this, [this, nodeId]()
                    {
                        if (nodeId == m_selectedNodeId)
                        {
                            updateADSPropertyPanel(nodeId);
                        }
                    }, Qt::QueuedConnection);
                }
            }, Qt::QueuedConnection);

    // 使用统一的节点更新连接方法
    setupNodeUpdateConnections();

    // 直接重新设置ADS中央部件
    setupADSCentralWidget();

    // 更新属性面板容器的图形模型
    updatePropertyPanelReference();

    // 节点编辑器已重新初始化
}

void MainWindow::cleanupGraphicsComponents()
{
    // 安全地删除图形组件
    if (m_graphicsView)
    {
        m_graphicsView->setParent(nullptr);
        m_graphicsView->deleteLater();
        m_graphicsView = nullptr;
    }

    if (m_graphicsScene)
    {
        m_graphicsScene->deleteLater();
        m_graphicsScene = nullptr;
    }
}

std::shared_ptr<QtNodes::NodeDelegateModelRegistry> MainWindow::registerDataModels()
{
    auto ret = std::make_shared<QtNodes::NodeDelegateModelRegistry>();
    // 使用英文名称注册（与节点的name()方法返回值一致）
    ret->registerModel<OpenExcelModel>("OpenExcel");
    ret->registerModel<SelectSheetModel>("SelectSheet");
    ret->registerModel<ReadCellModel>("ReadCell");
    ret->registerModel<DisplayCellModel>("DisplayCell");
    ret->registerModel<ReadRangeModel>("ReadRange");
    ret->registerModel<DisplayRangeModel>("DisplayRange");
    ret->registerModel<StringCompareModel>("StringCompare");
    ret->registerModel<DisplayBooleanModel>("DisplayBoolean");

    ret->registerModel<SmartLoopProcessorModel>("SmartLoopProcessor");
    ret->registerModel<DisplayCellListModel>("DisplayCellList");
    ret->registerModel<DisplayRowModel>("DisplayRow");
    ret->registerModel<RangeInfoModel>("RangeInfo");
    ret->registerModel<SaveExcelModel>("SaveExcel");
    return ret;
}

void MainWindow::setupModernToolbar()
{
    // 创建现代化工具栏（不显示文件操作，因为已移到菜单）
    m_modernToolBar = new ModernToolBar(this, false);

    // 创建一个容器工具栏来实现居中效果
    QToolBar* containerToolBar = addToolBar("主工具栏");
    containerToolBar->setMovable(false);
    containerToolBar->setFloatable(false);

    // 创建布局来居中显示工具栏
    QWidget* centralWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(centralWidget);
    layout->addStretch(); // 左侧弹性空间
    layout->addWidget(m_modernToolBar);
    layout->addStretch(); // 右侧弹性空间
    layout->setContentsMargins(0, 0, 0, 0);

    containerToolBar->addWidget(centralWidget);

    // 移除文件操作信号连接（现在由菜单处理）
    // connect(m_modernToolBar, &ModernToolBar::newFileRequested, this, &MainWindow::onNewFile);
    // connect(m_modernToolBar, &ModernToolBar::openFileRequested, this, &MainWindow::onOpenFile);
    // connect(m_modernToolBar, &ModernToolBar::saveFileRequested, this, &MainWindow::onSaveFile);

    // 连接编辑操作信号
    connect(m_modernToolBar, &ModernToolBar::undoRequested, this, &MainWindow::onUndoClicked);
    connect(m_modernToolBar, &ModernToolBar::redoRequested, this, &MainWindow::onRedoClicked);

    // 连接执行控制信号
    connect(m_modernToolBar, &ModernToolBar::runRequested, this, &MainWindow::onRunClicked);
    connect(m_modernToolBar, &ModernToolBar::pauseRequested, this, &MainWindow::onPauseClicked);
    connect(m_modernToolBar, &ModernToolBar::stopRequested, this, &MainWindow::onStopClicked);


    // 连接视图控制信号
    connect(m_modernToolBar, &ModernToolBar::zoomFitRequested, this, [this]()
    {
        if (m_graphicsView)
        {
            m_graphicsView->fitInView(m_graphicsScene->itemsBoundingRect(), Qt::KeepAspectRatio);
            ui->statusbar->showMessage(tr("视图已适应窗口"), 1000);
        }
    });
    connect(m_modernToolBar, &ModernToolBar::zoomInRequested, this, [this]()
    {
        if (m_graphicsView)
        {
            // 获取当前变换矩阵
            QTransform transform = m_graphicsView->transform();
            double currentScale = transform.m11(); // 获取X轴缩放比例

            // 设置最大缩放比例为5.0（500%）
            const double maxScale = 5.0;
            const double zoomFactor = 1.2;

            if (currentScale * zoomFactor <= maxScale)
            {
                m_graphicsView->scale(zoomFactor, zoomFactor);
                double newScale = currentScale * zoomFactor;
                ui->statusbar->showMessage(tr("缩放: %1%").arg(qRound(newScale * 100)), 1000);
            }
            else
            {
                ui->statusbar->showMessage(tr("已达到最大缩放比例 (500%)"), 2000);
            }
        }
    });
    connect(m_modernToolBar, &ModernToolBar::zoomOutRequested, this, [this]()
    {
        if (m_graphicsView)
        {
            // 获取当前变换矩阵
            QTransform transform = m_graphicsView->transform();
            double currentScale = transform.m11(); // 获取X轴缩放比例

            // 设置最小缩放比例为0.1（10%）
            const double minScale = 0.1;
            const double zoomFactor = 0.8;

            if (currentScale * zoomFactor >= minScale)
            {
                m_graphicsView->scale(zoomFactor, zoomFactor);
                double newScale = currentScale * zoomFactor;
                ui->statusbar->showMessage(tr("缩放: %1%").arg(qRound(newScale * 100)), 1000);
            }
            else
            {
                ui->statusbar->showMessage(tr("已达到最小缩放比例 (10%)"), 2000);
            }
        }
    });


    // 连接命令管理器信号到工具栏（使用统一信号避免重入问题）
    auto& commandManager = CommandManager::instance();
    connect(&commandManager, &CommandManager::undoRedoStateChanged, [this](bool canUndo, bool canRedo)
    {
        if (m_modernToolBar)
        {
            m_modernToolBar->updateUndoRedoState(canUndo, canRedo);
        }
    });

    // 初始化状态
    m_modernToolBar->updateExecutionState(false);
    m_modernToolBar->updateUndoRedoState(false, false);
}

void MainWindow::onNewFile()
{
    // 完全重新初始化节点编辑器以重置ID计数器
    reinitializeNodeEditor();

    // 清空命令历史 - 新文件不应该有撤销重做历史
    auto& commandManager = CommandManager::instance();
    commandManager.clear();

    // 清空属性面板 - 没有选中的节点
    clearADSPropertyPanel();
    m_selectedNodeId = QtNodes::NodeId{};

    // 重置视图缩放
    if (m_graphicsView)
    {
        m_graphicsView->resetTransform();
    }

    setWindowTitle("TinaFlow - 新建");
    ui->statusbar->showMessage(tr("新建流程，拖拽节点开始设计 (节点ID已重置)"), 3000);

    // 新文件已创建
}

void MainWindow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("打开流程文件"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        Constants::FILE_FILTER
    );

    if (!fileName.isEmpty())
    {
        loadFromFile(fileName);
    }
}

void MainWindow::onSaveFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("保存流程文件"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        Constants::FILE_FILTER
    );

    if (!fileName.isEmpty())
    {
        saveToFile(fileName);
    }
}

bool MainWindow::saveToFile(const QString& fileName)
{
    if (!m_graphModel) {
        handleFileError("保存", fileName, "没有可保存的数据");
        return false;
    }

    try {
        // 创建包含元数据的完整文档
        QJsonObject documentJson;

        // 添加文件元数据
        QJsonObject metadata;
        metadata["version"] = "1.0";
        metadata["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        metadata["application"] = "TinaFlow";
        metadata["nodeCount"] = static_cast<int>(m_graphModel->allNodeIds().size());
        metadata["connectionCount"] = getTotalConnectionCount();

        documentJson["metadata"] = metadata;
        documentJson["workflow"] = m_graphModel->save();

        QJsonDocument jsonDocument(documentJson);

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            handleFileError("保存", fileName, "无法打开文件进行写入");
            return false;
        }

        file.write(jsonDocument.toJson());
        file.close();

        // 更新当前文件路径和保存状态
        m_currentFilePath = fileName;
        m_hasUnsavedChanges = false;

        setWindowTitle(QString("TinaFlow - %1").arg(QFileInfo(fileName).baseName()));
        ui->statusbar->showMessage(tr("文件已保存: %1 (%2个节点, %3个连接)")
            .arg(fileName)
            .arg(m_graphModel->allNodeIds().size())
            .arg(getTotalConnectionCount()), 3000);
        return true;
    }
    catch (const std::exception& e) {
        handleFileError("保存", fileName, e.what());
        return false;
    }
}

void MainWindow::handleFileError(const QString& operation, const QString& fileName, const QString& error)
{
    QString message = tr("%1文件时发生错误: %2\n文件: %3").arg(operation, error, fileName);
    QMessageBox::critical(this, tr("文件操作错误"), message);
}

bool MainWindow::loadFromFile(const QString& fileName)
{
    if (!m_graphModel) {
        handleFileError("加载", fileName, "图形模型未初始化");
        return false;
    }

    try {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            handleFileError("加载", fileName, "无法打开文件进行读取");
            return false;
        }

        QByteArray data = file.readAll();
        file.close();

        QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
        if (jsonDocument.isNull()) {
            handleFileError("加载", fileName, "文件格式无效");
            return false;
        }

        // 重新初始化节点编辑器以重置ID计数器
        reinitializeNodeEditor();

        QJsonObject rootObject = jsonDocument.object();
        QJsonObject workflowData;

        // 检查是否是新格式（包含元数据）
        if (rootObject.contains("metadata") && rootObject.contains("workflow")) {
            // 新格式：包含元数据
            QJsonObject metadata = rootObject["metadata"].toObject();
            workflowData = rootObject["workflow"].toObject();

            // 显示文件信息
            QString version = metadata["version"].toString();
            QString created = metadata["created"].toString();
            int nodeCount = metadata["nodeCount"].toInt();
            int connectionCount = metadata["connectionCount"].toInt();

            qDebug() << "Loading TinaFlow file version:" << version
                     << "created:" << created
                     << "nodes:" << nodeCount
                     << "connections:" << connectionCount;
        } else {
            // 旧格式：直接是工作流数据
            workflowData = rootObject;
        }

        // 加载工作流数据
        m_graphModel->load(workflowData);

        // 更新当前文件路径
        m_currentFilePath = fileName;
        m_hasUnsavedChanges = false;

        // 重置视图缩放并适应内容
        if (m_graphicsView) {
            m_graphicsView->resetTransform();
            // 延迟适应视图，确保节点已完全加载
            QMetaObject::invokeMethod(this, [this]() {
                if (m_graphicsView && m_graphicsScene) {
                    m_graphicsView->fitInView(m_graphicsScene->itemsBoundingRect(), Qt::KeepAspectRatio);
                }
            }, Qt::QueuedConnection);
        }

        setWindowTitle(QString("%1 - %2").arg(Constants::WINDOW_TITLE_PREFIX, QFileInfo(fileName).baseName()));
        ui->statusbar->showMessage(tr("流程已加载，点击运行按钮(F5)开始执行"), 0);
        return true;
    }
    catch (const std::exception& e) {
        handleFileError("加载", fileName, e.what());
        return false;
    }
}

void MainWindow::onRunClicked()
{
    // 运行按钮被点击
    setGlobalExecutionState(true);

    // 更新现代化工具栏状态
    if (m_modernToolBar)
    {
        m_modernToolBar->updateExecutionState(true);
    }

    // 重新触发数据流处理
    triggerDataFlow();

    ui->statusbar->showMessage(tr("流程正在运行..."), 0);
}

void MainWindow::onPauseClicked()
{
    // 暂停按钮被点击
    setGlobalExecutionState(false);

    // 更新现代化工具栏状态
    if (m_modernToolBar)
    {
        m_modernToolBar->updateExecutionState(false);
    }

    ui->statusbar->showMessage(tr("流程已暂停"), 3000);
}

void MainWindow::onStopClicked()
{
    // 停止按钮被点击
    setGlobalExecutionState(false);

    // 更新现代化工具栏状态
    if (m_modernToolBar)
    {
        m_modernToolBar->updateExecutionState(false);
    }

    ui->statusbar->showMessage(tr("流程已停止"), 3000);
}

void MainWindow::onUndoClicked()
{
    // 撤销操作
    auto& commandManager = CommandManager::instance();
    if (commandManager.canUndo())
    {
        if (commandManager.undo())
        {
            ui->statusbar->showMessage(tr("已撤销: %1").arg(commandManager.getUndoText()), 2000);
        }
        else
        {
            ui->statusbar->showMessage(tr("撤销失败"), 2000);
        }
    }
}

void MainWindow::onRedoClicked()
{
    // 重做操作
    auto& commandManager = CommandManager::instance();
    if (commandManager.canRedo())
    {
        if (commandManager.redo())
        {
            ui->statusbar->showMessage(tr("已重做: %1").arg(commandManager.getRedoText()), 2000);
        }
        else
        {
            ui->statusbar->showMessage(tr("重做失败"), 2000);
        }
    }
}


void MainWindow::setGlobalExecutionState(bool running)
{
    s_globalExecutionEnabled = running;
    // 全局执行状态已设置

    // 这里可以添加通知所有节点状态变化的逻辑
    // 例如通过信号通知所有节点更新执行状态
}

void MainWindow::onNodeSelected(QtNodes::NodeId nodeId)
{
    m_selectedNodeId = nodeId;

    // 获取节点信息用于状态栏显示
    if (m_graphModel && nodeId != QtNodes::NodeId{}) {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (nodeDelegate) {
            ui->statusbar->showMessage(tr("已选择节点: %1 (按Delete键删除)").arg(nodeDelegate->name()), 5000);
        }
    }

    updateADSPropertyPanel(nodeId);
}

void MainWindow::showNodeContextMenu(QtNodes::NodeId nodeId, const QPointF& pos)
{
    m_selectedNodeId = nodeId;

    QMenu contextMenu(this);
    contextMenu.setStyleSheet(
        "QMenu {"
        "background-color: white;"
        "border: 1px solid #ccc;"
        "border-radius: 4px;"
        "padding: 4px;"
        "}"
        "QMenu::item {"
        "padding: 8px 24px;"
        "border: none;"
        "}"
        "QMenu::item:selected {"
        "background-color: #e3f2fd;"
        "color: #1976d2;"
        "}"
        "QMenu::separator {"
        "height: 1px;"
        "background-color: #eee;"
        "margin: 4px 8px;"
        "}"
    );

    // 获取节点信息
    auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
    QString nodeName = nodeDelegate ? nodeDelegate->name() : "未知节点";

    // 节点信息（只读）
    QAction* infoAction = contextMenu.addAction(QString("📋 节点: %1").arg(nodeName));
    infoAction->setEnabled(false);

    contextMenu.addSeparator();

    // 删除节点
    QAction* deleteAction = contextMenu.addAction("🗑️ 删除节点");
    deleteAction->setShortcut(QKeySequence::Delete);
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedNode);

    // 复制节点
    QAction* duplicateAction = contextMenu.addAction("📋 复制节点");
    duplicateAction->setShortcut(QKeySequence("Ctrl+D"));
    connect(duplicateAction, &QAction::triggered, this, &MainWindow::duplicateSelectedNode);

    contextMenu.addSeparator();

    // 属性
    QAction* propertiesAction = contextMenu.addAction("⚙️ 节点属性");
    connect(propertiesAction, &QAction::triggered, this, [this, nodeId]() {
        // 确保属性面板显示该节点
        updateADSPropertyPanel(nodeId);
        if (m_adsPanelManager) {
            m_adsPanelManager->showPanel("property_panel");
        }
    });

    // 转换坐标并显示菜单
    QPoint globalPos = m_graphicsView->mapToGlobal(m_graphicsView->mapFromScene(pos));

    // 确保菜单在屏幕范围内
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    if (!screenGeometry.contains(globalPos)) {
        globalPos = QCursor::pos(); // 使用鼠标当前位置作为备选
    }

    contextMenu.exec(globalPos);
}

void MainWindow::showConnectionContextMenu(QtNodes::ConnectionId connectionId, const QPointF& pos)
{
    m_selectedConnectionId = connectionId;

    QMenu contextMenu(this);

    // 获取连接信息
    auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.outNodeId);
    auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.inNodeId);

    if (outNodeDelegate && inNodeDelegate)
    {
        QString outPortType =
            getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out, connectionId.outPortIndex);
        QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In, connectionId.inPortIndex);

        QString description = QString("%1[%2:%3] → %4[%5:%6]")
                              .arg(outNodeDelegate->name())
                              .arg(connectionId.outPortIndex)
                              .arg(outPortType)
                              .arg(inNodeDelegate->name())
                              .arg(connectionId.inPortIndex)
                              .arg(inPortType);

        QAction* infoAction = contextMenu.addAction(QString("连接: %1").arg(description));
        infoAction->setEnabled(false);
        contextMenu.addSeparator();
    }

    QAction* deleteAction = contextMenu.addAction("删除连接");
    deleteAction->setIcon(QIcon(":/icons/disconnect.png"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedConnection);

    // 转换坐标并显示菜单
    QPoint globalPos = m_graphicsView->mapToGlobal(m_graphicsView->mapFromScene(pos));
    contextMenu.exec(globalPos);
}

void MainWindow::deleteSelectedNode()
{
    // 检查节点是否存在于图模型中
    bool nodeExists = m_graphModel && m_graphModel->allNodeIds().contains(m_selectedNodeId);

    if (nodeExists)
    {
        // 获取节点信息用于反馈
        QString nodeInfo = "未知节点";
        if (m_graphModel) {
            auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedNodeId);
            if (nodeDelegate) {
                nodeInfo = nodeDelegate->name();
            }
        }

        // 使用命令系统删除节点
        auto command = std::make_unique<DeleteNodeCommand>(m_graphicsScene, m_selectedNodeId);
        auto& commandManager = CommandManager::instance();

        if (commandManager.executeCommand(std::move(command)))
        {
            m_selectedNodeId = QtNodes::NodeId{};
            // 清空属性面板
            clearADSPropertyPanel();
            ui->statusbar->showMessage(tr("已删除节点: %1").arg(nodeInfo), Constants::STATUS_MESSAGE_TIMEOUT);
        }
        else
        {
            ui->statusbar->showMessage(tr("删除节点失败: %1").arg(nodeInfo), Constants::STATUS_MESSAGE_TIMEOUT);
        }
    }
    else
    {
        ui->statusbar->showMessage(tr("请先选择要删除的节点"), Constants::STATUS_MESSAGE_TIMEOUT);
    }
}

void MainWindow::deleteSelectedConnection()
{
    // 检查是否有有效的连接被选中（简化检查）
    // 实际应用中需要根据QtNodes的ConnectionId结构来检查
    bool hasValidConnection = true; // 简化处理

    if (!hasValidConnection)
    {
        QMessageBox::information(this, "提示", "没有选中有效的连接");
        return;
    }

    // 使用命令系统删除连接
    auto command = std::make_unique<DeleteConnectionCommand>(m_graphicsScene, m_selectedConnectionId);
    auto& commandManager = CommandManager::instance();

    // 获取连接信息用于显示
    auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedConnectionId.outNodeId);
    auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedConnectionId.inNodeId);
    QString description = "连接";

    if (outNodeDelegate && inNodeDelegate)
    {
        QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out,
                                                     m_selectedConnectionId.outPortIndex);
        QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In,
                                                    m_selectedConnectionId.inPortIndex);

        description = QString("%1[%2:%3] → %4[%5:%6]")
                      .arg(outNodeDelegate->name())
                      .arg(m_selectedConnectionId.outPortIndex)
                      .arg(outPortType)
                      .arg(inNodeDelegate->name())
                      .arg(m_selectedConnectionId.inPortIndex)
                      .arg(inPortType);
    }

    if (commandManager.executeCommand(std::move(command)))
    {
        // 连接已删除
        ui->statusbar->showMessage(tr("连接已删除: %1").arg(description), 3000);
    }
    else
    {
        ui->statusbar->showMessage(tr("删除连接失败"), 2000);
    }

    // 重置选中的连接
    m_selectedConnectionId = QtNodes::ConnectionId{};
}

void MainWindow::showAllConnectionsForDeletion()
{
    auto allNodes = m_graphModel->allNodeIds();
    QStringList connectionList;
    QList<QtNodes::ConnectionId> connections;

    for (auto nodeId : allNodes)
    {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (!nodeDelegate) continue;

        // 检查所有输出端口的连接
        unsigned int outputPorts = nodeDelegate->nPorts(QtNodes::PortType::Out);
        for (unsigned int portIndex = 0; portIndex < outputPorts; ++portIndex)
        {
            auto nodeConnections = m_graphModel->connections(nodeId, QtNodes::PortType::Out, portIndex);
            for (auto connectionId : nodeConnections)
            {
                auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.outNodeId);
                auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.inNodeId);

                if (outNodeDelegate && inNodeDelegate)
                {
                    QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out,
                                                                 connectionId.outPortIndex);
                    QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In,
                                                                connectionId.inPortIndex);

                    QString description = QString("%1[%2:%3] → %4[%5:%6]")
                                          .arg(outNodeDelegate->name())
                                          .arg(connectionId.outPortIndex)
                                          .arg(outPortType)
                                          .arg(inNodeDelegate->name())
                                          .arg(connectionId.inPortIndex)
                                          .arg(inPortType);
                    connectionList.append(description);
                    connections.append(connectionId);
                }
            }
        }
    }

    if (connections.isEmpty())
    {
        QMessageBox::information(this, "提示", "没有找到可删除的连接");
        return;
    }

    bool ok;
    QString selectedConnection = QInputDialog::getItem(this, "删除连接",
                                                       "选择要删除的连接:", connectionList, 0, false, &ok);

    if (ok && !selectedConnection.isEmpty())
    {
        int index = connectionList.indexOf(selectedConnection);
        if (index >= 0 && index < connections.size())
        {
            m_graphModel->deleteConnection(connections[index]);
            // 选中的连接已删除
        }
    }
}

void MainWindow::duplicateSelectedNode()
{
    if (m_selectedNodeId != QtNodes::NodeId{})
    {
        // 获取原节点的信息
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedNodeId);
        if (!nodeDelegate)
        {
            ui->statusbar->showMessage(tr("复制节点失败：无法获取节点信息"), 2000);
            return;
        }

        // 获取节点的真实类型
        QString nodeType = nodeDelegate->name();

        // 获取原节点位置并偏移
        QVariant posVariant = m_graphModel->nodeData(m_selectedNodeId, QtNodes::NodeRole::Position);
        QPointF originalPos = posVariant.toPointF();
        QPointF newPos = originalPos + QPointF(50, 50); // 偏移50像素

        // 复制节点

        // 使用命令系统创建相同类型的新节点
        auto command = std::make_unique<CreateNodeCommand>(m_graphicsScene, nodeType, newPos);
        auto& commandManager = CommandManager::instance();

        // 保存原节点的完整数据（包括属性）
        QJsonObject originalNodeData = m_graphModel->saveNode(m_selectedNodeId);

        if (commandManager.executeCommand(std::move(command)))
        {
            // 获取新创建的节点ID（应该是最后一个创建的）
            auto allNodeIds = m_graphModel->allNodeIds();
            QtNodes::NodeId newNodeId;

            // 找到新创建的节点（位置最接近newPos的节点）
            double minDistance = std::numeric_limits<double>::max();
            for (const auto& nodeId : allNodeIds)
            {
                QVariant posVar = m_graphModel->nodeData(nodeId, QtNodes::NodeRole::Position);
                QPointF nodePos = posVar.toPointF();
                double distance = QPointF(nodePos - newPos).manhattanLength();
                if (distance < minDistance)
                {
                    minDistance = distance;
                    newNodeId = nodeId;
                }
            }

            // 恢复节点属性（除了位置）
            if (newNodeId != QtNodes::NodeId{} && !originalNodeData.isEmpty())
            {
                // 创建一个新的数据对象，保留新位置
                QJsonObject newNodeData = originalNodeData;
                newNodeData.remove("position"); // 移除位置信息，保持新位置

                // 应用属性到新节点
                auto newNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(newNodeId);
                if (newNodeDelegate)
                {
                    newNodeDelegate->load(newNodeData);
                    // 属性已复制到新节点
                }
            }

            ui->statusbar->showMessage(tr("已复制 %1 节点（包含属性）").arg(nodeDelegate->caption()), 2000);
        }
        else
        {
            ui->statusbar->showMessage(tr("复制节点失败"), 2000);
        }
    }
}

void MainWindow::triggerDataFlow()
{
    // 触发数据流

    if (!m_graphModel)
    {
        // 图形模型不可用
        return;
    }

    // 获取所有节点ID
    auto nodeIds = m_graphModel->allNodeIds();
    // 找到节点

    // 遍历所有节点，找到源节点（没有输入连接的节点）并触发它们
    for (const auto& nodeId : nodeIds)
    {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (!nodeDelegate) continue;

        QString nodeName = nodeDelegate->name();


        // 检查是否为源节点（没有输入端口或输入端口没有连接）
        bool isSourceNode = true;
        unsigned int inputPorts = nodeDelegate->nPorts(QtNodes::PortType::In);

        for (unsigned int portIndex = 0; portIndex < inputPorts; ++portIndex)
        {
            auto connections = m_graphModel->connections(nodeId, QtNodes::PortType::In, portIndex);
            if (!connections.empty())
            {
                isSourceNode = false;
                break;
            }
        }

        if (isSourceNode)
        {
            // 找到源节点

            // 根据节点类型调用特定的触发方法
            if (nodeName == "OpenExcel")
            {
                auto* openExcelModel = m_graphModel->delegateModel<OpenExcelModel>(nodeId);
                if (openExcelModel)
                {
                    // 触发OpenExcelModel执行
                    openExcelModel->triggerExecution();
                }
            }

            // 触发源节点的数据更新
            for (unsigned int portIndex = 0; portIndex < nodeDelegate->nPorts(QtNodes::PortType::Out); ++portIndex)
            {
                emit nodeDelegate->dataUpdated(portIndex);
            }
        }
    }
}

void MainWindow::setupCustomStyles()
{
    // 自定义节点样式JSON
    QString nodeStyleJson = R"({
        "NodeStyle": {
            "NormalBoundaryColor": [255, 255, 255],
            "SelectedBoundaryColor": [255, 165, 0],
            "GradientColor0": [240, 240, 240],
            "GradientColor1": [220, 220, 220],
            "GradientColor2": [200, 200, 200],
            "GradientColor3": [180, 180, 180],
            "ShadowColor": [20, 20, 20],
            "FontColor": [10, 10, 10],
            "FontColorFaded": [100, 100, 100],
            "ConnectionPointColor": [70, 130, 180],
            "FilledConnectionPointColor": [34, 139, 34],
            "WarningColor": [128, 128, 0],
            "ErrorColor": [255, 50, 50],
            "PenWidth": 2.0,
            "HoveredPenWidth": 2.5,
            "ConnectionPointDiameter": 10.0,
            "Opacity": 1.0
        }
    })";

    // 自定义连接线样式JSON
    QString connectionStyleJson = R"({
        "ConnectionStyle": {
            "ConstructionColor": [169, 169, 169],
            "NormalColor": [100, 100, 100],
            "SelectedColor": [255, 165, 0],
            "SelectedHaloColor": [255, 165, 0, 50],
            "HoveredColor": [136, 136, 136],
            "LineWidth": 3.0,
            "ConstructionLineWidth": 2.0,
            "PointDiameter": 10.0,
            "UseDataDefinedColors": true
        }
    })";

    // 应用样式
    QtNodes::ConnectionStyle::setConnectionStyle(connectionStyleJson);
    QtNodes::NodeStyle::setNodeStyle(nodeStyleJson);

    // 样式已应用
}

QString MainWindow::getPortTypeDescription(QtNodes::NodeDelegateModel* nodeModel, QtNodes::PortType portType,
                                           QtNodes::PortIndex portIndex)
{
    if (!nodeModel) return "Unknown";

    auto dataType = nodeModel->dataType(portType, portIndex);
    QString typeName = dataType.name;

    // 将数据类型转换为更友好的显示名称
    static QMap<QString, QString> typeDescriptions = {
        {"WorkbookData", "工作簿"},
        {"SheetData", "工作表"},
        {"RangeData", "范围数据"},
        {"RowData", "行数据"},
        {"CellData", "单元格"},
        {"BooleanData", "布尔值"}
    };

    return typeDescriptions.value(typeName, typeName);
}

void MainWindow::showImprovedSceneContextMenu(const QPointF& pos)
{
    QMenu contextMenu(this);
    contextMenu.setStyleSheet(
        "QMenu {"
        "background-color: white;"
        "border: 1px solid #ccc;"
        "border-radius: 4px;"
        "padding: 4px;"
        "}"
        "QMenu::item {"
        "padding: 8px 24px;"
        "border: none;"
        "}"
        "QMenu::item:selected {"
        "background-color: #e3f2fd;"
        "color: #1976d2;"
        "}"
        "QMenu::separator {"
        "height: 1px;"
        "background-color: #eee;"
        "margin: 4px 8px;"
        "}"
    );

    // 这里只处理空白区域的菜单，不包含节点操作

    // 常用节点快速访问
    QMenu* quickAccessMenu = contextMenu.addMenu("⭐ 常用节点");
    QList<NodeInfo> favoriteNodes = NodeCatalog::getFrequentlyUsedNodes();
    for (const NodeInfo& nodeInfo : favoriteNodes)
    {
        QAction* action = quickAccessMenu->addAction(nodeInfo.displayName);
        action->setToolTip(nodeInfo.description);
        connect(action, &QAction::triggered, [this, nodeInfo, pos]()
        {
            createNodeFromPalette(nodeInfo.id, pos);
        });
    }

    contextMenu.addSeparator();

    // 按分类添加节点
    QStringList categories = NodeCatalog::getAllCategories();
    for (const QString& category : categories)
    {
        QMenu* categoryMenu = contextMenu.addMenu(category);
        QList<NodeInfo> categoryNodes = NodeCatalog::getNodesByCategory(category);

        for (const NodeInfo& nodeInfo : categoryNodes)
        {
            QAction* action = categoryMenu->addAction(nodeInfo.displayName);
            action->setToolTip(nodeInfo.description);
            connect(action, &QAction::triggered, [this, nodeInfo, pos]()
            {
                createNodeFromPalette(nodeInfo.id, pos);
            });
        }
    }

    contextMenu.addSeparator();

    // 画布操作
    QAction* clearAllAction = contextMenu.addAction("🗑️ 清空画布");
    connect(clearAllAction, &QAction::triggered, [this]()
    {
        if (QMessageBox::question(this, "确认", "确定要清空所有节点吗？\n此操作可以撤销。") == QMessageBox::Yes)
        {
            auto nodeIds = m_graphModel->allNodeIds();
            if (!nodeIds.empty())
            {
                auto& commandManager = CommandManager::instance();
                commandManager.beginMacro("清空画布");

                for (auto nodeId : nodeIds)
                {
                    auto command = std::make_unique<DeleteNodeCommand>(m_graphicsScene, nodeId);
                    commandManager.executeCommand(std::move(command));
                }

                commandManager.endMacro();
                ui->statusbar->showMessage(tr("已清空画布，删除了 %1 个节点").arg(nodeIds.size()), 3000);
            }
        }
    });

    // 转换坐标并显示菜单
    QPoint globalPos = m_graphicsView->mapToGlobal(m_graphicsView->mapFromScene(pos));
    contextMenu.exec(globalPos);
}

void MainWindow::setupAdvancedPanels()
{
    // 设置ADS面板系统

    try
    {
        // 创建ADS面板管理器
        m_adsPanelManager = new ADSPanelManager(this, this);

        // 初始化ADS系统
        m_adsPanelManager->initialize();

        // 连接面板事件
        connect(m_adsPanelManager, &ADSPanelManager::panelCreated,
                this, [this](const QString& panelId, ADSPanelManager::PanelType type)
                {
                    // ADS面板已创建
                    if (ui->statusbar)
                    {
                        ui->statusbar->showMessage(tr("面板已创建: %1").arg(panelId), 2000);
                    }
                });
        connect(m_adsPanelManager, &ADSPanelManager::panelFocused,
                this, [this](const QString& panelId)
                {
                    // 当属性面板获得焦点时，确保图形模型引用是最新的
                    if (panelId == "property_panel")
                    {
                        updatePropertyPanelReference();
                    }
                });




        // 设置ADS中央部件
        setupADSCentralWidget();

        // 设置默认布局
        m_adsPanelManager->setupDefaultLayout();

        // 更新面板引用
        updatePropertyPanelReference();

        // 连接节点面板信号
        connectADSNodePaletteSignals();

        // ADS系统设置完成
    }
    catch (const std::exception& e)
    {
        qCritical() << "MainWindow: ADS系统初始化失败:" << e.what();

        // 清理失败的初始化
        if (m_adsPanelManager)
        {
            delete m_adsPanelManager;
            m_adsPanelManager = nullptr;
        }

        QMessageBox::critical(this, "错误",
                              QString("ADS面板系统初始化失败：%1").arg(e.what()));
    } catch (...)
    {
        qCritical() << "MainWindow: ADS系统初始化发生未知错误";

        // 清理失败的初始化
        if (m_adsPanelManager)
        {
            delete m_adsPanelManager;
            m_adsPanelManager = nullptr;
        }

        QMessageBox::critical(this, "错误", "ADS面板系统初始化发生未知错误");
    }
}


void MainWindow::updateADSPropertyPanel(QtNodes::NodeId nodeId)
{
    if (!m_adsPanelManager) return;

    // 确保属性面板引用是最新的（只在必要时调用）
    static bool referenceUpdated = false;
    if (!referenceUpdated)
    {
        updatePropertyPanelReference();
        referenceUpdated = true;
    }

    // 确保属性面板可见
    m_adsPanelManager->showPanel("property_panel");

    // 使用ADS属性面板
    auto* adsPropertyPanel = m_adsPanelManager->getADSPropertyPanel();
    if (adsPropertyPanel)
    {
        adsPropertyPanel->updateNodeProperties(nodeId);

        // 立即更新面板标题
        if (auto* propertyPanel = m_adsPanelManager->getPanel("property_panel"))
        {
            auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
            if (nodeDelegate)
            {
                QString newTitle = QString("🔧 属性面板 - %1").arg(nodeDelegate->caption());
                propertyPanel->setWindowTitle(newTitle);
            }
        }
    }
    else
    {
        qWarning() << "MainWindow: ADS属性面板不可用";
    }
}

void MainWindow::clearADSPropertyPanel()
{
    if (!m_adsPanelManager) return;

    // 确保属性面板引用是最新的
    updatePropertyPanelReference();

    // 使用ADS属性面板
    auto* adsPropertyPanel = m_adsPanelManager->getADSPropertyPanel();
    if (adsPropertyPanel)
    {
        adsPropertyPanel->clearProperties();
    }
    else
    {
        qWarning() << "MainWindow: ADS属性面板不可用";
    }

    // 重置ADS面板标题
    if (auto* propertyPanel = m_adsPanelManager->getPanel("property_panel"))
    {
        propertyPanel->setWindowTitle("🔧 属性面板");
    }
}

void MainWindow::setupADSCentralWidget()
{
    // 验证必要组件
    if (!validateADSComponents()) {
        return;
    }

    auto* dockManager = m_adsPanelManager->dockManager();

    // 检查是否已经有中央部件
    auto* existingCentralWidget = dockManager->centralWidget();
    if (existingCentralWidget)
    {
        // 如果已经有中央部件，只需要更新其内容
        existingCentralWidget->setWidget(m_graphicsView);
        return;
    }

    // 创建新的中央部件
    createADSCentralWidget(dockManager);
}

bool MainWindow::validateADSComponents()
{
    if (!m_adsPanelManager)
    {
        qCritical() << "MainWindow: ADS面板管理器不存在";
        return false;
    }

    if (!m_graphicsView)
    {
        qCritical() << "MainWindow: 图形视图不存在";
        return false;
    }

    auto* dockManager = m_adsPanelManager->dockManager();
    if (!dockManager)
    {
        qCritical() << "MainWindow: DockManager不存在";
        return false;
    }

    return true;
}

void MainWindow::createADSCentralWidget(ads::CDockManager* dockManager)
{
    // 创建中央停靠部件
    auto* centralDockWidget = new ads::CDockWidget("", dockManager);
    centralDockWidget->setWidget(m_graphicsView);
    centralDockWidget->setObjectName("central_editor");

    // 配置中央部件属性
    configureCentralWidgetFeatures(centralDockWidget);

    // 设置为ADS中央部件
    dockManager->setCentralWidget(centralDockWidget);
}

void MainWindow::configureCentralWidgetFeatures(ads::CDockWidget* centralDockWidget)
{
    // 禁用所有可能导致独立窗口的功能
    centralDockWidget->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    centralDockWidget->setFeature(ads::CDockWidget::DockWidgetMovable, false);
    centralDockWidget->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
    centralDockWidget->setFeature(ads::CDockWidget::DockWidgetPinnable, false);

    // 隐藏标题栏，让它看起来像普通的中央部件
    centralDockWidget->setFeature(ads::CDockWidget::NoTab, true);

    // 确保它不会作为独立窗口显示
    centralDockWidget->setWindowFlags(Qt::Widget);
}

void MainWindow::updatePropertyPanelReference()
{
    // 更新属性面板引用

    // 检查ADS面板管理器
    if (!m_adsPanelManager)
    {
        qWarning() << "MainWindow: ADS面板管理器不存在，尝试重新初始化";
        // 尝试重新初始化ADS系统
        QTimer::singleShot(100, this, [this]() {
            if (!m_adsPanelManager) {
                setupAdvancedPanels();
            }
        });
        return;
    }

    // 获取ADS属性面板
    try
    {
        auto* adsPropertyPanel = m_adsPanelManager->getADSPropertyPanel();
        if (adsPropertyPanel)
        {
            // 检查图形模型
            if (!m_graphModel)
            {
                qWarning() << "MainWindow: 图形模型尚未创建，无法设置到属性面板";
                return;
            }

            // 设置图形模型到ADS属性面板
            adsPropertyPanel->setGraphModel(m_graphModel.get());

        }
        else
        {
            qWarning() << "MainWindow: ADS属性面板未创建";
        }
    }
    catch (const std::exception& e)
    {
        qCritical() << "MainWindow: 更新属性面板引用时发生异常:" << e.what();
    } catch (...)
    {
        qCritical() << "MainWindow: 更新属性面板引用时发生未知异常";
    }
}

void MainWindow::connectADSNodePaletteSignals()
{
    // 连接ADS节点面板信号

    // 连接ADS节点面板的信号
    if (!m_adsPanelManager)
    {
        qWarning() << "MainWindow: ADS面板管理器不存在，无法连接节点面板信号";
        return;
    }

    // 获取节点面板

    try
    {
        auto* nodePalette = m_adsPanelManager->getNodePalette();
        if (!nodePalette)
        {
            qWarning() << "MainWindow: 节点面板尚未创建，无法连接信号";
            return;
        }

        // 节点面板获取成功，连接信号

        // 连接节点面板信号
        QObject::connect(nodePalette, SIGNAL(nodeCreationRequested(QString)),
                         this, SLOT(onNodePaletteCreationRequested(QString)));
        QObject::connect(nodePalette, SIGNAL(nodeSelectionChanged(QString)),
                         this, SLOT(onNodePaletteSelectionChanged(QString)));

        // 节点面板信号连接成功
    }
    catch (const std::exception& e)
    {
        qCritical() << "MainWindow: 连接节点面板信号时发生异常:" << e.what();
    } catch (...)
    {
        qCritical() << "MainWindow: 连接节点面板信号时发生未知异常";
    }
}

void MainWindow::onNodePaletteCreationRequested(const QString& nodeId)
{
    // 获取当前鼠标位置作为默认创建位置
    QPoint globalMousePos = QCursor::pos();
    QPoint viewPos = m_graphicsView->mapFromGlobal(globalMousePos);
    QPointF scenePos = m_graphicsView->mapToScene(viewPos);

    // 如果鼠标不在视图内，使用视图中心
    if (!m_graphicsView->rect().contains(viewPos))
    {
        scenePos = m_graphicsView->mapToScene(m_graphicsView->rect().center());
    }

    createNodeFromPalette(nodeId, scenePos);
}

void MainWindow::onNodePaletteSelectionChanged(const QString& nodeId)
{
    // 显示节点信息
    NodeInfo nodeInfo = NodeCatalog::getNodeInfo(nodeId);
    if (!nodeInfo.id.isEmpty())
    {
        ui->statusbar->showMessage(
            QString("选中节点: %1 - %2").arg(nodeInfo.displayName).arg(nodeInfo.description), 3000);
    }
}

void MainWindow::createNodeFromPalette(const QString& nodeId, const QPointF& position)
{
    // 从节点面板创建节点

    auto command = std::make_unique<CreateNodeCommand>(m_graphicsScene, nodeId, position);
    auto& commandManager = CommandManager::instance();

    if (commandManager.executeCommand(std::move(command)))
    {
        NodeInfo nodeInfo = NodeCatalog::getNodeInfo(nodeId);
        ui->statusbar->showMessage(tr("已创建节点: %1").arg(nodeInfo.displayName), 2000);
    }
    else
    {
        ui->statusbar->showMessage(tr("创建节点失败"), 2000);
    }
}

void MainWindow::setupKeyboardShortcuts()
{
    // 缩放快捷键
    QShortcut* zoomInShortcut = new QShortcut(QKeySequence("Ctrl++"), this);
    connect(zoomInShortcut, &QShortcut::activated, this, [this]()
    {
        if (m_modernToolBar)
        {
            emit m_modernToolBar->zoomInRequested();
        }
    });

    QShortcut* zoomOutShortcut = new QShortcut(QKeySequence("Ctrl+-"), this);
    connect(zoomOutShortcut, &QShortcut::activated, this, [this]()
    {
        if (m_modernToolBar)
        {
            emit m_modernToolBar->zoomOutRequested();
        }
    });

    QShortcut* zoomFitShortcut = new QShortcut(QKeySequence("Ctrl+0"), this);
    connect(zoomFitShortcut, &QShortcut::activated, this, [this]()
    {
        if (m_modernToolBar)
        {
            emit m_modernToolBar->zoomFitRequested();
        }
    });

    // 重置缩放快捷键
    QShortcut* resetZoomShortcut = new QShortcut(QKeySequence("Ctrl+1"), this);
    connect(resetZoomShortcut, &QShortcut::activated, this, [this]()
    {
        if (m_graphicsView)
        {
            m_graphicsView->resetTransform();
            ui->statusbar->showMessage(tr("缩放已重置为 100%"), 1000);
        }
    });

    // 删除快捷键
    QShortcut* deleteShortcut = new QShortcut(QKeySequence::Delete, this);
    connect(deleteShortcut, &QShortcut::activated, this, &MainWindow::deleteSelectedNode);

    // 备用删除快捷键
    QShortcut* deleteShortcut2 = new QShortcut(QKeySequence("Backspace"), this);
    connect(deleteShortcut2, &QShortcut::activated, this, &MainWindow::deleteSelectedNode);

    // 撤销重做快捷键
    QShortcut* undoShortcut = new QShortcut(QKeySequence::Undo, this);
    connect(undoShortcut, &QShortcut::activated, this, &MainWindow::onUndoClicked);

    QShortcut* redoShortcut = new QShortcut(QKeySequence::Redo, this);
    connect(redoShortcut, &QShortcut::activated, this, &MainWindow::onRedoClicked);

    // 复制快捷键
    QShortcut* duplicateShortcut = new QShortcut(QKeySequence("Ctrl+D"), this);
    connect(duplicateShortcut, &QShortcut::activated, this, &MainWindow::duplicateSelectedNode);

    // 帮助快捷键
    QShortcut* helpShortcut = new QShortcut(QKeySequence::HelpContents, this);
    connect(helpShortcut, &QShortcut::activated, this, &MainWindow::showShortcutHelp);

    // 快捷键设置完成
}

void MainWindow::setupLayoutMenu()
{
    setupFileMenu();
    setupViewMenu();
    setupHelpMenu();
}

void MainWindow::setupFileMenu()
{
    QMenu* fileMenu = menuBar()->addMenu("📁 文件");

    // 使用结构化数据定义菜单项
    struct MenuAction {
        QString text;
        QKeySequence shortcut;
        std::function<void()> slot;
        bool addSeparatorAfter = false;
    };

    QVector<MenuAction> fileActions = {
        {"🆕 新建", QKeySequence::New, [this]() { onNewFile(); }},
        {"📂 打开", QKeySequence::Open, [this]() { onOpenFile(); }, true},
        {"💾 保存", QKeySequence::Save, [this]() { onSaveFile(); }, true},
        {"🚪 退出", QKeySequence::Quit, [this]() { close(); }}
    };

    // 批量创建菜单项
    for (const auto& actionData : fileActions) {
        QAction* action = fileMenu->addAction(actionData.text);
        action->setShortcut(actionData.shortcut);
        connect(action, &QAction::triggered, this, actionData.slot);

        if (actionData.addSeparatorAfter) {
            fileMenu->addSeparator();
        }
    }
}



void MainWindow::setupViewMenu()
{
    QMenu* viewMenu = menuBar()->addMenu("👁️ 视图");

    // 创建ADS布局子菜单
    if (m_adsPanelManager) {
        createADSLayoutMenu(viewMenu);
        viewMenu->addSeparator();
    }

    // 添加全屏控制
    createViewControlMenu(viewMenu);
}

void MainWindow::createADSLayoutMenu(QMenu* parentMenu)
{
    QMenu* adsLayoutMenu = parentMenu->addMenu("🎛️ ADS布局");

    // 检查ADS管理器是否可用
    if (!m_adsPanelManager) {
        QAction* errorAction = adsLayoutMenu->addAction("❌ ADS系统未初始化");
        errorAction->setEnabled(false);
        return;
    }

    // 布局控制
    QAction* defaultLayoutAction = adsLayoutMenu->addAction("🏠 恢复默认布局");
    connect(defaultLayoutAction, &QAction::triggered, this, [this]() {
        if (m_adsPanelManager) {
            m_adsPanelManager->restoreDefaultLayout();
            ui->statusbar->showMessage("已恢复默认布局", 2000);
        }
    });

    QAction* resetLayoutAction = adsLayoutMenu->addAction("🔄 重置布局");
    connect(resetLayoutAction, &QAction::triggered, this, [this]() {
        if (m_adsPanelManager) {
            m_adsPanelManager->resetToDefaultLayout();
            ui->statusbar->showMessage("已重置布局", 2000);
        }
    });

    adsLayoutMenu->addSeparator();

    // 面板显示/隐藏
    QAction* propertyPanelAction = adsLayoutMenu->addAction("🔧 属性面板");
    propertyPanelAction->setCheckable(true);
    connect(propertyPanelAction, &QAction::triggered, this, [this](bool checked) {
        if (m_adsPanelManager) {
            if (checked) {
                m_adsPanelManager->showPanel("property_panel");
            } else {
                m_adsPanelManager->hidePanel("property_panel");
            }
        }
    });

    QAction* nodePaletteAction = adsLayoutMenu->addAction("🗂️ 节点面板");
    nodePaletteAction->setCheckable(true);
    connect(nodePaletteAction, &QAction::triggered, this, [this](bool checked) {
        if (m_adsPanelManager) {
            if (checked) {
                m_adsPanelManager->showPanel("node_palette");
            } else {
                m_adsPanelManager->hidePanel("node_palette");
            }
        }
    });

    QAction* outputConsoleAction = adsLayoutMenu->addAction("💻 输出控制台");
    outputConsoleAction->setCheckable(true);
    connect(outputConsoleAction, &QAction::triggered, this, [this](bool checked) {
        if (m_adsPanelManager) {
            if (checked) {
                m_adsPanelManager->showPanel("output_console");
            } else {
                m_adsPanelManager->hidePanel("output_console");
            }
        }
    });

    QAction* commandHistoryAction = adsLayoutMenu->addAction("📜 命令历史");
    commandHistoryAction->setCheckable(true);
    connect(commandHistoryAction, &QAction::triggered, this, [this](bool checked) {
        if (m_adsPanelManager) {
            if (checked) {
                m_adsPanelManager->showPanel("command_history");
            } else {
                m_adsPanelManager->hidePanel("command_history");
            }
        }
    });

    adsLayoutMenu->addSeparator();

    // 布局保存
    QAction* saveLayoutAction = adsLayoutMenu->addAction("💾 保存当前布局");
    connect(saveLayoutAction, &QAction::triggered, this, &MainWindow::saveCurrentLayout);
}

void MainWindow::createViewControlMenu(QMenu* parentMenu)
{
    // 全屏控制
    QAction* fullScreenAction = parentMenu->addAction("🖥️ 全屏");
    fullScreenAction->setCheckable(true);
    fullScreenAction->setShortcut(QKeySequence("F11"));
    connect(fullScreenAction, &QAction::toggled, this, [this](bool fullScreen) {
        fullScreen ? showFullScreen() : showNormal();
    });
}

void MainWindow::saveCurrentLayout()
{
    QString layoutName = QString("user_layout_%1").arg(
        QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    m_adsPanelManager->saveLayoutPreset(layoutName);
    ui->statusbar->showMessage(tr("布局已保存: %1").arg(layoutName), 3000);
}

QAction* MainWindow::createMenuAction(QMenu* menu, const QString& text,
                                     const QKeySequence& shortcut,
                                     std::function<void()> slot)
{
    QAction* action = menu->addAction(text);
    if (!shortcut.isEmpty()) {
        action->setShortcut(shortcut);
    }
    if (slot) {
        connect(action, &QAction::triggered, this, slot);
    }
    return action;
}

void MainWindow::setupWindowDisplay()
{
    // 设置窗口标题
    setWindowTitle("TinaFlow - 节点流程编辑器");

    // 设置窗口图标（如果有的话）
    // setWindowIcon(QIcon(":/icons/tinaflow.png"));

    // 尝试从设置中恢复窗口几何
    QSettings settings;
    bool geometryRestored = false;

    if (settings.contains("geometry"))
    {
        QByteArray geometry = settings.value("geometry").toByteArray();
        if (restoreGeometry(geometry))
        {
            geometryRestored = true;
        }
    }

    // 如果没有恢复几何信息，设置默认大小和位置
    if (!geometryRestored)
    {
        resize(1200, 800);
        // 居中显示
        QRect screenGeometry = QApplication::primaryScreen()->geometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }

    // 保存几何信息
    connect(this, &QWidget::destroyed, [this]()
    {
        QSettings settings;
        settings.setValue("geometry", saveGeometry());
    });
}

void MainWindow::setupAutoSave()
{
    // 设置自动保存间隔（5分钟）
    m_autoSaveTimer->setInterval(5 * 60 * 1000);
    m_autoSaveTimer->setSingleShot(false);

    connect(m_autoSaveTimer, &QTimer::timeout, this, [this]() {
        if (m_hasUnsavedChanges && !m_currentFilePath.isEmpty()) {
            // 创建自动保存文件名
            QFileInfo fileInfo(m_currentFilePath);
            QString autoSavePath = fileInfo.absolutePath() + "/" +
                                 fileInfo.baseName() + "_autosave." + fileInfo.suffix();

            try {
                // 保存到自动保存文件
                QJsonObject sceneJson = m_graphModel->save();
                QJsonDocument document(sceneJson);

                QFile file(autoSavePath);
                if (file.open(QIODevice::WriteOnly)) {
                    file.write(document.toJson());
                    file.close();
                    qDebug() << "Auto-saved to:" << autoSavePath;
                }
            } catch (const std::exception& e) {
                qWarning() << "Auto-save failed:" << e.what();
            }
        }
    });

    // 监听图形模型变化
    if (m_graphModel) {
        connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeCreated,
                this, [this]() {
                    m_hasUnsavedChanges = true;
                    updateWindowTitle();
                });
        connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeDeleted,
                this, [this]() {
                    m_hasUnsavedChanges = true;
                    updateWindowTitle();
                });
        connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionCreated,
                this, [this]() {
                    m_hasUnsavedChanges = true;
                    updateWindowTitle();
                });
        connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionDeleted,
                this, [this]() {
                    m_hasUnsavedChanges = true;
                    updateWindowTitle();
                });
    }

    // 启动自动保存定时器
    m_autoSaveTimer->start();
}

void MainWindow::setupHelpMenu()
{
    QMenu* helpMenu = menuBar()->addMenu("❓ 帮助");

    // 快捷键帮助
    QAction* shortcutHelpAction = helpMenu->addAction("⌨️ 快捷键帮助");
    shortcutHelpAction->setShortcut(QKeySequence::HelpContents);
    connect(shortcutHelpAction, &QAction::triggered, this, &MainWindow::showShortcutHelp);

    helpMenu->addSeparator();

    // 用户指南
    QAction* userGuideAction = helpMenu->addAction("📖 用户指南");
    connect(userGuideAction, &QAction::triggered, this, &MainWindow::showUserGuide);

    // 报告问题
    QAction* reportBugAction = helpMenu->addAction("🐛 报告问题");
    connect(reportBugAction, &QAction::triggered, this, &MainWindow::reportBug);

    helpMenu->addSeparator();

    // 关于
    QAction* aboutAction = helpMenu->addAction("ℹ️ 关于 TinaFlow");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);
}

void MainWindow::showShortcutHelp()
{
    // TODO: 实现快捷键帮助对话框
    QMessageBox::information(this, "快捷键帮助",
        "常用快捷键：\n\n"
        "文件操作：\n"
        "Ctrl+N - 新建\n"
        "Ctrl+O - 打开\n"
        "Ctrl+S - 保存\n\n"
        "编辑操作：\n"
        "Ctrl+Z - 撤销\n"
        "Ctrl+Y - 重做\n"
        "Delete - 删除选中节点\n"
        "Ctrl+D - 复制节点\n\n"
        "视图操作：\n"
        "Ctrl++ - 放大\n"
        "Ctrl+- - 缩小\n"
        "Ctrl+0 - 适应窗口\n"
        "F11 - 全屏\n\n"
        "执行控制：\n"
        "F5 - 运行\n"
        "Shift+F5 - 停止");
}

void MainWindow::showAboutDialog()
{
    QMessageBox::about(this, "关于 TinaFlow",
        "<h2>TinaFlow 节点流程编辑器</h2>"
        "<p><b>版本:</b> 1.0</p>"
        "<p>一个强大的可视化节点编程工具，专注于Excel数据处理和自动化流程</p>"
        "<p><b>主要功能：</b></p>"
        "<ul>"
        "<li>🎯 可视化节点编程</li>"
        "<li>📊 Excel数据读取与处理</li>"
        "<li>🔄 智能循环处理</li>"
        "<li>💾 Excel文件保存</li>"
        "<li>🔗 数据流可视化</li>"
        "<li>⚡ 高性能数据处理</li>"
        "<li>🎨 现代化用户界面</li>"
        "</ul>"
        "<p><b>技术栈：</b></p>"
        "<p>Qt6, C++20, OpenXLSX, QtNodes, ADS</p>"
        "<p><b>联系方式：</b></p>"
        "<p>📧 3344207732@qq.com | 💬 QQ群: 876680171</p>"
        "<p>© 2025 TinaFlow. All rights reserved.</p>");
}

void MainWindow::showUserGuide()
{
    QMessageBox::information(this, "用户指南",
        "<h3>TinaFlow 使用指南</h3>"
        "<p><b>1. 创建节点：</b></p>"
        "<p>从左侧节点面板拖拽节点到画布，或右键点击空白区域选择节点</p>"
        "<p><b>2. 连接节点：</b></p>"
        "<p>拖拽节点的输出端口到另一个节点的输入端口</p>"
        "<p><b>3. 配置属性：</b></p>"
        "<p>选中节点后在右侧属性面板中配置参数</p>"
        "<p><b>4. 运行流程：</b></p>"
        "<p>点击工具栏的运行按钮或按F5键执行流程</p>"
        "<p><b>5. 保存工作：</b></p>"
        "<p>使用Ctrl+S保存当前工作流程</p>");
}

void MainWindow::reportBug()
{
    QMessageBox::information(this, "报告问题",
        "<h3>问题反馈</h3>"
        "<p>如果您遇到问题或有改进建议，请通过以下方式联系我们：</p>"
        "<p><b>邮箱：</b> 3344207732@qq.com</p>"
        "<p><b>QQ群：</b> 876680171</p>"
        "<p>请详细描述问题的重现步骤，包括：</p>"
        "<ul>"
        "<li>操作系统版本</li>"
        "<li>具体的操作步骤</li>"
        "<li>期望的结果和实际结果</li>"
        "<li>如有可能，请提供相关的.tflow文件</li>"
        "</ul>"
        "<p>我们会尽快处理您的反馈。</p>");
}

void MainWindow::updateWindowTitle()
{
    QString title = "TinaFlow";

    if (!m_currentFilePath.isEmpty()) {
        QFileInfo fileInfo(m_currentFilePath);
        title += " - " + fileInfo.baseName();
    } else {
        title += " - 新建";
    }

    if (m_hasUnsavedChanges) {
        title += " *";  // 添加星号表示有未保存的更改
    }

    setWindowTitle(title);
}

void MainWindow::setupStatusBar()
{
    // 设置状态栏组件样式
    m_nodeCountLabel->setStyleSheet("QLabel { padding: 2px 8px; border: 1px solid #ccc; border-radius: 3px; background-color: #f0f0f0; }");
    m_connectionCountLabel->setStyleSheet("QLabel { padding: 2px 8px; border: 1px solid #ccc; border-radius: 3px; background-color: #f0f0f0; }");
    m_statusLabel->setStyleSheet("QLabel { padding: 2px 8px; color: #666; }");

    // 初始化显示
    updateStatusBarInfo();

    // 添加到状态栏
    ui->statusbar->addPermanentWidget(m_nodeCountLabel);
    ui->statusbar->addPermanentWidget(m_connectionCountLabel);
    ui->statusbar->addWidget(m_statusLabel, 1); // 拉伸填充

    // 连接图模型变化信号
    if (m_graphModel) {
        connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeCreated,
                this, &MainWindow::updateStatusBarInfo);
        connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeDeleted,
                this, &MainWindow::updateStatusBarInfo);
        connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionCreated,
                this, &MainWindow::updateStatusBarInfo);
        connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::connectionDeleted,
                this, &MainWindow::updateStatusBarInfo);
    }
}

void MainWindow::updateStatusBarInfo()
{
    if (!m_graphModel) return;

    int nodeCount = static_cast<int>(m_graphModel->allNodeIds().size());
    int connectionCount = getTotalConnectionCount();

    m_nodeCountLabel->setText(QString("📦 节点: %1").arg(nodeCount));
    m_connectionCountLabel->setText(QString("🔗 连接: %1").arg(connectionCount));

    if (nodeCount == 0) {
        m_statusLabel->setText("准备就绪 - 从左侧面板拖拽节点开始创建流程");
    } else {
        m_statusLabel->setText(QString("工作流包含 %1 个节点和 %2 个连接").arg(nodeCount).arg(connectionCount));
    }
}

int MainWindow::getTotalConnectionCount() const
{
    if (!m_graphModel) return 0;

    std::unordered_set<QtNodes::ConnectionId> allConnections;
    auto allNodes = m_graphModel->allNodeIds();

    // 遍历所有节点，收集所有连接
    for (const auto& nodeId : allNodes) {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (!nodeDelegate) continue;

        // 只检查输出端口，避免重复计算同一个连接
        unsigned int outputPorts = nodeDelegate->nPorts(QtNodes::PortType::Out);
        for (unsigned int portIndex = 0; portIndex < outputPorts; ++portIndex) {
            auto connections = m_graphModel->connections(nodeId, QtNodes::PortType::Out, portIndex);
            for (const auto& conn : connections) {
                allConnections.insert(conn);
            }
        }
    }

    return static_cast<int>(allConnections.size());
}
