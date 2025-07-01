#include "mainwindow.hpp"
#include "ui_mainwindow.h"

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

// 系统组件
#include "IPropertyProvider.hpp"
#include "widget/PropertyWidget.hpp"
#include "ErrorHandler.hpp"
#include "DataValidator.hpp"
#include "CommandManager.hpp"
#include "NodeCommands.hpp"
#include "widget/ModernToolBar.hpp"
#include "widget/ADSPanelManager.hpp"
#include "NodeCatalog.hpp"

// QtNodes
#include <QtNodes/ConnectionStyle>
#include <QtNodes/NodeStyle>
#include <QtNodes/DataFlowGraphicsScene>

// Qt核心
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QInputDialog>
#include <limits>

// Qt界面
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QFrame>
#include <QShortcut>
#include <QDockWidget>
#include <QCursor>
#include <QMenuBar>
#include <QSettings>
#include <QTimer>

// 静态成员变量定义
bool MainWindow::s_globalExecutionEnabled = false;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
      , ui(new Ui::MainWindow)
      , m_currentPropertyWidget(nullptr)
      , m_propertyPanelContainer(nullptr)
      , m_modernToolBar(nullptr)
      , m_adsPanelManager(nullptr)
      , m_nodePalette(nullptr)
      , m_selectedNodeId(QtNodes::NodeId{})
      , m_selectedConnectionId(QtNodes::ConnectionId{})
{
    ui->setupUi(this);
    
    // 1. 首先设置节点编辑器（创建图形视图）
    setupNodeEditor();
    
    // 2. 然后设置现代工具栏
    setupModernToolbar();
    
    // 3. 设置 ADS 面板系统（这会直接接管主窗口）
    setupAdvancedPanels();
    
    // 4. 设置 ADS 布局菜单
    setupADSLayoutMenu();
    
    // 5. 设置窗口管理菜单（简化版）
    setupLayoutMenu();
    
    // 6. 设置快捷键
    setupKeyboardShortcuts();
    
    qDebug() << "MainWindow: ADS系统初始化完成";
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow: 开始销毁主窗口";
    
    // 自动保存窗口布局
    QSettings settings;
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    qDebug() << "MainWindow: Layout saved on exit";
    
    // 安全清理ADS面板管理器
    if (m_adsPanelManager) {
        qDebug() << "MainWindow: 开始清理ADS面板管理器";
        
        // 先断开所有连接
        m_adsPanelManager->disconnect();
        
        // 关闭面板管理器
        m_adsPanelManager->shutdown();
        
        // 安全删除
        m_adsPanelManager->deleteLater();
        m_adsPanelManager = nullptr;
        
        qDebug() << "MainWindow: ADS面板管理器清理完成";
    }
    
    // 清理其他指针
    m_propertyPanelContainer = nullptr;
    m_nodePalette = nullptr;
    
    delete ui;
    
    qDebug() << "MainWindow: 主窗口销毁完成";
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
            this, [this](QtNodes::NodeId nodeId, QtNodes::PortType, QtNodes::PortIndex) {
                // 如果更新的节点是当前选中的节点，延迟刷新属性面板
                if (nodeId == m_selectedNodeId) {
                    QMetaObject::invokeMethod(this, [this, nodeId]() {
                        // 再次检查节点是否仍然选中，避免无效刷新
                if (nodeId == m_selectedNodeId) {
                    refreshCurrentPropertyPanel();
                }
                    }, Qt::QueuedConnection);
                }
            }, Qt::QueuedConnection);

    // 重新启用节点更新事件，但只处理真正需要刷新的情况
    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeUpdated,
            this, [this](QtNodes::NodeId nodeId) {
                // 只有当节点已经选中一段时间后才刷新，避免创建时的频繁更新
                if (nodeId == m_selectedNodeId && m_selectedNodeId != QtNodes::NodeId{}) {
                    static QDateTime lastUpdate;
                    QDateTime now = QDateTime::currentDateTime();
                    // 限制刷新频率，避免过于频繁的更新
                    if (lastUpdate.isNull() || lastUpdate.msecsTo(now) > 100) {
                        lastUpdate = now;
                        QMetaObject::invokeMethod(this, [this, nodeId]() {
                            // 再次检查节点是否仍然选中
                if (nodeId == m_selectedNodeId) {
                    refreshCurrentPropertyPanel();
                }
                        }, Qt::QueuedConnection);
                    }
                }
            }, Qt::QueuedConnection);
    
    // 核心视图将直接由ADS系统管理，不再添加到传统容器
    // m_graphicsView会在setupADSCentralWidget()中被设置为ADS中央部件
}

void MainWindow::reinitializeNodeEditor()
{
    // 删除旧的组件（ADS系统会自动管理视图的父子关系）
    if (m_graphicsView) {
        delete m_graphicsView;
        delete m_graphicsScene;
    }
    
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
            this, [this](QtNodes::NodeId nodeId, QtNodes::PortType, QtNodes::PortIndex) {
                if (nodeId == m_selectedNodeId) {
                    QMetaObject::invokeMethod(this, [this, nodeId]() {
                        if (nodeId == m_selectedNodeId) {
                            refreshCurrentPropertyPanel();
                        }
                    }, Qt::QueuedConnection);
                }
            }, Qt::QueuedConnection);

    connect(m_graphModel.get(), &QtNodes::DataFlowGraphModel::nodeUpdated,
            this, [this](QtNodes::NodeId nodeId) {
                if (nodeId == m_selectedNodeId && m_selectedNodeId != QtNodes::NodeId{}) {
                    static QDateTime lastUpdate;
                    QDateTime now = QDateTime::currentDateTime();
                    if (lastUpdate.isNull() || lastUpdate.msecsTo(now) > 100) {
                        lastUpdate = now;
                        QMetaObject::invokeMethod(this, [this, nodeId]() {
                            if (nodeId == m_selectedNodeId) {
                                refreshCurrentPropertyPanel();
                            }
                        }, Qt::QueuedConnection);
                    }
                }
            }, Qt::QueuedConnection);
    
    // 重新设置ADS中央部件
    setupADSCentralWidget();
    
    // 更新属性面板容器的图形模型
    updatePropertyPanelReference();
    
    qDebug() << "MainWindow: Node editor reinitialized with fresh model";
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
    // 创建现代化工具栏
    m_modernToolBar = new ModernToolBar(this);
    
    // 将工具栏添加到主窗口
    addToolBar(Qt::TopToolBarArea, m_modernToolBar);
    
    // 连接文件操作信号
    connect(m_modernToolBar, &ModernToolBar::newFileRequested, this, &MainWindow::onNewFile);
    connect(m_modernToolBar, &ModernToolBar::openFileRequested, this, &MainWindow::onOpenFile);
    connect(m_modernToolBar, &ModernToolBar::saveFileRequested, this, &MainWindow::onSaveFile);
    
    // 连接编辑操作信号
    connect(m_modernToolBar, &ModernToolBar::undoRequested, this, &MainWindow::onUndoClicked);
    connect(m_modernToolBar, &ModernToolBar::redoRequested, this, &MainWindow::onRedoClicked);
    
    // 连接执行控制信号
    connect(m_modernToolBar, &ModernToolBar::runRequested, this, &MainWindow::onRunClicked);
    connect(m_modernToolBar, &ModernToolBar::pauseRequested, this, &MainWindow::onPauseClicked);
    connect(m_modernToolBar, &ModernToolBar::stopRequested, this, &MainWindow::onStopClicked);

    
    // 连接视图控制信号
    connect(m_modernToolBar, &ModernToolBar::zoomFitRequested, this, [this](){
        if (m_graphicsView) {
            m_graphicsView->fitInView(m_graphicsScene->itemsBoundingRect(), Qt::KeepAspectRatio);
            ui->statusbar->showMessage(tr("视图已适应窗口"), 1000);
        }
    });
    connect(m_modernToolBar, &ModernToolBar::zoomInRequested, this, [this](){
        if (m_graphicsView) {
            // 获取当前变换矩阵
            QTransform transform = m_graphicsView->transform();
            double currentScale = transform.m11(); // 获取X轴缩放比例
            
            // 设置最大缩放比例为5.0（500%）
            const double maxScale = 5.0;
            const double zoomFactor = 1.2;
            
            if (currentScale * zoomFactor <= maxScale) {
                m_graphicsView->scale(zoomFactor, zoomFactor);
                double newScale = currentScale * zoomFactor;
                ui->statusbar->showMessage(tr("缩放: %1%").arg(qRound(newScale * 100)), 1000);
        } else {
                ui->statusbar->showMessage(tr("已达到最大缩放比例 (500%)"), 2000);
            }
        }
    });
    connect(m_modernToolBar, &ModernToolBar::zoomOutRequested, this, [this](){
        if (m_graphicsView) {
            // 获取当前变换矩阵
            QTransform transform = m_graphicsView->transform();
            double currentScale = transform.m11(); // 获取X轴缩放比例
            
            // 设置最小缩放比例为0.1（10%）
            const double minScale = 0.1;
            const double zoomFactor = 0.8;
            
            if (currentScale * zoomFactor >= minScale) {
                m_graphicsView->scale(zoomFactor, zoomFactor);
                double newScale = currentScale * zoomFactor;
                ui->statusbar->showMessage(tr("缩放: %1%").arg(qRound(newScale * 100)), 1000);
        } else {
                ui->statusbar->showMessage(tr("已达到最小缩放比例 (10%)"), 2000);
            }
        }
    });

    
    // 连接命令管理器信号到工具栏（使用统一信号避免重入问题）
    auto& commandManager = CommandManager::instance();
    connect(&commandManager, &CommandManager::undoRedoStateChanged, [this](bool canUndo, bool canRedo){
        if (m_modernToolBar) {
            m_modernToolBar->updateUndoRedoState(canUndo, canRedo);
        }
    });
    
    // 初始化状态
    m_modernToolBar->updateExecutionState(false);
    m_modernToolBar->updateUndoRedoState(false, false);
}

void MainWindow::setupPropertyPanel()
{
    // 属性面板现在完全由ADS面板系统管理
    // 这个方法保留为空，保持接口兼容性
    m_currentPropertyWidget = nullptr;
    m_propertyPanelContainer = nullptr;
    
    qDebug() << "MainWindow: 属性面板由ADS系统管理";
}



void MainWindow::onNewFile()
{
    // 完全重新初始化节点编辑器以重置ID计数器
    reinitializeNodeEditor();
    
    // 清空命令历史 - 新文件不应该有撤销重做历史
    auto& commandManager = CommandManager::instance();
    commandManager.clear();
    
    // 清空属性面板 - 没有选中的节点
    clearPropertyPanel();
    m_selectedNodeId = QtNodes::NodeId{};
    
    // 重置视图缩放
    if (m_graphicsView) {
        m_graphicsView->resetTransform();
    }
    
    setWindowTitle("TinaFlow - 新建");
    ui->statusbar->showMessage(tr("新建流程，拖拽节点开始设计 (节点ID已重置)"), 3000);
    
    qDebug() << "MainWindow: New file created with fresh graph model, node IDs reset";
}

void MainWindow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("打开流程文件"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("TinaFlow文件 (*.tflow);;JSON文件 (*.json);;所有文件 (*)")
    );

    if (!fileName.isEmpty()) {
        loadFromFile(fileName);
    }
}

void MainWindow::onSaveFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("保存流程文件"),
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        tr("TinaFlow文件 (*.tflow);;JSON文件 (*.json)")
    );

    if (!fileName.isEmpty()) {
        saveToFile(fileName);
    }
}

void MainWindow::saveToFile(const QString& fileName)
{
    if (!m_graphModel) {
        QMessageBox::warning(this, tr("错误"), tr("没有可保存的数据"));
        return;
    }

    try {
        // 使用QtNodes的保存功能
        QJsonObject jsonObject = m_graphModel->save();
        QJsonDocument jsonDocument(jsonObject);

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(jsonDocument.toJson());
            file.close();

            setWindowTitle(QString("TinaFlow - %1").arg(QFileInfo(fileName).baseName()));
            ui->statusbar->showMessage(tr("文件已保存: %1").arg(fileName), 3000);
        } else {
            QMessageBox::critical(this, tr("错误"), tr("无法保存文件: %1").arg(fileName));
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("错误"), tr("保存文件时发生错误: %1").arg(e.what()));
    }
}

void MainWindow::loadFromFile(const QString& fileName)
{
    if (!m_graphModel) {
        QMessageBox::warning(this, tr("错误"), tr("图形模型未初始化"));
        return;
    }

    try {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();

            QJsonDocument jsonDocument = QJsonDocument::fromJson(data);
            if (jsonDocument.isNull()) {
                QMessageBox::critical(this, tr("错误"), tr("文件格式无效"));
                return;
            }

            // 重新初始化节点编辑器以重置ID计数器
            reinitializeNodeEditor();

            // 加载新数据
            m_graphModel->load(jsonDocument.object());
            
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

            setWindowTitle(QString("TinaFlow - %1").arg(QFileInfo(fileName).baseName()));
            ui->statusbar->showMessage(tr("流程已加载，点击运行按钮(F5)开始执行"), 0);
        } else {
            QMessageBox::critical(this, tr("错误"), tr("无法打开文件: %1").arg(fileName));
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, tr("错误"), tr("加载文件时发生错误: %1").arg(e.what()));
    }
}

void MainWindow::onRunClicked()
{
    qDebug() << "MainWindow: Run button clicked";
    setGlobalExecutionState(true);

    // 更新现代化工具栏状态
    if (m_modernToolBar) {
        m_modernToolBar->updateExecutionState(true);
    }

    // 重新触发数据流处理
    triggerDataFlow();

    ui->statusbar->showMessage(tr("流程正在运行..."), 0);
}

void MainWindow::onPauseClicked()
{
    qDebug() << "MainWindow: Pause button clicked";
    setGlobalExecutionState(false);

    // 更新现代化工具栏状态
    if (m_modernToolBar) {
        m_modernToolBar->updateExecutionState(false);
    }

    ui->statusbar->showMessage(tr("流程已暂停"), 3000);
}

void MainWindow::onStopClicked()
{
    qDebug() << "MainWindow: Stop button clicked";
    setGlobalExecutionState(false);

    // 更新现代化工具栏状态
    if (m_modernToolBar) {
        m_modernToolBar->updateExecutionState(false);
    }

    ui->statusbar->showMessage(tr("流程已停止"), 3000);
}

void MainWindow::onUndoClicked()
{
    qDebug() << "MainWindow: Undo clicked";
    auto& commandManager = CommandManager::instance();
    if (commandManager.canUndo()) {
        if (commandManager.undo()) {
            ui->statusbar->showMessage(tr("已撤销: %1").arg(commandManager.getUndoText()), 2000);
        } else {
            ui->statusbar->showMessage(tr("撤销失败"), 2000);
        }
    }
}

void MainWindow::onRedoClicked()
{
    qDebug() << "MainWindow: Redo clicked";
    auto& commandManager = CommandManager::instance();
    if (commandManager.canRedo()) {
        if (commandManager.redo()) {
            ui->statusbar->showMessage(tr("已重做: %1").arg(commandManager.getRedoText()), 2000);
        } else {
            ui->statusbar->showMessage(tr("重做失败"), 2000);
        }
    }
}


void MainWindow::setGlobalExecutionState(bool running)
{
    s_globalExecutionEnabled = running;
    qDebug() << "MainWindow: Global execution state set to:" << running;

    // 这里可以添加通知所有节点状态变化的逻辑
    // 例如通过信号通知所有节点更新执行状态
}

void MainWindow::onNodeSelected(QtNodes::NodeId nodeId)
{
    qDebug() << "MainWindow: Node selected:" << nodeId;
    m_selectedNodeId = nodeId; // 保存选中的节点ID
    updatePropertyPanel(nodeId);
    updateADSPropertyPanel(nodeId); // 同时更新ADS属性面板
}

void MainWindow::onNodeDeselected()
{
    qDebug() << "MainWindow: Node deselected";
    m_selectedNodeId = QtNodes::NodeId{}; // 清除选中的节点ID
    clearPropertyPanel();
    clearADSPropertyPanel(); // 同时清空ADS属性面板
}

void MainWindow::refreshCurrentPropertyPanel()
{
    // 如果有选中的节点，刷新其属性面板
    if (m_selectedNodeId != QtNodes::NodeId{}) {
        qDebug() << "MainWindow: Refreshing property panel for node" << m_selectedNodeId;
        updatePropertyPanel(m_selectedNodeId);
    }
}

void MainWindow::updatePropertyPanel(QtNodes::NodeId nodeId)
{
    // 传统属性面板已被ADS系统取代，直接委托给ADS属性面板
    updateADSPropertyPanel(nodeId);
    qDebug() << "MainWindow: Delegated property panel update to ADS system for node" << nodeId;
}

void MainWindow::clearPropertyPanel()
{
    // 传统属性面板已被ADS系统取代，直接委托给ADS属性面板
    clearADSPropertyPanel();
    qDebug() << "MainWindow: Delegated property panel clear to ADS system";
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

    qDebug() << "MainWindow: Custom styles applied";
}

QString MainWindow::getPortTypeDescription(QtNodes::NodeDelegateModel* nodeModel, QtNodes::PortType portType, QtNodes::PortIndex portIndex)
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

// 右键菜单实现
void MainWindow::showNodeContextMenu(QtNodes::NodeId nodeId, const QPointF& pos)
{
    m_selectedNodeId = nodeId;

    QMenu contextMenu(this);

    QAction* deleteAction = contextMenu.addAction("删除节点");
    deleteAction->setIcon(QIcon(":/icons/delete.png"));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedNode);

    QAction* duplicateAction = contextMenu.addAction("复制节点");
    duplicateAction->setIcon(QIcon(":/icons/copy.png"));
    connect(duplicateAction, &QAction::triggered, this, &MainWindow::duplicateSelectedNode);

    contextMenu.addSeparator();

    // 获取节点信息
    auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
    if (nodeDelegate) {
        QAction* infoAction = contextMenu.addAction(QString("节点: %1").arg(nodeDelegate->name()));
        infoAction->setEnabled(false);
    }

    // 转换坐标并显示菜单
    QPoint globalPos = m_graphicsView->mapToGlobal(m_graphicsView->mapFromScene(pos));
    contextMenu.exec(globalPos);
}

void MainWindow::showConnectionContextMenu(QtNodes::ConnectionId connectionId, const QPointF& pos)
{
    m_selectedConnectionId = connectionId;

    QMenu contextMenu(this);

    // 获取连接信息
    auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.outNodeId);
    auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.inNodeId);

    if (outNodeDelegate && inNodeDelegate) {
        QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out, connectionId.outPortIndex);
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

// 删除了不稳定的位置查找方法

void MainWindow::deleteSelectedNode()
{
    if (m_selectedNodeId != QtNodes::NodeId{}) {
        // 使用命令系统删除节点
        auto command = std::make_unique<DeleteNodeCommand>(m_graphicsScene, m_selectedNodeId);
        auto& commandManager = CommandManager::instance();
        
        if (commandManager.executeCommand(std::move(command))) {
            m_selectedNodeId = QtNodes::NodeId{};
        // 清空属性面板
        clearPropertyPanel();
            ui->statusbar->showMessage(tr("节点已删除"), 2000);
        } else {
            ui->statusbar->showMessage(tr("删除节点失败"), 2000);
        }
    }
}

void MainWindow::deleteSelectedConnection()
{
    // 检查是否有有效的连接被选中（简化检查）
    // 实际应用中需要根据QtNodes的ConnectionId结构来检查
    bool hasValidConnection = true; // 简化处理
    
    if (!hasValidConnection) {
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

    if (outNodeDelegate && inNodeDelegate) {
        QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out, m_selectedConnectionId.outPortIndex);
        QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In, m_selectedConnectionId.inPortIndex);

        description = QString("%1[%2:%3] → %4[%5:%6]")
            .arg(outNodeDelegate->name())
            .arg(m_selectedConnectionId.outPortIndex)
            .arg(outPortType)
            .arg(inNodeDelegate->name())
            .arg(m_selectedConnectionId.inPortIndex)
            .arg(inPortType);
    }

    if (commandManager.executeCommand(std::move(command))) {
        qDebug() << "MainWindow: Deleted connection:" << description;
        ui->statusbar->showMessage(tr("连接已删除: %1").arg(description), 3000);
    } else {
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

    for (auto nodeId : allNodes) {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (!nodeDelegate) continue;
        
        // 检查所有输出端口的连接
        unsigned int outputPorts = nodeDelegate->nPorts(QtNodes::PortType::Out);
        for (unsigned int portIndex = 0; portIndex < outputPorts; ++portIndex) {
            auto nodeConnections = m_graphModel->connections(nodeId, QtNodes::PortType::Out, portIndex);
        for (auto connectionId : nodeConnections) {
            auto outNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.outNodeId);
            auto inNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(connectionId.inNodeId);

            if (outNodeDelegate && inNodeDelegate) {
                QString outPortType = getPortTypeDescription(outNodeDelegate, QtNodes::PortType::Out, connectionId.outPortIndex);
                QString inPortType = getPortTypeDescription(inNodeDelegate, QtNodes::PortType::In, connectionId.inPortIndex);

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

    if (connections.isEmpty()) {
        QMessageBox::information(this, "提示", "没有找到可删除的连接");
        return;
    }

    bool ok;
    QString selectedConnection = QInputDialog::getItem(this, "删除连接",
        "选择要删除的连接:", connectionList, 0, false, &ok);

    if (ok && !selectedConnection.isEmpty()) {
        int index = connectionList.indexOf(selectedConnection);
        if (index >= 0 && index < connections.size()) {
            m_graphModel->deleteConnection(connections[index]);
            qDebug() << "MainWindow: Deleted connection:" << selectedConnection;
        }
    }
}

void MainWindow::duplicateSelectedNode()
{
    if (m_selectedNodeId != QtNodes::NodeId{}) {
        // 获取原节点的信息
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(m_selectedNodeId);
        if (!nodeDelegate) {
            ui->statusbar->showMessage(tr("复制节点失败：无法获取节点信息"), 2000);
            return;
        }

        // 获取节点的真实类型
        QString nodeType = nodeDelegate->name();

            // 获取原节点位置并偏移
            QVariant posVariant = m_graphModel->nodeData(m_selectedNodeId, QtNodes::NodeRole::Position);
            QPointF originalPos = posVariant.toPointF();
            QPointF newPos = originalPos + QPointF(50, 50); // 偏移50像素
        
        qDebug() << "MainWindow: Duplicating node of type:" << nodeType << "at position:" << newPos;
        
        // 使用命令系统创建相同类型的新节点
        auto command = std::make_unique<CreateNodeCommand>(m_graphicsScene, nodeType, newPos);
        auto& commandManager = CommandManager::instance();
        
        // 保存原节点的完整数据（包括属性）
        QJsonObject originalNodeData = m_graphModel->saveNode(m_selectedNodeId);
        
        if (commandManager.executeCommand(std::move(command))) {
            // 获取新创建的节点ID（应该是最后一个创建的）
            auto allNodeIds = m_graphModel->allNodeIds();
            QtNodes::NodeId newNodeId;
            
            // 找到新创建的节点（位置最接近newPos的节点）
            double minDistance = std::numeric_limits<double>::max();
            for (const auto& nodeId : allNodeIds) {
                QVariant posVar = m_graphModel->nodeData(nodeId, QtNodes::NodeRole::Position);
                QPointF nodePos = posVar.toPointF();
                double distance = QPointF(nodePos - newPos).manhattanLength();
                if (distance < minDistance) {
                    minDistance = distance;
                    newNodeId = nodeId;
                }
            }
            
            // 恢复节点属性（除了位置）
            if (newNodeId != QtNodes::NodeId{} && !originalNodeData.isEmpty()) {
                // 创建一个新的数据对象，保留新位置
                QJsonObject newNodeData = originalNodeData;
                newNodeData.remove("position"); // 移除位置信息，保持新位置
                
                // 应用属性到新节点
                auto newNodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(newNodeId);
                if (newNodeDelegate) {
                    newNodeDelegate->load(newNodeData);
                    qDebug() << "MainWindow: Copied properties to new node" << newNodeId;
                }
            }
            
            ui->statusbar->showMessage(tr("已复制 %1 节点（包含属性）").arg(nodeDelegate->caption()), 2000);
        } else {
            ui->statusbar->showMessage(tr("复制节点失败"), 2000);
        }
    }
}

void MainWindow::triggerDataFlow()
{
    qDebug() << "MainWindow: Triggering data flow";

    if (!m_graphModel) {
        qDebug() << "MainWindow: No graph model available";
        return;
    }

    // 获取所有节点ID
    auto nodeIds = m_graphModel->allNodeIds();
    qDebug() << "MainWindow: Found" << nodeIds.size() << "nodes";

    // 遍历所有节点，找到源节点（没有输入连接的节点）并触发它们
    for (const auto& nodeId : nodeIds) {
        auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
        if (!nodeDelegate) continue;

        QString nodeName = nodeDelegate->name();



        // 检查是否为源节点（没有输入端口或输入端口没有连接）
        bool isSourceNode = true;
        unsigned int inputPorts = nodeDelegate->nPorts(QtNodes::PortType::In);

        for (unsigned int portIndex = 0; portIndex < inputPorts; ++portIndex) {
            auto connections = m_graphModel->connections(nodeId, QtNodes::PortType::In, portIndex);
            if (!connections.empty()) {
                isSourceNode = false;
                break;
            }
        }

        if (isSourceNode) {
            qDebug() << "MainWindow: Found source node:" << nodeName;

            // 根据节点类型调用特定的触发方法
            if (nodeName == "OpenExcel") {
                auto* openExcelModel = m_graphModel->delegateModel<OpenExcelModel>(nodeId);
                if (openExcelModel) {
                    qDebug() << "MainWindow: Triggering OpenExcelModel execution";
                    openExcelModel->triggerExecution();
                }
            }

            // 触发源节点的数据更新
            for (unsigned int portIndex = 0; portIndex < nodeDelegate->nPorts(QtNodes::PortType::Out); ++portIndex) {
                emit nodeDelegate->dataUpdated(portIndex);
            }
        }
    }
}

void MainWindow::setupNodePalette()
{
    // 节点面板现在完全由ADS面板系统管理
    // 这个方法保留为空，保持接口兼容性
    m_nodePalette = nullptr;
    
    qDebug() << "MainWindow: 节点面板由ADS系统管理";
}

void MainWindow::setupKeyboardShortcuts()
{
    // 缩放快捷键
    QShortcut* zoomInShortcut = new QShortcut(QKeySequence("Ctrl++"), this);
    connect(zoomInShortcut, &QShortcut::activated, this, [this](){
        if (m_modernToolBar) {
            emit m_modernToolBar->zoomInRequested();
        }
    });
    
    QShortcut* zoomOutShortcut = new QShortcut(QKeySequence("Ctrl+-"), this);
    connect(zoomOutShortcut, &QShortcut::activated, this, [this](){
        if (m_modernToolBar) {
            emit m_modernToolBar->zoomOutRequested();
        }
    });
    
    QShortcut* zoomFitShortcut = new QShortcut(QKeySequence("Ctrl+0"), this);
    connect(zoomFitShortcut, &QShortcut::activated, this, [this](){
        if (m_modernToolBar) {
            emit m_modernToolBar->zoomFitRequested();
        }
    });
    
    // 重置缩放快捷键
    QShortcut* resetZoomShortcut = new QShortcut(QKeySequence("Ctrl+1"), this);
    connect(resetZoomShortcut, &QShortcut::activated, this, [this](){
        if (m_graphicsView) {
            m_graphicsView->resetTransform();
            ui->statusbar->showMessage(tr("缩放已重置为 100%"), 1000);
        }
    });
    
    qDebug() << "MainWindow: Keyboard shortcuts setup completed";
}

void MainWindow::setupLayoutMenu()
{
    // 创建视图菜单（如果不存在）
    QMenuBar* menuBar = this->menuBar();
    QMenu* viewMenu = nullptr;
    
    // 查找现有的视图菜单
    for (QAction* action : menuBar->actions()) {
        if (action->menu() && action->menu()->title() == "视图") {
            viewMenu = action->menu();
            break;
        }
    }
    
    // 如果没有视图菜单，创建一个
    if (!viewMenu) {
        viewMenu = menuBar->addMenu("视图");
    }
    
    // 只添加窗口管理功能，面板控制由ADS系统处理
    viewMenu->addSeparator();
    
    // 全屏控制
    QAction* fullScreenAction = viewMenu->addAction("🖥️ 全屏");
    fullScreenAction->setCheckable(true);
    fullScreenAction->setShortcut(QKeySequence("F11"));
    connect(fullScreenAction, &QAction::toggled, this, [this](bool fullScreen) {
        if (fullScreen) {
            showFullScreen();
        } else {
            showNormal();
        }
    });
    
    // 启动时自动恢复窗口几何布局
    QSettings settings;
    if (settings.contains("geometry")) {
        restoreGeometry(settings.value("geometry").toByteArray());
        qDebug() << "MainWindow: 窗口几何布局已从设置中恢复";
    }
    
    qDebug() << "MainWindow: 布局菜单设置完成";
}

// 节点面板信号处理
void MainWindow::onNodePaletteCreationRequested(const QString& nodeId)
{
    // 获取当前鼠标位置作为默认创建位置
    QPoint globalMousePos = QCursor::pos();
    QPoint viewPos = m_graphicsView->mapFromGlobal(globalMousePos);
    QPointF scenePos = m_graphicsView->mapToScene(viewPos);
    
    // 如果鼠标不在视图内，使用视图中心
    if (!m_graphicsView->rect().contains(viewPos)) {
        scenePos = m_graphicsView->mapToScene(m_graphicsView->rect().center());
    }
    
    createNodeFromPalette(nodeId, scenePos);
}

void MainWindow::onNodePaletteSelectionChanged(const QString& nodeId)
{
    // 显示节点信息
    NodeInfo nodeInfo = NodeCatalog::getNodeInfo(nodeId);
    if (!nodeInfo.id.isEmpty()) {
        ui->statusbar->showMessage(
            QString("选中节点: %1 - %2").arg(nodeInfo.displayName).arg(nodeInfo.description), 3000);
    }
}

void MainWindow::connectADSNodePaletteSignals()
{
    qDebug() << "MainWindow: 开始连接ADS节点面板信号";
    
    // 连接ADS节点面板的信号
    if (!m_adsPanelManager) {
        qWarning() << "MainWindow: ADS面板管理器不存在，无法连接节点面板信号";
        return;
    }
    
    qDebug() << "MainWindow: ADS面板管理器存在，获取节点面板";
    
    try {
        auto* nodePalette = m_adsPanelManager->getNodePalette();
        if (!nodePalette) {
            qWarning() << "MainWindow: 节点面板尚未创建，无法连接信号";
            return;
        }
        
        qDebug() << "MainWindow: 成功获取节点面板，开始连接信号";
        
        connect(nodePalette, &NodePalette::nodeCreationRequested, 
                this, &MainWindow::onNodePaletteCreationRequested);
        connect(nodePalette, &NodePalette::nodeSelectionChanged, 
                this, &MainWindow::onNodePaletteSelectionChanged);
        
        qDebug() << "MainWindow: ADS节点面板信号连接成功";
        
    } catch (const std::exception& e) {
        qCritical() << "MainWindow: 连接节点面板信号时发生异常:" << e.what();
    } catch (...) {
        qCritical() << "MainWindow: 连接节点面板信号时发生未知异常";
    }
}

void MainWindow::createNodeFromPalette(const QString& nodeId, const QPointF& position)
{
    qDebug() << "MainWindow: Creating node from palette:" << nodeId << "at position:" << position;
    
    auto command = std::make_unique<CreateNodeCommand>(m_graphicsScene, nodeId, position);
    auto& commandManager = CommandManager::instance();
    
    if (commandManager.executeCommand(std::move(command))) {
        NodeInfo nodeInfo = NodeCatalog::getNodeInfo(nodeId);
        ui->statusbar->showMessage(tr("已创建节点: %1").arg(nodeInfo.displayName), 2000);
    } else {
        ui->statusbar->showMessage(tr("创建节点失败"), 2000);
    }
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

    // 常用节点快速访问
    QMenu* quickAccessMenu = contextMenu.addMenu("⭐ 常用节点");
    QList<NodeInfo> favoriteNodes = NodeCatalog::getFrequentlyUsedNodes();
    for (const NodeInfo& nodeInfo : favoriteNodes) {
        QAction* action = quickAccessMenu->addAction(nodeInfo.displayName);
        action->setToolTip(nodeInfo.description);
        connect(action, &QAction::triggered, [this, nodeInfo, pos]() {
            createNodeFromPalette(nodeInfo.id, pos);
        });
    }

    contextMenu.addSeparator();

    // 按分类添加节点
    QStringList categories = NodeCatalog::getAllCategories();
    for (const QString& category : categories) {
        QMenu* categoryMenu = contextMenu.addMenu(category);
        QList<NodeInfo> categoryNodes = NodeCatalog::getNodesByCategory(category);
        
        for (const NodeInfo& nodeInfo : categoryNodes) {
            QAction* action = categoryMenu->addAction(nodeInfo.displayName);
            action->setToolTip(nodeInfo.description);
            connect(action, &QAction::triggered, [this, nodeInfo, pos]() {
                createNodeFromPalette(nodeInfo.id, pos);
            });
        }
    }

    contextMenu.addSeparator();

    // 画布操作
    QAction* clearAllAction = contextMenu.addAction("🗑️ 清空画布");
    connect(clearAllAction, &QAction::triggered, [this]() {
        if (QMessageBox::question(this, "确认", "确定要清空所有节点吗？\n此操作可以撤销。") == QMessageBox::Yes) {
            auto nodeIds = m_graphModel->allNodeIds();
            if (!nodeIds.empty()) {
                auto& commandManager = CommandManager::instance();
                commandManager.beginMacro("清空画布");
                
                for (auto nodeId : nodeIds) {
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
    qDebug() << "MainWindow: 设置ADS高级面板系统";
    
    try {
        // 创建ADS面板管理器
        m_adsPanelManager = new ADSPanelManager(this, this);
        
        // 初始化ADS系统
        m_adsPanelManager->initialize();
        
        // 连接面板事件
        connect(m_adsPanelManager, &ADSPanelManager::panelCreated,
                this, [this](const QString& panelId, ADSPanelManager::PanelType type) {
                    qDebug() << "MainWindow: ADS面板创建" << panelId << type;
                    if (ui->statusbar) {
                        ui->statusbar->showMessage(tr("面板已创建: %1").arg(panelId), 2000);
                    }
                });
        
        connect(m_adsPanelManager, &ADSPanelManager::layoutChanged,
                this, [this]() {
                    qDebug() << "MainWindow: ADS布局已更改";
                });
        
        connect(m_adsPanelManager, &ADSPanelManager::panelFocused,
                this, [this](const QString& panelId) {
                    // 当属性面板获得焦点时，确保图形模型引用是最新的
                    if (panelId == "property_panel") {
                        updatePropertyPanelReference();
                    }
                });
        
        // 关键步骤：设置中央部件
        setupADSCentralWidget();
        
        // 设置默认布局
        m_adsPanelManager->setupDefaultLayout();
        
        // 更新面板引用
        updatePropertyPanelReference();
        
        // 连接节点面板信号
        connectADSNodePaletteSignals();
        
        qDebug() << "MainWindow: ADS高级面板系统设置完成";
        
    } catch (const std::exception& e) {
        qCritical() << "MainWindow: ADS系统初始化失败:" << e.what();
        
        // 清理失败的初始化
        if (m_adsPanelManager) {
            delete m_adsPanelManager;
            m_adsPanelManager = nullptr;
        }
        
        QMessageBox::critical(this, "错误", 
            QString("ADS面板系统初始化失败：%1").arg(e.what()));
        
    } catch (...) {
        qCritical() << "MainWindow: ADS系统初始化发生未知错误";
        
        // 清理失败的初始化
        if (m_adsPanelManager) {
            delete m_adsPanelManager;
            m_adsPanelManager = nullptr;
        }
        
        QMessageBox::critical(this, "错误", "ADS面板系统初始化发生未知错误");
    }
}

void MainWindow::setupADSLayoutMenu()
{
    if (!m_adsPanelManager) return;
    
    // 创建视图菜单（如果不存在）
    QMenuBar* menuBar = this->menuBar();
    QMenu* viewMenu = nullptr;
    
    // 查找现有的视图菜单
    for (QAction* action : menuBar->actions()) {
        if (action->menu() && action->menu()->title() == "视图") {
            viewMenu = action->menu();
            break;
        }
    }
    
    // 如果没有视图菜单，创建一个
    if (!viewMenu) {
        viewMenu = menuBar->addMenu("视图");
    }
    
    // 添加ADS布局控制菜单
    QMenu* adsLayoutMenu = viewMenu->addMenu("🎛️ ADS布局");
    
    // 布局预设
    QAction* defaultLayoutAction = adsLayoutMenu->addAction("🏠 默认布局");
    connect(defaultLayoutAction, &QAction::triggered, 
            m_adsPanelManager, &ADSPanelManager::setupDefaultLayout);
    
    QAction* minimalLayoutAction = adsLayoutMenu->addAction("📦 最小化布局");
    connect(minimalLayoutAction, &QAction::triggered, 
            m_adsPanelManager, &ADSPanelManager::setupMinimalLayout);
    
    QAction* developerLayoutAction = adsLayoutMenu->addAction("🛠️ 开发者布局");
    connect(developerLayoutAction, &QAction::triggered, 
            m_adsPanelManager, &ADSPanelManager::setupDeveloperLayout);
    
    QAction* designerLayoutAction = adsLayoutMenu->addAction("🎨 设计师布局");
    connect(designerLayoutAction, &QAction::triggered, 
            m_adsPanelManager, &ADSPanelManager::setupDesignerLayout);
    
    adsLayoutMenu->addSeparator();
    
    // 面板控制
    QAction* showPropertyAction = adsLayoutMenu->addAction("🔧 显示属性面板");
    connect(showPropertyAction, &QAction::triggered, 
            [this]() { m_adsPanelManager->showPanel("property_panel"); });
    
    QAction* showNodePaletteAction = adsLayoutMenu->addAction("🗂️ 显示节点面板");
    connect(showNodePaletteAction, &QAction::triggered, 
            [this]() { m_adsPanelManager->showPanel("node_palette"); });
    
    QAction* showOutputAction = adsLayoutMenu->addAction("💻 显示输出控制台");
    connect(showOutputAction, &QAction::triggered, 
            [this]() { m_adsPanelManager->showPanel("output_console"); });
    
    adsLayoutMenu->addSeparator();
    
    // 布局管理
    QAction* saveLayoutAction = adsLayoutMenu->addAction("💾 保存当前布局");
    connect(saveLayoutAction, &QAction::triggered, 
            [this]() { 
                QString layoutName = QString("user_layout_%1").arg(
                    QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
                m_adsPanelManager->saveLayoutPreset(layoutName);
                ui->statusbar->showMessage(tr("布局已保存: %1").arg(layoutName), 3000);
            });
    
    QAction* resetLayoutAction = adsLayoutMenu->addAction("🔄 重置到默认布局");
    connect(resetLayoutAction, &QAction::triggered, 
            m_adsPanelManager, &ADSPanelManager::resetToDefaultLayout);
    
    qDebug() << "MainWindow: ADS布局菜单设置完成";
}

void MainWindow::updateADSPropertyPanel(QtNodes::NodeId nodeId)
{
    if (!m_adsPanelManager) return;
    
    // 确保属性面板引用是最新的
    updatePropertyPanelReference();
    
    // 确保属性面板可见
    m_adsPanelManager->showPanel("property_panel");
    
    // 更新属性面板内容
    if (m_propertyPanelContainer) {
        m_propertyPanelContainer->updateNodeProperties(nodeId);
        
        // 获取ADS面板并更新标题
        if (auto* propertyPanel = m_adsPanelManager->getPanel("property_panel")) {
            auto nodeDelegate = m_graphModel->delegateModel<QtNodes::NodeDelegateModel>(nodeId);
            if (nodeDelegate) {
                QString newTitle = QString("🔧 属性面板 - %1").arg(nodeDelegate->caption());
                propertyPanel->setWindowTitle(newTitle);
            }
        }
    }
}

void MainWindow::clearADSPropertyPanel()
{
    // 确保属性面板引用是最新的
    updatePropertyPanelReference();
    
    if (m_propertyPanelContainer) {
        m_propertyPanelContainer->clearProperties();
        
        // 重置ADS面板标题
        if (m_adsPanelManager) {
            if (auto* propertyPanel = m_adsPanelManager->getPanel("property_panel")) {
                propertyPanel->setWindowTitle("🔧 属性面板");
            }
        }
    }
}

void MainWindow::setupADSCentralWidget()
{
    qDebug() << "MainWindow: 开始设置ADS中央部件";
    
    if (!m_adsPanelManager) {
        qCritical() << "MainWindow: ADS面板管理器不存在";
        return;
    }
    
    if (!m_graphicsView) {
        qCritical() << "MainWindow: 图形视图不存在";
        return;
    }
    
    auto* dockManager = m_adsPanelManager->dockManager();
    if (!dockManager) {
        qCritical() << "MainWindow: DockManager不存在";
        return;
    }
    
    // 按照ADS标准方式：CDockManager已经接管了MainWindow
    // 现在只需要创建中央部件并设置给DockManager
    
    // 创建中央停靠部件
    auto* centralDockWidget = new ads::CDockWidget("节点编辑器", dockManager);
    centralDockWidget->setWidget(m_graphicsView);
    centralDockWidget->setObjectName("central_editor");
    
    // 设置中央部件属性（不可关闭、不可移动、不可浮动）
    centralDockWidget->setFeature(ads::CDockWidget::DockWidgetClosable, false);
    centralDockWidget->setFeature(ads::CDockWidget::DockWidgetMovable, false);
    centralDockWidget->setFeature(ads::CDockWidget::DockWidgetFloatable, false);
    
    qDebug() << "MainWindow: 中央停靠部件已创建";
    
    // 使用ADS的setCentralWidget API
    auto* centralArea = dockManager->setCentralWidget(centralDockWidget);
    
    if (centralArea) {
        qDebug() << "MainWindow: 成功设置ADS中央部件";
        
        // 设置中央区域的允许停靠区域
        centralArea->setAllowedAreas(ads::DockWidgetArea::OuterDockAreas);
        
        // 确保组件正确显示
        centralDockWidget->show();
        m_graphicsView->show();
        
        qDebug() << "MainWindow: ADS中央部件设置完成";
    } else {
        qCritical() << "MainWindow: 设置ADS中央部件失败";
    }
}

void MainWindow::updatePropertyPanelReference()
{
    qDebug() << "MainWindow: 开始更新属性面板引用";
    
    // 检查ADS面板管理器
    if (!m_adsPanelManager) {
        qWarning() << "MainWindow: ADS面板管理器不存在";
        return;
    }
    
    qDebug() << "MainWindow: ADS面板管理器存在，获取属性面板容器";
    
    // 获取属性面板容器引用
    try {
        m_propertyPanelContainer = m_adsPanelManager->getPropertyPanelContainer();
        
        // 添加空指针检查
        if (!m_propertyPanelContainer) {
            qWarning() << "MainWindow: 属性面板容器尚未创建";
            return;
        }
        
        qDebug() << "MainWindow: 成功获取属性面板容器引用";
        
        // 检查图形模型
        if (!m_graphModel) {
            qWarning() << "MainWindow: 图形模型尚未创建，无法设置到属性面板";
            return;
        }
        
        qDebug() << "MainWindow: 图形模型存在，设置到属性面板";
        
        // 设置图形模型到属性面板
        m_propertyPanelContainer->setGraphModel(m_graphModel.get());
        qDebug() << "MainWindow: 属性面板引用已同步并设置图形模型";
        
    } catch (const std::exception& e) {
        qCritical() << "MainWindow: 更新属性面板引用时发生异常:" << e.what();
    } catch (...) {
        qCritical() << "MainWindow: 更新属性面板引用时发生未知异常";
    }
}
