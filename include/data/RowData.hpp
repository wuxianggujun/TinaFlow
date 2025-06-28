//
// Created by wuxianggujun on 25-6-27.
//

#pragma once

#include <QVariant>
#include <QString>
#include <QStringList>
#include <QtNodes/NodeData>
#include <vector>

/**
 * @brief 封装Excel单行数据的类
 * 
 * 这个类用于在节点之间传递单行的数据，包括：
 * - 行索引（从0开始）
 * - 该行的所有列数据
 * - 行的元数据信息
 */
class RowData : public QtNodes::NodeData
{
public:
    RowData() = default;

    /**
     * @brief 构造函数
     * @param rowIndex 行索引（从0开始）
     * @param rowData 该行的所有列数据
     * @param totalRows 总行数（可选）
     */
    explicit RowData(int rowIndex, const std::vector<QVariant>& rowData, int totalRows = -1)
        : m_rowIndex(rowIndex), m_rowData(rowData), m_totalRows(totalRows)
    {
    }

    /**
     * @brief 返回节点数据类型
     */
    QtNodes::NodeDataType type() const override
    {
        return {"row", "Row"};
    }

    /**
     * @brief 获取行索引
     * @return 行索引（从0开始）
     */
    int rowIndex() const
    {
        return m_rowIndex;
    }

    /**
     * @brief 设置行索引
     * @param rowIndex 行索引
     */
    void setRowIndex(int rowIndex)
    {
        m_rowIndex = rowIndex;
    }

    /**
     * @brief 获取行数据
     * @return 该行的所有列数据
     */
    const std::vector<QVariant>& rowData() const
    {
        return m_rowData;
    }

    /**
     * @brief 设置行数据
     * @param rowData 该行的所有列数据
     */
    void setRowData(const std::vector<QVariant>& rowData)
    {
        m_rowData = rowData;
    }

    /**
     * @brief 获取列数
     * @return 该行的列数
     */
    int columnCount() const
    {
        return static_cast<int>(m_rowData.size());
    }

    /**
     * @brief 获取指定列的值
     * @param columnIndex 列索引（从0开始）
     * @return 指定列的值
     */
    QVariant cellValue(int columnIndex) const
    {
        if (columnIndex >= 0 && columnIndex < columnCount()) {
            return m_rowData[columnIndex];
        }
        return QVariant();
    }

    /**
     * @brief 设置指定列的值
     * @param columnIndex 列索引（从0开始）
     * @param value 要设置的值
     */
    void setCellValue(int columnIndex, const QVariant& value)
    {
        if (columnIndex >= 0 && columnIndex < columnCount()) {
            m_rowData[columnIndex] = value;
        }
    }

    /**
     * @brief 获取总行数
     * @return 总行数，如果未设置则返回-1
     */
    int totalRows() const
    {
        return m_totalRows;
    }

    /**
     * @brief 设置总行数
     * @param totalRows 总行数
     */
    void setTotalRows(int totalRows)
    {
        m_totalRows = totalRows;
    }

    /**
     * @brief 检查是否为空行
     * @return 如果所有列都为空则返回true
     */
    bool isEmpty() const
    {
        for (const auto& cell : m_rowData) {
            if (!cell.isNull() && !cell.toString().isEmpty()) {
                return false;
            }
        }
        return true;
    }

    /**
     * @brief 获取行的字符串表示
     * @return 该行所有列的字符串形式
     */
    QStringList toStringList() const
    {
        QStringList result;
        for (const auto& cell : m_rowData) {
            result.append(cell.toString());
        }
        return result;
    }

    /**
     * @brief 获取调试信息字符串
     * @return 包含行索引和数据的调试信息
     */
    QString debugString() const
    {
        return QString("Row[%1/%2] %3 columns")
            .arg(m_rowIndex + 1)  // 显示时使用1基索引
            .arg(m_totalRows > 0 ? QString::number(m_totalRows) : "?")
            .arg(columnCount());
    }

    /**
     * @brief 获取进度百分比
     * @return 当前行在总行数中的百分比（0-100），如果总行数未知则返回-1
     */
    double progressPercentage() const
    {
        if (m_totalRows <= 0) {
            return -1.0;
        }
        return (static_cast<double>(m_rowIndex + 1) / m_totalRows) * 100.0;
    }

    /**
     * @brief 检查是否为第一行
     * @return 如果是第一行则返回true
     */
    bool isFirstRow() const
    {
        return m_rowIndex == 0;
    }

    /**
     * @brief 检查是否为最后一行
     * @return 如果是最后一行则返回true（需要设置了总行数）
     */
    bool isLastRow() const
    {
        return m_totalRows > 0 && m_rowIndex == m_totalRows - 1;
    }

    /**
     * @brief 添加列数据
     * @param value 要添加的列值
     */
    void addColumn(const QVariant& value)
    {
        m_rowData.push_back(value);
    }

    /**
     * @brief 清空行数据
     */
    void clear()
    {
        m_rowData.clear();
        m_rowIndex = -1;
        m_totalRows = -1;
    }

private:
    int m_rowIndex = -1;                    ///< 行索引（从0开始）
    std::vector<QVariant> m_rowData;        ///< 该行的所有列数据
    int m_totalRows = -1;                   ///< 总行数（用于进度计算）
};
