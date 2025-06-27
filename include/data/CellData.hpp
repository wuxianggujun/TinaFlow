//
// Created by wuxianggujun on 25-6-26.
//

#pragma once
#include <QtNodes/NodeData>
#include <XLCell.hpp>

class CellData : public QtNodes::NodeData
{
public:
    CellData() = default;

    explicit CellData(OpenXLSX::XLCell cell) : m_cell(std::make_shared<OpenXLSX::XLCell>(cell))
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
        return m_cell != nullptr;
    }

private:
    std::shared_ptr<OpenXLSX::XLCell> m_cell;
};
