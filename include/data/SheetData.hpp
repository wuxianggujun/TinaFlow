//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include <string>
#include <XLSheet.hpp>
#include <QtNodes/NodeData>

class SheetData : public QtNodes::NodeData
{
public:
    SheetData() = default;

    explicit SheetData(const std::string& name, const OpenXLSX::XLWorksheet& sheet) : m_sheetName(name),
        m_xlsxWorksheet(sheet)
    {
    }

    QtNodes::NodeDataType type() const override
    {
        return {"sheet", "Worksheet"};
    }

    const std::string& sheetName() const
    {
        return m_sheetName;
    }

    OpenXLSX::XLWorksheet& worksheet()
    {
        return m_xlsxWorksheet;
    }

private:
    std::string m_sheetName;
    OpenXLSX::XLWorksheet m_xlsxWorksheet;
};
