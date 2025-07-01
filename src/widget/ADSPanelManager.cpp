//
// Created by wuxianggujun on 25-7-1.
//

#include "widget/ADSPanelManager.hpp"
#include "widget/PropertyPanelContainer.hpp"
#include "widget/CommandHistoryWidget.hpp"
#include "NodePalette.hpp"

// ADS库头文件
#include <DockManager.h>
#include <DockWidget.h>
#include <DockAreaWidget.h>
#include <DockComponentsFactory.h>

#include <QApplication>
#include <QMainWindow>
#include <QTextEdit>
#include <QTreeView>
#include <QFileSystemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIcon>
#include <QDebug>
#include <QTimer>

ADSPanelManager::ADSPanelManager(QMainWindow* mainWindow, QObject* parent)
    : QObject(parent)
    , m_mainWindow(mainWindow)
    , m_dockManager(nullptr)
    , m_propertyPanelContainer(nullptr)
    , m_nodePalette(nullptr)
    , m_commandHistoryWidget(nullptr)
{
    qDebug() << "ADSPanelManager: 创建面板管理器";
}

ADSPanelManager::~ADSPanelManager()
{
    qDebug() << "ADSPanelManager: 开始销毁面板管理器";
    
    // 安全关闭所有面板
    if (m_dockManager) {
        // 先断开所有信号连接，避免在销毁过程中触发信号
        m_dockManager->disconnect();
        
        // 移除所有面板
        QList<ads::CDockWidget*> panels = m_panels.values();
        for (auto* panel : panels) {
            if (panel) {
                panel->disconnect(); // 断开面板的信号连接
                if (panel->dockManager() == m_dockManager) {
                    m_dockManager->removeDockWidget(panel);
                }
            }
        }
        
        // 清理注册表
        m_panels.clear();
        m_panelTypes.clear();
        
        // 重置指针
        m_propertyPanelContainer = nullptr;
        m_nodePalette = nullptr;
        m_commandHistoryWidget = nullptr;
    }
    
    qDebug() << "ADSPanelManager: 面板管理器销毁完成";
}

void ADSPanelManager::initialize()
{
    qDebug() << "ADSPanelManager: 初始化ADS系统";
    
    // 创建ADS停靠管理器
    setupDockManager();
    
    // 加载布局预设
    loadLayoutPresets();
    
    // 设置面板连接
    setupPanelConnections();
    
    qDebug() << "ADSPanelManager: ADS系统初始化完成";
}

void ADSPanelManager::shutdown()
{
    if (m_dockManager) {
        // 保存当前布局
        saveLayoutPresets();
        
        // 清理面板
        m_panels.clear();
        m_panelTypes.clear();
        
        // ADS会自动清理其子组件
        m_dockManager = nullptr;
    }
}

void ADSPanelManager::setupDockManager()
{
    // 配置ADS特性
    ads::CDockManager::setConfigFlag(ads::CDockManager::OpaqueSplitterResize, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::XmlCompressionEnabled, false);
    ads::CDockManager::setConfigFlag(ads::CDockManager::FocusHighlighting, true);
    
    // 启用高级功能
    ads::CDockManager::setConfigFlag(ads::CDockManager::DragPreviewIsDynamic, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DragPreviewShowsContentPixmap, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DragPreviewHasWindowFrame, false);
    
    // 启用Auto-Hide功能
    ads::CDockManager::setAutoHideConfigFlag(ads::CDockManager::AutoHideFeatureEnabled, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaHideDisabledButtons, true);
    ads::CDockManager::setConfigFlag(ads::CDockManager::DockAreaDynamicTabsMenuButtonVisibility, true);
    
    // 关键修复：CDockManager应该直接接管MainWindow，而不是作为子部件
    m_dockManager = new ads::CDockManager(m_mainWindow);
    
    // 设置样式
    setupADSStyle();
    
    qDebug() << "ADSPanelManager: ADS停靠管理器创建完成，已接管主窗口";
}

void ADSPanelManager::setupADSStyle()
{
    // 设置ADS样式表
    QString adsStyleSheet = R"(
        ads--CDockWidget {
            background: #2b2b2b;
            color: #cccccc;
            border: 1px solid #464646;
        }
        
        ads--CDockWidget[focused="true"] {
            border: 2px solid #007acc;
        }
        
        ads--CTitleBarButton {
            background: transparent;
            border: none;
            padding: 2px;
        }
        
        ads--CTitleBarButton:hover {
            background: #464646;
            border-radius: 2px;
        }
        
        ads--CTitleBarButton:pressed {
            background: #007acc;
        }
        
        ads--CDockAreaWidget {
            background: #2b2b2b;
            border: 1px solid #464646;
        }
        
        ads--CDockAreaTitleBar {
            background: #383838;
            border-bottom: 1px solid #464646;
        }
        
        ads--CTitleBarButton#tabsMenuButton::menu-indicator {
            image: none;
        }
        
        ads--CTabBar {
            background: #383838;
        }
        
        ads--CTabBar::tab {
            background: #383838;
            color: #cccccc;
            border: none;
            padding: 6px 12px;
        }
        
        ads--CTabBar::tab:selected {
            background: #007acc;
            color: white;
        }
        
        ads--CTabBar::tab:hover {
            background: #464646;
        }
        
        ads--CSplitter::handle {
            background: #464646;
        }
        
        ads--CSplitter::handle:horizontal {
            width: 2px;
        }
        
        ads--CSplitter::handle:vertical {
            height: 2px;
        }
        
        ads--CAutoHideTab {
            background: #383838;
            color: #cccccc;
            border: 1px solid #464646;
        }
        
        ads--CAutoHideTab:hover {
            background: #464646;
        }
        
        ads--CAutoHideTab[activeTab="true"] {
            background: #007acc;
            color: white;
        }
    )";
    
    m_dockManager->setStyleSheet(adsStyleSheet);
}

void ADSPanelManager::setupPanelConnections()
{
    if (!m_dockManager) return;
    
    // 连接ADS信号
    connect(m_dockManager, &ads::CDockManager::focusedDockWidgetChanged,
            this, [this](ads::CDockWidget* old, ads::CDockWidget* now) {
                Q_UNUSED(old);
                if (now) {
                    onFocusChanged(now);
                }
            });
    
    connect(m_dockManager, &ads::CDockManager::dockWidgetAdded,
            this, [this](ads::CDockWidget* dockWidget) {
                QString panelId = dockWidget->objectName();
                PanelType type = m_panelTypes.value(panelId, CustomPanel);
                emit panelCreated(panelId, type);
                qDebug() << "ADSPanelManager: 面板添加" << panelId;
            });
    
    connect(m_dockManager, &ads::CDockManager::dockWidgetRemoved,
            this, [this](ads::CDockWidget* dockWidget) {
                QString panelId = dockWidget->objectName();
                m_panels.remove(panelId);
                m_panelTypes.remove(panelId);
                emit panelDestroyed(panelId);
                qDebug() << "ADSPanelManager: 面板移除" << panelId;
            });
}

ads::CDockWidget* ADSPanelManager::createPanel(PanelType type, const QString& panelId, const QString& title)
{
    if (m_panels.contains(panelId)) {
        qWarning() << "ADSPanelManager: 面板已存在" << panelId;
        return m_panels[panelId];
    }
    
    // 创建面板内容
    QWidget* content = createPanelContent(type);
    if (!content) {
        qWarning() << "ADSPanelManager: 无法创建面板内容" << panelId;
        return nullptr;
    }
    
    // 创建ADS停靠面板 - 重要：设置正确的父对象
    ads::CDockWidget* dockWidget = new ads::CDockWidget(title, m_dockManager);
    dockWidget->setObjectName(panelId);
    dockWidget->setWidget(content);
    dockWidget->setIcon(getPanelIcon(type));
    
    // 配置面板属性
    configurePanelProperties(dockWidget, type);
    
    // 注册面板
    m_panels[panelId] = dockWidget;
    m_panelTypes[panelId] = type;
    
    // 连接面板信号
    connect(dockWidget, &ads::CDockWidget::visibilityChanged,
            this, [this, panelId](bool visible) {
                emit panelVisibilityChanged(panelId, visible);
            });
    
    connect(dockWidget, &ads::CDockWidget::closed,
            this, [this, dockWidget]() {
                onPanelClosed(dockWidget);
            });
    
    qDebug() << "ADSPanelManager: 创建面板" << panelId << title;
    return dockWidget;
}

void ADSPanelManager::removePanel(const QString& panelId)
{
    if (!m_panels.contains(panelId)) {
        qWarning() << "ADSPanelManager: 面板不存在" << panelId;
        return;
    }
    
    ads::CDockWidget* dockWidget = m_panels[panelId];
    m_dockManager->removeDockWidget(dockWidget);
    dockWidget->deleteLater();
    
    m_panels.remove(panelId);
    m_panelTypes.remove(panelId);
    
    qDebug() << "ADSPanelManager: 移除面板" << panelId;
}

ads::CDockWidget* ADSPanelManager::getPanel(const QString& panelId) const
{
    return m_panels.value(panelId, nullptr);
}

QList<ads::CDockWidget*> ADSPanelManager::getAllPanels() const
{
    return m_panels.values();
}

// 专用面板创建方法实现
ads::CDockWidget* ADSPanelManager::createPropertyPanel()
{
    return createPanel(PropertyPanel, "property_panel", "🔧 属性面板");
}

ads::CDockWidget* ADSPanelManager::createNodePalettePanel()
{
    return createPanel(NodePalettePanel, "node_palette", "🗂️ 节点面板");
}

ads::CDockWidget* ADSPanelManager::createCommandHistoryPanel()
{
    return createPanel(CommandHistory, "command_history", "📜 命令历史");
}

ads::CDockWidget* ADSPanelManager::createOutputConsolePanel()
{
    return createPanel(OutputConsole, "output_console", "💻 输出控制台");
}

ads::CDockWidget* ADSPanelManager::createProjectExplorerPanel()
{
    return createPanel(ProjectExplorer, "project_explorer", "📁 项目浏览器");
}

// 布局管理实现
void ADSPanelManager::setupDefaultLayout()
{
    if (!m_dockManager) {
        qCritical() << "ADSPanelManager: DockManager 不存在，无法设置布局";
        return;
    }
    
    qDebug() << "ADSPanelManager: 开始设置默认布局";
    
    // 清理现有面板（安全删除）
    QStringList panelIds = m_panels.keys();
    for (const QString& panelId : panelIds) {
        auto* panel = m_panels.value(panelId);
        if (panel) {
            if (panel->dockManager()) {
                m_dockManager->removeDockWidget(panel);
            }
            // 从注册表中移除，但不删除对象，让ADS系统管理
            m_panels.remove(panelId);
            m_panelTypes.remove(panelId);
        }
    }
    
    // 重新创建所有面板
    auto* propertyPanel = createPropertyPanel();
    auto* nodePanel = createNodePalettePanel();
    auto* historyPanel = createCommandHistoryPanel();
    auto* outputPanel = createOutputConsolePanel();
    
    // 检查面板创建是否成功
    if (!propertyPanel || !nodePanel || !historyPanel || !outputPanel) {
        qCritical() << "ADSPanelManager: 面板创建失败，无法设置布局";
        return;
    }
    
    qDebug() << "ADSPanelManager: 所有面板创建成功，开始添加到布局";
    
    // 按布局添加面板
    try {
        // 左侧：节点面板
        m_dockManager->addDockWidget(ads::LeftDockWidgetArea, nodePanel);
        qDebug() << "ADSPanelManager: 节点面板添加完成";
        
        // 右侧：属性面板
        m_dockManager->addDockWidget(ads::RightDockWidgetArea, propertyPanel);
        qDebug() << "ADSPanelManager: 属性面板添加完成";
        
        // 右侧：命令历史面板（添加到属性面板的同一区域作为标签页）
        m_dockManager->addDockWidgetTabToArea(historyPanel, propertyPanel->dockAreaWidget());
        qDebug() << "ADSPanelManager: 命令历史面板添加完成";
        
        // 底部：输出面板
        m_dockManager->addDockWidget(ads::BottomDockWidgetArea, outputPanel);
        qDebug() << "ADSPanelManager: 输出面板添加完成";
        
        // 设置默认激活的标签页
        if (propertyPanel->dockAreaWidget()) {
            propertyPanel->dockAreaWidget()->setCurrentDockWidget(propertyPanel);
        }
        
        // 确保所有面板可见
        propertyPanel->show();
        nodePanel->show();
        historyPanel->show();
        outputPanel->show();
        
        qDebug() << "ADSPanelManager: 默认布局设置完成";
        
    } catch (const std::exception& e) {
        qCritical() << "ADSPanelManager: 设置布局时发生异常:" << e.what();
    } catch (...) {
        qCritical() << "ADSPanelManager: 设置布局时发生未知异常";
    }
}

void ADSPanelManager::setupMinimalLayout()
{
    if (!m_dockManager) return;
    
    // 只创建核心面板
    auto* propertyPanel = createPropertyPanel();
    auto* nodePanel = createNodePalettePanel();
    
    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, nodePanel);
    m_dockManager->addDockWidget(ads::RightDockWidgetArea, propertyPanel);
    
    // 其他面板设为Auto-Hide
    auto* historyPanel = createCommandHistoryPanel();
    auto* outputPanel = createOutputConsolePanel();
    
    m_dockManager->addAutoHideDockWidget(ads::SideBarBottom, historyPanel);
    m_dockManager->addAutoHideDockWidget(ads::SideBarBottom, outputPanel);
    
    qDebug() << "ADSPanelManager: 最小化布局设置完成";
}

void ADSPanelManager::setupDeveloperLayout()
{
    if (!m_dockManager) return;
    
    // 开发者布局 - 更多调试面板
    auto* propertyPanel = createPropertyPanel();
    auto* nodePanel = createNodePalettePanel();
    auto* historyPanel = createCommandHistoryPanel();
    auto* outputPanel = createOutputConsolePanel();
    auto* explorerPanel = createProjectExplorerPanel();
    
    // 左侧：节点面板 + 项目浏览器
    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, nodePanel);
    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, explorerPanel);
    
    // 右侧：属性面板
    m_dockManager->addDockWidget(ads::RightDockWidgetArea, propertyPanel);
    
    // 底部：输出控制台 + 命令历史
    m_dockManager->addDockWidget(ads::BottomDockWidgetArea, outputPanel);
    m_dockManager->addDockWidgetTabToArea(historyPanel, outputPanel->dockAreaWidget());
    
    qDebug() << "ADSPanelManager: 开发者布局设置完成";
}

void ADSPanelManager::setupDesignerLayout()
{
    if (!m_dockManager) return;
    
    // 设计师布局 - 突出视觉编辑
    auto* propertyPanel = createPropertyPanel();
    auto* nodePanel = createNodePalettePanel();
    
    // 主要工作区域最大化，面板最小化
    m_dockManager->addDockWidget(ads::LeftDockWidgetArea, nodePanel);
    m_dockManager->addDockWidget(ads::RightDockWidgetArea, propertyPanel);
    
    // 其他面板都设为Auto-Hide
    auto* historyPanel = createCommandHistoryPanel();
    auto* outputPanel = createOutputConsolePanel();
    
    m_dockManager->addAutoHideDockWidget(ads::SideBarBottom, historyPanel);
    m_dockManager->addAutoHideDockWidget(ads::SideBarBottom, outputPanel);
    
    // 调整面板大小比例
    propertyPanel->dockAreaWidget()->setCurrentIndex(0);
    
    qDebug() << "ADSPanelManager: 设计师布局设置完成";
}

// 面板内容创建实现
QWidget* ADSPanelManager::createPanelContent(PanelType type)
{
    switch (type) {
    case PropertyPanel:
        if (!m_propertyPanelContainer) {
            m_propertyPanelContainer = new PropertyPanelContainer(m_mainWindow);
        }
        return m_propertyPanelContainer;
        
    case NodePalettePanel:
        if (!m_nodePalette) {
            m_nodePalette = new ::NodePalette(m_mainWindow); // 设置正确的父对象
        }
        return m_nodePalette;
        
    case CommandHistory:
        if (!m_commandHistoryWidget) {
            m_commandHistoryWidget = new CommandHistoryWidget(m_mainWindow);
        }
        return m_commandHistoryWidget;
        
    case OutputConsole:
        return createOutputConsoleWidget();
        
    case ProjectExplorer:
        return createProjectExplorerWidget();
        
    default:
        qWarning() << "ADSPanelManager: 未知面板类型" << type;
        return nullptr;
    }
}

QWidget* ADSPanelManager::createOutputConsoleWidget()
{
    // 设置正确的父对象，确保生命周期管理
    auto* widget = new QWidget(m_mainWindow);
    auto* layout = new QVBoxLayout(widget);
    
    // 创建控制栏
    auto* controlLayout = new QHBoxLayout();
    auto* levelFilter = new QComboBox(widget); // 设置父对象
    levelFilter->addItems({"全部", "调试", "信息", "警告", "错误"});
    
    auto* clearButton = new QPushButton("清空", widget); // 设置父对象
    
    controlLayout->addWidget(new QLabel("日志级别:", widget)); // 设置父对象
    controlLayout->addWidget(levelFilter);
    controlLayout->addStretch();
    controlLayout->addWidget(clearButton);
    
    // 创建输出文本区域
    auto* outputText = new QTextEdit(widget); // 设置父对象
    outputText->setReadOnly(true);
    outputText->setFont(QFont("Consolas", 9));
    outputText->setStyleSheet(
        "QTextEdit {"
        "background: #1e1e1e;"
        "color: #d4d4d4;"
        "border: 1px solid #464646;"
        "}"
    );
    
    layout->addLayout(controlLayout);
    layout->addWidget(outputText);
    
    // 连接信号
    connect(clearButton, &QPushButton::clicked, outputText, &QTextEdit::clear);
    
    // 添加一些示例输出
    outputText->append("[INFO] TinaFlow 启动完成");
    outputText->append("[DEBUG] ADS面板系统初始化");
    outputText->append("[INFO] 节点编辑器准备就绪");
    
    return widget;
}

QWidget* ADSPanelManager::createProjectExplorerWidget()
{
    // 设置正确的父对象，确保生命周期管理
    auto* widget = new QWidget(m_mainWindow);
    auto* layout = new QVBoxLayout(widget);
    
    // 创建文件树视图
    auto* treeView = new QTreeView(widget); // 设置父对象
    auto* fileModel = new QFileSystemModel(widget); // 设置父对象，这很重要！
    
    // 设置模型
    fileModel->setRootPath(QApplication::applicationDirPath());
    treeView->setModel(fileModel);
    treeView->setRootIndex(fileModel->index(QApplication::applicationDirPath()));
    
    // 隐藏不需要的列
    treeView->hideColumn(1); // Size
    treeView->hideColumn(2); // Type  
    treeView->hideColumn(3); // Date Modified
    
    layout->addWidget(new QLabel("项目文件:", widget)); // 设置父对象
    layout->addWidget(treeView);
    
    return widget;
}

void ADSPanelManager::configurePanelProperties(ads::CDockWidget* panel, PanelType type)
{
    if (!panel) return;
    
    switch (type) {
    case PropertyPanel:
        panel->setFeature(ads::CDockWidget::DockWidgetClosable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetMovable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetFloatable, true);
        break;
        
    case NodePalettePanel:
        panel->setFeature(ads::CDockWidget::DockWidgetClosable, false); // 节点面板不可关闭
        panel->setFeature(ads::CDockWidget::DockWidgetMovable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetFloatable, true);
        break;
        
    case CommandHistory:
        panel->setFeature(ads::CDockWidget::DockWidgetClosable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetMovable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetFloatable, true);
        break;
        
    case OutputConsole:
        panel->setFeature(ads::CDockWidget::DockWidgetClosable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetMovable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetFloatable, true);
        break;
        
    case ProjectExplorer:
        panel->setFeature(ads::CDockWidget::DockWidgetClosable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetMovable, true);
        panel->setFeature(ads::CDockWidget::DockWidgetFloatable, true);
        break;
        
    default:
        break;
    }
}

QString ADSPanelManager::getPanelTitle(PanelType type) const
{
    switch (type) {
    case PropertyPanel: return "🔧 属性面板";
    case NodePalettePanel: return "🗂️ 节点面板";
    case CommandHistory: return "📜 命令历史";
    case OutputConsole: return "💻 输出控制台";
    case ProjectExplorer: return "📁 项目浏览器";
    case DebugConsole: return "🐛 调试控制台";
    default: return "自定义面板";
    }
}

QIcon ADSPanelManager::getPanelIcon(PanelType type) const
{
    // 这里可以加载真实的图标文件
    // 暂时返回空图标
    Q_UNUSED(type);
    return QIcon();
}

// 面板控制实现
void ADSPanelManager::showPanel(const QString& panelId)
{
    if (auto* panel = getPanel(panelId)) {
        panel->show();
        panel->raise();
        panel->setFocus();
    }
}

void ADSPanelManager::hidePanel(const QString& panelId)
{
    if (auto* panel = getPanel(panelId)) {
        panel->hide();
    }
}

void ADSPanelManager::togglePanel(const QString& panelId)
{
    if (auto* panel = getPanel(panelId)) {
        if (panel->isVisible()) {
            panel->hide();
        } else {
            panel->show();
            panel->raise();
            panel->setFocus();
        }
    }
}

void ADSPanelManager::focusPanel(const QString& panelId)
{
    if (auto* panel = getPanel(panelId)) {
        panel->raise();
        panel->setFocus();
        if (panel->dockAreaWidget()) {
            panel->dockAreaWidget()->setCurrentDockWidget(panel);
        }
    }
}

// 布局预设实现
void ADSPanelManager::saveLayoutPreset(const QString& name)
{
    if (!m_dockManager) return;
    
    QByteArray layoutData = m_dockManager->saveState();
    QJsonObject layoutObj;
    layoutObj["data"] = QString::fromUtf8(layoutData.toBase64());
    layoutObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    m_layoutPresets[name] = layoutObj;
    saveLayoutPresets();
    
    emit layoutPresetChanged(name);
    qDebug() << "ADSPanelManager: 保存布局预设" << name;
}

void ADSPanelManager::loadLayoutPreset(const QString& name)
{
    if (!m_dockManager || !m_layoutPresets.contains(name)) {
        qWarning() << "ADSPanelManager: 布局预设不存在" << name;
        return;
    }
    
    QJsonObject layoutObj = m_layoutPresets[name];
    QByteArray layoutData = QByteArray::fromBase64(layoutObj["data"].toString().toUtf8());
    
    m_dockManager->restoreState(layoutData);
    
    emit layoutPresetChanged(name);
    qDebug() << "ADSPanelManager: 加载布局预设" << name;
}

void ADSPanelManager::deleteLayoutPreset(const QString& name)
{
    int removedCount = m_layoutPresets.remove(name);
    if (removedCount > 0) {
        saveLayoutPresets();
        qDebug() << "ADSPanelManager: 删除布局预设" << name;
    }
}

QStringList ADSPanelManager::getLayoutPresets() const
{
    return m_layoutPresets.keys();
}

void ADSPanelManager::loadLayoutPresets()
{
    QSettings settings;
    settings.beginGroup("LayoutPresets");
    
    for (const QString& name : settings.childKeys()) {
        QJsonDocument doc = QJsonDocument::fromJson(settings.value(name).toByteArray());
        m_layoutPresets[name] = doc.object();
    }
    
    settings.endGroup();
    qDebug() << "ADSPanelManager: 加载了" << m_layoutPresets.size() << "个布局预设";
}

void ADSPanelManager::saveLayoutPresets()
{
    QSettings settings;
    settings.beginGroup("LayoutPresets");
    settings.clear();
    
    for (auto it = m_layoutPresets.begin(); it != m_layoutPresets.end(); ++it) {
        QJsonDocument doc(it.value());
        settings.setValue(it.key(), doc.toJson(QJsonDocument::Compact));
    }
    
    settings.endGroup();
}

// 状态管理实现
QJsonObject ADSPanelManager::saveState() const
{
    QJsonObject state;
    
    if (m_dockManager) {
        QByteArray layoutData = m_dockManager->saveState();
        state["layout"] = QString::fromUtf8(layoutData.toBase64());
    }
    
    state["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    state["version"] = "1.0";
    
    return state;
}

void ADSPanelManager::restoreState(const QJsonObject& state)
{
    if (!m_dockManager || !state.contains("layout")) {
        qWarning() << "ADSPanelManager: 无效的状态数据";
        return;
    }
    
    QByteArray layoutData = QByteArray::fromBase64(state["layout"].toString().toUtf8());
    m_dockManager->restoreState(layoutData);
    
    qDebug() << "ADSPanelManager: 恢复状态完成";
}

// 插槽实现
void ADSPanelManager::resetToDefaultLayout()
{
    if (m_dockManager) {
        // 清除当前布局
        for (auto* panel : m_panels.values()) {
            m_dockManager->removeDockWidget(panel);
        }
        
        // 重新设置默认布局
        setupDefaultLayout();
    }
}

void ADSPanelManager::saveCurrentLayout()
{
    QString name = QString("auto_save_%1").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    saveLayoutPreset(name);
}

void ADSPanelManager::restoreLastLayout()
{
    QStringList presets = getLayoutPresets();
    if (!presets.isEmpty()) {
        // 加载最新的预设
        std::sort(presets.begin(), presets.end(), std::greater<QString>());
        loadLayoutPreset(presets.first());
    }
}

void ADSPanelManager::onPanelClosed(ads::CDockWidget* panel)
{
    if (panel) {
        QString panelId = panel->objectName();
        emit panelVisibilityChanged(panelId, false);
        qDebug() << "ADSPanelManager: 面板关闭" << panelId;
    }
}

void ADSPanelManager::onPanelOpened(ads::CDockWidget* panel)
{
    if (panel) {
        QString panelId = panel->objectName();
        emit panelVisibilityChanged(panelId, true);
        qDebug() << "ADSPanelManager: 面板打开" << panelId;
    }
}

void ADSPanelManager::onFocusChanged(ads::CDockWidget* panel)
{
    if (panel) {
        QString panelId = panel->objectName();
        emit panelFocused(panelId);
        qDebug() << "ADSPanelManager: 面板获得焦点" << panelId;
    }
} 
