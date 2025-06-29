//
// Created by wuxianggujun on 25-6-29.
//

#pragma once

#include <QtNodes/NodeData>
#include <QList>
#include "CellData.hpp"

/**
 * @brief 单元格列表数据类型
 * 
 * 用于传递多个单元格数据的集合，通常用于：
 * - 循环处理的结果
 * - 筛选后的单元格集合
 * - 批量操作的目标单元格
 */
class CellListData : public QtNodes::NodeData
{
public:
    CellListData() = default;
    
    /**
     * @brief 构造函数
     * @param cells 单元格数据列表
     * @param rowIndices 对应的行索引列表（可选）
     */
    CellListData(const QList<CellData>& cells, const QList<int>& rowIndices = {})
        : m_cells(cells), m_rowIndices(rowIndices)
    {
    }
    
    QtNodes::NodeDataType type() const override
    {
        return {"CellListData", "单元格列表"};
    }
    
    // 访问方法
    const QList<CellData>& cells() const { return m_cells; }
    const QList<int>& rowIndices() const { return m_rowIndices; }
    
    // 修改方法
    void addCell(const CellData& cell, int rowIndex = -1) 
    {
        m_cells.append(cell);
        if (rowIndex >= 0) {
            m_rowIndices.append(rowIndex);
        }
    }
    
    void clear() 
    {
        m_cells.clear();
        m_rowIndices.clear();
    }
    
    // 便利方法
    int count() const { return m_cells.size(); }
    bool isEmpty() const { return m_cells.isEmpty(); }
    
    CellData at(int index) const 
    {
        if (index >= 0 && index < m_cells.size()) {
            return m_cells[index];
        }
        return CellData();
    }
    
    int rowIndexAt(int index) const 
    {
        if (index >= 0 && index < m_rowIndices.size()) {
            return m_rowIndices[index];
        }
        return -1;
    }
    
    // 获取所有单元格的值
    QStringList values() const
    {
        QStringList result;
        for (const auto& cell : m_cells) {
            result.append(cell.value().toString());
        }
        return result;
    }

    // 获取所有单元格的地址
    QStringList addresses() const
    {
        QStringList result;
        for (const auto& cell : m_cells) {
            result.append(cell.address());
        }
        return result;
    }

private:
    QList<CellData> m_cells;      // 单元格数据列表
    QList<int> m_rowIndices;      // 对应的行索引（可选）
};
