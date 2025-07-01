//
// Created by TinaFlow Team
//

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QSplitter>
#include "CommandManager.hpp"

/**
 * @brief 命令历史小部件
 * 
 * 显示撤销重做历史，允许用户查看和跳转到特定的历史状态
 */
class CommandHistoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit CommandHistoryWidget(QWidget* parent = nullptr);

private slots:
    void updateHistory();
    void onHistoryItemClicked(int row);
    void onClearHistoryClicked();

private:
    void setupUI();
    void connectSignals();

private:
    QVBoxLayout* m_mainLayout;
    QLabel* m_titleLabel;
    QListWidget* m_undoList;
    QListWidget* m_redoList;
    QSplitter* m_splitter;
    QPushButton* m_clearButton;
    
    CommandManager* m_commandManager;
}; 
