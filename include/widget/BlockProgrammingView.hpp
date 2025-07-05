//
// Created by wuxianggujun on 25-7-3.
//

#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QToolBar>
#include <QSplitter>
#include <QScrollArea>
#include <QListWidget>
#include <QJsonObject>
#include <QJsonArray>

// bgfx渲染器
#include "BgfxBlockRenderer.hpp"

/**
 * @brief 积木编程视图
 * 
 * 提供可视化积木编程界面，用户可以通过拖拽积木块来编写Excel处理逻辑
 */
class BlockProgrammingView : public QWidget
{
    Q_OBJECT

public:
    explicit BlockProgrammingView(QWidget* parent = nullptr);
    ~BlockProgrammingView() override = default;

    // 设置和获取脚本配置
    void setScriptName(const QString& name);
    QString getScriptName() const;

    void setBlockConfiguration(const QJsonObject& config);
    QJsonObject getBlockConfiguration() const;

signals:
    void scriptSaved(const QString& scriptName, const QJsonObject& configuration);
    void viewClosed();

public slots:
    void saveScript();
    void closeView();

private slots:
    void onNewBlock();
    void onDeleteBlock();
    void onClearAll();
    void onRunScript();

private:
    void setupUI();
    void setupToolBar();
    void setupBlockPalette();
    void setupWorkspace();
    void setupStatusBar();

private:
    // UI 组件
    QVBoxLayout* m_mainLayout = nullptr;
    QToolBar* m_toolBar = nullptr;
    QSplitter* m_splitter = nullptr;
    
    // 积木调色板
    QWidget* m_paletteWidget = nullptr;
    QListWidget* m_paletteList = nullptr;
    
    // 工作区域
    QScrollArea* m_workspace = nullptr;
    QWidget* m_workspaceContent = nullptr;
    BgfxBlockRenderer* m_bgfxRenderer = nullptr;
    
    // 状态栏
    QWidget* m_statusBar = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_zoomLabel = nullptr;
    QLabel* m_coordLabel = nullptr;
    
    // 数据
    QString m_scriptName;
    QJsonObject m_blockConfiguration;
};

/**
 * @brief 积木块项目
 */
class BlockItem : public QWidget
{
    Q_OBJECT

public:
    enum BlockType {
        ControlFlow,    // 控制流
        DataOperation,  // 数据操作
        Logic,          // 逻辑运算
        ExcelOperation, // Excel操作
        InputOutput     // 输入输出
    };

    explicit BlockItem(BlockType type, const QString& name, QWidget* parent = nullptr);

    BlockType getType() const { return m_type; }
    QString getName() const { return m_name; }

signals:
    void blockClicked();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    BlockType m_type;
    QString m_name;
    QColor m_color;
};
