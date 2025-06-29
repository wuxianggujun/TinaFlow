//
// Created by wuxianggujun on 25-6-26.
//

#pragma once
#include <QtNodes/NodeData>
#include <XLCell.hpp>
#include <QString>
#include <QVariant>

class CellData : public QtNodes::NodeData
{
public:
    CellData() = default;

    explicit CellData(OpenXLSX::XLCell cell) : m_cell(std::make_shared<OpenXLSX::XLCell>(cell))
    {
    }

    // 新的构造函数，用于从地址和值创建虚拟单元格
    CellData(const QString& address, const QVariant& value)
        : m_address(address), m_value(value), m_cell(nullptr)
    {
    }

    QtNodes::NodeDataType type() const override
    {
        return {"cell", "Cell"};
    }

    std::shared_ptr<OpenXLSX::XLCell> cell() const
    {
        return m_cell;
    }

    bool isValid() const
    {
        return m_cell != nullptr || !m_address.isEmpty();
    }

    // 获取单元格地址
    QString address() const
    {
        if (m_cell) {
            return QString::fromStdString(m_cell->cellReference().address());
        }
        return m_address;
    }

    // 获取单元格值
    QVariant value() const
    {
        if (m_cell) {
            return QVariant(QString::fromStdString(m_cell->value().get<std::string>()));
        }
        return m_value;
    }

private:
    std::shared_ptr<OpenXLSX::XLCell> m_cell;
    QString m_address;  // 用于虚拟单元格
    QVariant m_value;   // 用于虚拟单元格
};
