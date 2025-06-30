//
// Created by wuxianggujun on 25-6-30.
//

#pragma once

#include <QtNodes/NodeDelegateModel>
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
class BaseDisplayModel : public QtNodes::NodeDelegateModel
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
    virtual QString getNodeTypeName() const = 0;
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

private:
    std::shared_ptr<DataType> m_data;
};
