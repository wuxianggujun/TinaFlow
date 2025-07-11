//
// Created by wuxianggujun on 25-7-3.
//

#include "widget/BlockProgrammingView.hpp"
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QAction>
#include <QIcon>
#include "BgfxBlockRenderer.hpp"

BlockProgrammingView::BlockProgrammingView(QWidget* parent)
    : QWidget(parent), m_scriptName("未命名脚本")
{
    setupUI();
    qDebug() << "BlockProgrammingView created";
}

void BlockProgrammingView::setupUI()
{
    setWindowTitle("积木编程视图");
    setMinimumSize(800, 600);
    
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);
    
    // 设置工具栏
    setupToolBar();
    
    // 设置主要内容区域
    m_splitter = new QSplitter(Qt::Horizontal);
    
    // 设置积木调色板
    setupBlockPalette();
    
    // 设置工作区域
    setupWorkspace();
    
    m_splitter->addWidget(m_paletteWidget);
    m_splitter->addWidget(m_workspace);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({200, 600});
    
    m_mainLayout->addWidget(m_splitter);
    
    // 设置状态栏
    setupStatusBar();
}

void BlockProgrammingView::setupToolBar()
{
    m_toolBar = new QToolBar("积木编程工具栏");
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    // 新建积木块
    auto* newAction = m_toolBar->addAction("新建积木");
    newAction->setIcon(QIcon(":/icons/actions/plus"));
    connect(newAction, &QAction::triggered, this, &BlockProgrammingView::onNewBlock);

    // 删除积木块
    auto* deleteAction = m_toolBar->addAction("删除积木");
    deleteAction->setIcon(QIcon(":/icons/actions/trash"));
    connect(deleteAction, &QAction::triggered, this, &BlockProgrammingView::onDeleteBlock);

    m_toolBar->addSeparator();

    // 清空所有
    auto* clearAction = m_toolBar->addAction("清空所有");
    clearAction->setIcon(QIcon(":/icons/actions/trash")); // 使用trash图标代替clear
    connect(clearAction, &QAction::triggered, this, &BlockProgrammingView::onClearAll);

    m_toolBar->addSeparator();

    // 运行脚本
    auto* runAction = m_toolBar->addAction("运行脚本");
    runAction->setIcon(QIcon(":/icons/actions/play"));
    connect(runAction, &QAction::triggered, this, &BlockProgrammingView::onRunScript);

    m_toolBar->addSeparator();

    // 重置视图
    auto* resetViewAction = m_toolBar->addAction("重置视图");
    resetViewAction->setIcon(QIcon(":/icons/actions/home")); // 使用home图标
    connect(resetViewAction, &QAction::triggered, [this]() {
        if (m_bgfxRenderer) {
            m_bgfxRenderer->resetView();
        }
    });

    m_toolBar->addSeparator();

    // 保存脚本
    auto* saveAction = m_toolBar->addAction("保存脚本");
    saveAction->setIcon(QIcon(":/icons/files/save"));
    connect(saveAction, &QAction::triggered, this, &BlockProgrammingView::saveScript);

    // 关闭视图
    auto* closeAction = m_toolBar->addAction("关闭");
    closeAction->setIcon(QIcon(":/icons/ui/maximize")); // 使用maximize图标代替close
    connect(closeAction, &QAction::triggered, this, &BlockProgrammingView::closeView);
    
    m_mainLayout->addWidget(m_toolBar);
}

void BlockProgrammingView::setupBlockPalette()
{
    m_paletteWidget = new QWidget();
    m_paletteWidget->setMaximumWidth(250);
    m_paletteWidget->setMinimumWidth(200);
    
    auto* layout = new QVBoxLayout(m_paletteWidget);
    layout->setContentsMargins(5, 5, 5, 5);
    
    // 标题
    auto* title = new QLabel("积木工具箱");
    title->setStyleSheet("QLabel { font-weight: bold; font-size: 14px; color: #333; padding: 5px; }");
    title->setAlignment(Qt::AlignCenter);
    layout->addWidget(title);
    
    // 积木列表
    m_paletteList = new QListWidget();
    m_paletteList->setStyleSheet(
        "QListWidget {"
        "  border: 1px solid #ccc;"
        "  border-radius: 5px;"
        "  background-color: #f9f9f9;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  margin: 2px;"
        "  border-radius: 3px;"
        "}"
        "QListWidget::item:hover {"
        "  background-color: #e3f2fd;"
        "}"
        "QListWidget::item:selected {"
        "  background-color: #2196f3;"
        "  color: white;"
        "}"
    );
    
    // 添加积木块类型
    m_paletteList->addItem("🔄 如果...那么...");
    m_paletteList->addItem("🔁 重复...次");
    m_paletteList->addItem("📋 对于每个...");
    m_paletteList->addItem("📝 设置变量");
    m_paletteList->addItem("📊 获取单元格");
    m_paletteList->addItem("✏️ 设置单元格");
    m_paletteList->addItem("⚖️ 比较");
    m_paletteList->addItem("🧮 数学运算");
    m_paletteList->addItem("🔍 查找文本");
    m_paletteList->addItem("➕ 添加行");
    m_paletteList->addItem("➖ 删除行");
    m_paletteList->addItem("📥 输入端口");
    m_paletteList->addItem("📤 输出端口");
    
    layout->addWidget(m_paletteList);
}

void BlockProgrammingView::setupWorkspace()
{
    m_workspace = new QScrollArea();
    m_workspace->setWidgetResizable(true);

    // 创建bgfx渲染器作为工作区域
    m_bgfxRenderer = new BgfxBlockRenderer();
    m_bgfxRenderer->setMinimumSize(800, 600);

    // 连接缩放变化信号
    connect(m_bgfxRenderer, &BgfxBlockRenderer::zoomChanged, this, [this](float zoom) {
        if (m_zoomLabel) {
            m_zoomLabel->setText(QString("缩放: %1%").arg(QString::number(zoom * 100, 'f', 1)));
        }
    });

    // 连接鼠标坐标变化信号
    connect(m_bgfxRenderer, &BgfxBlockRenderer::mouseWorldPosChanged, this, [this](float x, float y) {
        if (m_coordLabel) {
            m_coordLabel->setText(QString("坐标: (%1, %2)").arg(QString::number(x, 'f', 1)).arg(QString::number(y, 'f', 1)));
        }
    });

    // 设置工作区域容器
    m_workspaceContent = new QWidget();
    auto* layout = new QVBoxLayout(m_workspaceContent);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_bgfxRenderer);

    m_workspace->setWidget(m_workspaceContent);

    qDebug() << "BlockProgrammingView: bgfx renderer integrated into workspace";
}

void BlockProgrammingView::setupStatusBar()
{
    m_statusBar = new QWidget();
    m_statusBar->setFixedHeight(30);
    m_statusBar->setStyleSheet("QWidget { background-color: #f0f0f0; border-top: 1px solid #ccc; }");
    
    auto* layout = new QHBoxLayout(m_statusBar);
    layout->setContentsMargins(10, 5, 10, 5);
    
    m_statusLabel = new QLabel(QString("脚本: %1 | 积木块: 0").arg(m_scriptName));
    m_statusLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");

    m_zoomLabel = new QLabel("缩放: 100%");
    m_zoomLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");

    m_coordLabel = new QLabel("坐标: (0, 0)");
    m_coordLabel->setStyleSheet("QLabel { color: #666; font-size: 11px; }");

    layout->addWidget(m_statusLabel);
    layout->addStretch();
    layout->addWidget(m_coordLabel);
    layout->addWidget(m_zoomLabel);
    
    m_mainLayout->addWidget(m_statusBar);
}

void BlockProgrammingView::setScriptName(const QString& name)
{
    m_scriptName = name;
    setWindowTitle(QString("积木编程视图 - %1").arg(name));

    if (m_statusLabel) {
        m_statusLabel->setText(QString("脚本: %1 | 积木块: 0").arg(m_scriptName));
    }
}

QString BlockProgrammingView::getScriptName() const
{
    return m_scriptName;
}

void BlockProgrammingView::setBlockConfiguration(const QJsonObject& config)
{
    m_blockConfiguration = config;
    
    // TODO: 根据配置加载积木块到工作区域
    qDebug() << "BlockProgrammingView: Loading block configuration";
}

QJsonObject BlockProgrammingView::getBlockConfiguration() const
{
    // TODO: 从工作区域收集积木块配置
    QJsonObject config;
    config["scriptName"] = m_scriptName;
    config["blocks"] = QJsonArray(); // 暂时为空
    return config;
}

void BlockProgrammingView::saveScript()
{
    QJsonObject config = getBlockConfiguration();
    emit scriptSaved(m_scriptName, config);
    
    QMessageBox::information(this, "保存成功", 
        QString("积木脚本 '%1' 已保存").arg(m_scriptName));
    
    qDebug() << "BlockProgrammingView: Script saved:" << m_scriptName;
}

void BlockProgrammingView::closeView()
{
    // 询问是否保存
    auto reply = QMessageBox::question(this, "关闭确认", 
        "是否保存当前脚本？", 
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    
    if (reply == QMessageBox::Yes) {
        saveScript();
    } else if (reply == QMessageBox::Cancel) {
        return; // 取消关闭
    }
    
    emit viewClosed();
    close();
}

void BlockProgrammingView::onNewBlock()
{
    QMessageBox::information(this, "新建积木", "新建积木功能待实现");
    qDebug() << "BlockProgrammingView: New block requested";
}

void BlockProgrammingView::onDeleteBlock()
{
    QMessageBox::information(this, "删除积木", "删除积木功能待实现");
    qDebug() << "BlockProgrammingView: Delete block requested";
}

void BlockProgrammingView::onClearAll()
{
    auto reply = QMessageBox::question(this, "清空确认", 
        "确定要清空所有积木块吗？", 
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        // TODO: 清空工作区域的所有积木块
        QMessageBox::information(this, "清空完成", "所有积木块已清空");
        qDebug() << "BlockProgrammingView: All blocks cleared";
    }
}

void BlockProgrammingView::onRunScript()
{
    QMessageBox::information(this, "运行脚本", "脚本运行功能待实现");
    qDebug() << "BlockProgrammingView: Run script requested";
}

// ============================================================================
// BlockItem 实现
// ============================================================================

BlockItem::BlockItem(BlockType type, const QString& name, QWidget* parent)
    : QWidget(parent), m_type(type), m_name(name)
{
    setMinimumSize(120, 40);
    setMaximumHeight(50);
    
    // 根据类型设置颜色
    switch (type) {
        case ControlFlow:
            m_color = QColor(255, 171, 64); // 橙色
            break;
        case DataOperation:
            m_color = QColor(64, 171, 255); // 蓝色
            break;
        case Logic:
            m_color = QColor(171, 64, 255); // 紫色
            break;
        case ExcelOperation:
            m_color = QColor(64, 255, 171); // 绿色
            break;
        case InputOutput:
            m_color = QColor(255, 64, 171); // 粉色
            break;
    }
}

void BlockItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制积木块
    QRect rect = this->rect().adjusted(2, 2, -2, -2);
    painter.setBrush(m_color);
    painter.setPen(QPen(m_color.darker(120), 2));
    painter.drawRoundedRect(rect, 8, 8);
    
    // 绘制文字
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(rect, Qt::AlignCenter, m_name);
}

void BlockItem::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit blockClicked();
    }
    QWidget::mousePressEvent(event);
}


