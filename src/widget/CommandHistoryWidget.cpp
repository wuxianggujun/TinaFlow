#include "widget/CommandHistoryWidget.hpp"
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QListWidgetItem>
#include <QIcon>

CommandHistoryWidget::CommandHistoryWidget(QWidget* parent)
    : QWidget(parent)
    , m_commandManager(&CommandManager::instance())
{
    setupUI();
    connectSignals();
    // 直接更新历史，不使用QTimer::singleShot避免卡死
    updateHistory();
}

void CommandHistoryWidget::setupUI()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(6, 6, 6, 6);
    m_mainLayout->setSpacing(6);

    // 标题
    m_titleLabel = new QLabel("命令历史", this);
    m_titleLabel->setStyleSheet(
        "QLabel {"
        "    font-weight: bold;"
        "    font-size: 14px;"
        "    color: #2E86AB;"
        "    padding: 6px;"
        "    background-color: #f0f0f0;"
        "    border-radius: 4px;"
        "}"
    );
    m_titleLabel->setAlignment(Qt::AlignCenter);
    m_mainLayout->addWidget(m_titleLabel);

    // 分割器用于显示撤销和重做历史
    m_splitter = new QSplitter(Qt::Vertical, this);

    // 撤销历史组
    QGroupBox* undoGroup = new QGroupBox("撤销历史", this);
    QVBoxLayout* undoLayout = new QVBoxLayout(undoGroup);
    m_undoList = new QListWidget(this);
    m_undoList->setToolTip("点击项目可以跳转到该状态");
    m_undoList->setAlternatingRowColors(true);
    undoLayout->addWidget(m_undoList);
    m_splitter->addWidget(undoGroup);

    // 重做历史组
    QGroupBox* redoGroup = new QGroupBox("重做历史", this);
    QVBoxLayout* redoLayout = new QVBoxLayout(redoGroup);
    m_redoList = new QListWidget(this);
    m_redoList->setToolTip("点击项目可以跳转到该状态");
    m_redoList->setAlternatingRowColors(true);
    redoLayout->addWidget(m_redoList);
    m_splitter->addWidget(redoGroup);

    m_mainLayout->addWidget(m_splitter);

    // 清除历史按钮
    m_clearButton = new QPushButton("清除所有历史", this);
    m_clearButton->setToolTip("清除所有撤销重做历史（不可恢复）");
    m_clearButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #dc3545;"
        "    color: white;"
        "    border: none;"
        "    padding: 6px 12px;"
        "    border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #c82333;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #bd2130;"
        "}"
    );
    
    connect(m_clearButton, &QPushButton::clicked, this, &CommandHistoryWidget::onClearHistoryClicked);
    m_mainLayout->addWidget(m_clearButton);

    // 设置分割器比例
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);
}

void CommandHistoryWidget::connectSignals()
{
    // 重新启用命令管理器信号，使用队列连接避免循环调用
    connect(m_commandManager, &CommandManager::historyChanged,
            this, &CommandHistoryWidget::updateHistory, Qt::QueuedConnection);
    connect(m_commandManager, &CommandManager::undoRedoStateChanged,
            this, [this](bool, bool){ 
                // 延迟更新，避免频繁调用
                QMetaObject::invokeMethod(this, [this](){
                    updateHistory();
                }, Qt::QueuedConnection);
            });

    // 连接列表点击事件
    connect(m_undoList, &QListWidget::itemClicked,
            this, [this](QListWidgetItem* item) {
                int row = m_undoList->row(item);
                onHistoryItemClicked(row);
            });
    
    connect(m_redoList, &QListWidget::itemClicked,
            this, [this](QListWidgetItem* item) {
                int row = m_redoList->row(item);
                // 重做历史的索引需要转换
                onHistoryItemClicked(-(row + 1));
            });
}

void CommandHistoryWidget::updateHistory()
{
    // 清空现有内容
    m_undoList->clear();
    m_redoList->clear();

    // 获取撤销历史（最多显示20个）
    QStringList undoHistory = m_commandManager->getUndoHistory(20);
    for (int i = 0; i < undoHistory.size(); ++i) {
        QListWidgetItem* item = new QListWidgetItem(undoHistory[i]);
        
        // 最新的命令用不同颜色显示
        if (i == 0) {
            item->setForeground(QColor("#2E86AB"));
            item->setToolTip("当前状态");
        } else {
            item->setToolTip(QString("点击撤销到此状态（需要 %1 次撤销）").arg(i));
        }
        
        m_undoList->addItem(item);
    }

    // 获取重做历史（最多显示20个）
    QStringList redoHistory = m_commandManager->getRedoHistory(20);
    for (int i = 0; i < redoHistory.size(); ++i) {
        QListWidgetItem* item = new QListWidgetItem(redoHistory[i]);
        item->setForeground(QColor("#6c757d"));
        item->setToolTip(QString("点击重做到此状态（需要 %1 次重做）").arg(i + 1));
        m_redoList->addItem(item);
    }

    // 更新按钮状态
    m_clearButton->setEnabled(m_commandManager->getUndoCount() > 0 || m_commandManager->getRedoCount() > 0);

    // 更新标题显示统计信息
    int undoCount = m_commandManager->getUndoCount();
    int redoCount = m_commandManager->getRedoCount();
    m_titleLabel->setText(QString("命令历史 (撤销: %1, 重做: %2)").arg(undoCount).arg(redoCount));
}

void CommandHistoryWidget::onHistoryItemClicked(int index)
{
    // 临时断开信号连接，避免在批量操作时反复更新UI
    disconnect(m_commandManager, &CommandManager::historyChanged,
               this, &CommandHistoryWidget::updateHistory);
    disconnect(m_commandManager, &CommandManager::undoRedoStateChanged,
               this, nullptr);
    
    if (index >= 0) {
        // 撤销到指定状态
        for (int i = 0; i < index; ++i) {
            if (!m_commandManager->undo()) {
                break;
            }
        }
    } else {
        // 重做到指定状态
        int redoSteps = -index;
        for (int i = 0; i < redoSteps; ++i) {
            if (!m_commandManager->redo()) {
                break;
            }
        }
    }
    
    // 重新连接信号
    connect(m_commandManager, &CommandManager::historyChanged,
            this, &CommandHistoryWidget::updateHistory);
    connect(m_commandManager, &CommandManager::undoRedoStateChanged,
            this, [this](bool, bool){ updateHistory(); });
    
    // 最后更新一次UI
    updateHistory();
}

void CommandHistoryWidget::onClearHistoryClicked()
{
    if (QMessageBox::question(this, "确认清除", 
                             "确定要清除所有命令历史吗？\n此操作不可恢复！") 
        == QMessageBox::Yes) {
        m_commandManager->clear();
    }
}
