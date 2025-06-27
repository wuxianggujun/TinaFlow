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
 * @brief 封装Excel单元格范围数据的类
 * 
 * 这个类用于在节点之间传递单元格范围的数据，包括：
 * - 范围地址（如"A1:C10"）
 * - 二维数据数组
 * - 行列信息
 */
class RangeData : public QtNodes::NodeData
{
public:
    RangeData() = default;

    /**
     * @brief 构造函数
     * @param rangeAddress 范围地址，如"A1:C10"
     * @param data 二维数据数组
     */
    explicit RangeData(const QString& rangeAddress, const std::vector<std::vector<QVariant>>& data)
        : m_rangeAddress(rangeAddress), m_data(data)
    {
    }

    /**
     * @brief 返回节点数据类型
     */
    QtNodes::NodeDataType type() const override
    {
        return {"range", "Range"};
    }

    /**
     * @brief 获取范围地址
     * @return 范围地址字符串，如"A1:C10"
     */
    const QString& rangeAddress() const
    {
        return m_rangeAddress;
    }

    /**
     * @brief 设置范围地址
     * @param rangeAddress 范围地址
     */
    void setRangeAddress(const QString& rangeAddress)
    {
        m_rangeAddress = rangeAddress;
    }

    /**
     * @brief 获取数据
     * @return 二维数据数组
     */
    const std::vector<std::vector<QVariant>>& data() const
    {
        return m_data;
    }

    /**
     * @brief 设置数据
     * @param data 二维数据数组
     */
    void setData(const std::vector<std::vector<QVariant>>& data)
    {
        m_data = data;
    }

    /**
     * @brief 获取行数
     * @return 数据的行数
     */
    int rowCount() const
    {
        return static_cast<int>(m_data.size());
    }

    /**
     * @brief 获取列数
     * @return 数据的列数（基于第一行）
     */
    int columnCount() const
    {
        return m_data.empty() ? 0 : static_cast<int>(m_data[0].size());
    }

    /**
     * @brief 获取指定位置的单元格值
     * @param row 行索引（0开始）
     * @param col 列索引（0开始）
     * @return 单元格值
     */
    QVariant cellValue(int row, int col) const
    {
        if (row >= 0 && row < rowCount() && col >= 0 && col < columnCount()) {
            return m_data[row][col];
        }
        return QVariant();
    }

    /**
     * @brief 设置指定位置的单元格值
     * @param row 行索引（0开始）
     * @param col 列索引（0开始）
     * @param value 要设置的值
     */
    void setCellValue(int row, int col, const QVariant& value)
    {
        if (row >= 0 && row < rowCount() && col >= 0 && col < columnCount()) {
            m_data[row][col] = value;
        }
    }

    /**
     * @brief 获取指定行的数据
     * @param row 行索引（0开始）
     * @return 该行的数据
     */
    std::vector<QVariant> rowData(int row) const
    {
        if (row >= 0 && row < rowCount()) {
            return m_data[row];
        }
        return std::vector<QVariant>();
    }

    /**
     * @brief 获取指定列的数据
     * @param col 列索引（0开始）
     * @return 该列的数据
     */
    std::vector<QVariant> columnData(int col) const
    {
        std::vector<QVariant> result;
        if (col >= 0 && col < columnCount()) {
            for (const auto& row : m_data) {
                if (col < static_cast<int>(row.size())) {
                    result.push_back(row[col]);
                }
            }
        }
        return result;
    }

    /**
     * @brief 检查范围是否为空
     * @return 如果范围为空则返回true
     */
    bool isEmpty() const
    {
        return m_data.empty() || m_rangeAddress.isEmpty();
    }

    /**
     * @brief 获取数据的字符串表示（用于调试）
     * @return 包含范围和数据信息的字符串
     */
    QString debugString() const
    {
        return QString("Range[%1] %2x%3 cells")
            .arg(m_rangeAddress)
            .arg(rowCount())
            .arg(columnCount());
    }

    /**
     * @brief 将数据转换为QStringList的二维数组（用于显示）
     * @return 字符串形式的二维数组
     */
    std::vector<QStringList> toStringMatrix() const
    {
        std::vector<QStringList> result;
        for (const auto& row : m_data) {
            QStringList stringRow;
            for (const auto& cell : row) {
                stringRow.append(cell.toString());
            }
            result.push_back(stringRow);
        }
        return result;
    }

    /**
     * @brief 添加新行
     * @param rowData 新行的数据
     */
    void addRow(const std::vector<QVariant>& rowData)
    {
        m_data.push_back(rowData);
    }

    /**
     * @brief 清空所有数据
     */
    void clear()
    {
        m_data.clear();
        m_rangeAddress.clear();
    }

private:
    QString m_rangeAddress;                        ///< 范围地址，如"A1:C10"
    std::vector<std::vector<QVariant>> m_data;     ///< 二维数据数组
};
