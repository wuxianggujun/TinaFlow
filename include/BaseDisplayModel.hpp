//
// Created by wuxianggujun on 25-6-30.
//

#pragma once

#include "BaseNodeModel.hpp"
#include "PropertyWidget.hpp"
#include <QDebug>
#include <memory>

/**
 * @brief Display节点的基类
 * 
 * 封装了所有Display类型节点的通用功能：
 * - 标准化的setInData模式（空指针检查、类型转换、调试日志）
 * - 统一的save/load逻辑
 * - 通用的updateDisplay调用
 * 
 * 子类只需要：
 * 1. 实现具体的updateDisplay()方法
 * 2. 提供数据类型信息
 * 3. 重写getDataTypeName()和getNodeTypeName()
 */
template<typename DataType>
class BaseDisplayModel : public BaseNodeModel
{
public:
    BaseDisplayModel() = default;
    ~BaseDisplayModel() override = default;

public:
    // 基本节点信息
    unsigned int nPorts(QtNodes::PortType const portType) const override
    {
        if (portType == QtNodes::PortType::In) {
            return 1; // 所有Display节点都只有一个输入端口
        } else {
            return 0; // Display节点没有输出端口
        }
    }

    QtNodes::NodeDataType dataType(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        if (portType == QtNodes::PortType::In && portIndex == 0) {
            return DataType().type();
        }
        return QtNodes::NodeDataType();
    }

    std::shared_ptr<QtNodes::NodeData> outData(QtNodes::PortIndex const port) override
    {
        return nullptr; // Display节点没有输出
    }

    // 标准化的setInData实现
    void setInData(std::shared_ptr<QtNodes::NodeData> nodeData, QtNodes::PortIndex const portIndex) override
    {
        qDebug() << getNodeTypeName() << "::setInData called, portIndex:" << portIndex;
        
        if (!nodeData) {
            qDebug() << getNodeTypeName() << ": Received null nodeData";
            m_data.reset();
            updateDisplay();
            return;
        }
        
        m_data = std::dynamic_pointer_cast<DataType>(nodeData);
        if (m_data) {
            qDebug() << getNodeTypeName() << ": Successfully received" << getDataTypeName();
            onDataReceived(m_data); // 允许子类处理接收到的数据
        } else {
            qDebug() << getNodeTypeName() << ": Failed to cast to" << getDataTypeName();
        }
        
        updateDisplay();
    }

    // 标准化的save/load实现
    QJsonObject save() const override
    {
        return QtNodes::NodeDelegateModel::save(); // 大多数Display节点不需要保存状态
    }

    void load(QJsonObject const& json) override
    {
        // 大多数Display节点不需要加载状态
        Q_UNUSED(json);
    }

protected:
    // 子类需要实现的纯虚函数
    virtual void updateDisplay() = 0;
    virtual QString getDataTypeName() const = 0;
    
    // 可选的回调函数，子类可以重写
    virtual void onDataReceived(std::shared_ptr<DataType> data) 
    {
        Q_UNUSED(data);
        // 默认不做任何处理
    }

    // 获取当前数据
    std::shared_ptr<DataType> getData() const { return m_data; }
    
    // 检查是否有有效数据
    bool hasValidData() const { return m_data && isDataValid(m_data); }

    // 子类可以重写的数据有效性检查
    virtual bool isDataValid(std::shared_ptr<DataType> data) const
    {
        return data != nullptr; // 默认只检查非空
    }

protected:
    // 实现BaseNodeModel的虚函数
    QString getNodeTypeName() const override
    {
        return QString("BaseDisplayModel<%1>").arg(DataType().type().name);
    }

    // 通用的属性面板实现
    bool createPropertyPanel(PropertyWidget* propertyWidget) override
    {
        propertyWidget->addTitle(getDisplayName());
        propertyWidget->addDescription(getDescription());

        // 显示节点只需要查看模式，不需要模式切换按钮

        // 数据状态信息
        if (hasValidData()) {
            propertyWidget->addInfoProperty("数据状态", "已连接", "color: #28a745; font-weight: bold;");

            // 调用子类的具体数据显示
            addDataSpecificProperties(propertyWidget);
        } else {
            propertyWidget->addInfoProperty("数据状态", "未连接", "color: #999; font-style: italic;");
        }

        // 数据类型信息
        propertyWidget->addSeparator();
        propertyWidget->addTitle("数据类型信息");
        propertyWidget->addInfoProperty("输入类型", DataType().type().name, "color: #666;");

        return true;
    }

    // 子类可以重写此方法来添加特定的数据属性
    virtual void addDataSpecificProperties(PropertyWidget* propertyWidget)
    {
        Q_UNUSED(propertyWidget);
        // 默认不添加额外属性
    }



    QString getDisplayName() const override
    {
        return QString("显示%1").arg(DataType().type().name);
    }

    QString getDescription() const override
    {
        return QString("显示%1类型的数据内容").arg(DataType().type().name);
    }

private:
    std::shared_ptr<DataType> m_data;
};
