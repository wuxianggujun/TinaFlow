//
// Created by wuxianggujun on 25-6-26.
//

#pragma once

#include <QtNodes/NodeData>

#include <XLWorkbook.hpp>

class WorkbookData : public QtNodes::NodeData
{
public:
    WorkbookData() = default;

    explicit WorkbookData(OpenXLSX::XLWorkbook workbook,OpenXLSX::XLDocument* doc)
    : m_workbook(std::make_shared<OpenXLSX::XLWorkbook>(workbook)),
    m_document(doc)
    {
    }

    QtNodes::NodeDataType type() const override
    {
        return {"workbook", "Workbook"};
    }

    std::shared_ptr<OpenXLSX::XLWorkbook> workbook() const { return m_workbook; }

    OpenXLSX::XLDocument* document() const { return m_document; }
    
    bool isValid() const { return m_workbook != nullptr && m_document != nullptr; }

    ~WorkbookData() override
    {
        if (m_document != nullptr)
        {
            m_document->close();
            delete m_document;
        }
    }
    
private:
    std::shared_ptr<OpenXLSX::XLWorkbook> m_workbook;
    OpenXLSX::XLDocument* m_document;
};
