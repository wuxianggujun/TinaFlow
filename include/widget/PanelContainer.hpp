//
// Created by wuxianggujun on 25-7-1.
//

#pragma once


#include <QWidget>
#include <QSize>
#include <QString>
#include <QJsonObject>

class PanelContainer : public QWidget
{
    Q_OBJECT

public:
    
    explicit PanelContainer(QWidget* widget) : QWidget(widget)
    {
    }
    
    enum PanelType
    {
        NodePalette,
        PropertyPanel,
        CommandHistory,
        NodeEditor
    };

    virtual void setPanelContent(QWidget* content) = 0;
    virtual void setCollapsible(bool collapsible) = 0;
    virtual void setDragAndDropEnabled(bool enabled) = 0;

    virtual QString getPanelTitle() const = 0;
    virtual QString getPanelId() const = 0;
    virtual bool isCloseable() const { return true; }
    virtual bool isMovable() const { return true; }
    virtual bool isFloatable() const { return true; }

    virtual QSize getMinimumSize() const { return QSize(200, 150); }
    virtual QSize getPreferredSize() const { return QSize(300, 400); }

    virtual void saveState(QJsonObject& state) const
    {
    }

    virtual void restoreState(const QJsonObject& state)
    {
    }


signals:
    void panelDragStarted(PanelType type, QWidget* content);
    void panelDropRequested(PanelType type, QWidget* content);
    void layoutChanged();

    void titleChanged(const QString& title);
    void closeRequested();
    void floatRequested();
};
