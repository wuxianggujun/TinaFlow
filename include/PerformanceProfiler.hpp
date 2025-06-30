//
// Created by TinaFlow Team
//

#pragma once

#include <QString>
#include <QElapsedTimer>
#include <QMap>
#include <QMutex>
#include <QDebug>
#include <memory>

/**
 * @brief 性能分析器
 * 
 * 提供简单的性能监控功能：
 * - 操作计时
 * - 性能统计
 * - 内存使用监控
 * - 性能报告生成
 */
class PerformanceProfiler
{
public:
    /**
     * @brief 作用域计时器 - RAII方式自动计时
     */
    class ScopedTimer
    {
    public:
        ScopedTimer(const QString& operation)
            : m_operation(operation)
        {
            m_timer.start();
        }
        
        ~ScopedTimer()
        {
            qint64 elapsed = m_timer.elapsed();
            PerformanceProfiler::reportTiming(m_operation, elapsed);
        }
        
    private:
        QString m_operation;
        QElapsedTimer m_timer;
    };
    
    /**
     * @brief 性能统计信息
     */
    struct PerformanceStats
    {
        qint64 totalTime = 0;       // 总时间（毫秒）
        qint64 minTime = LLONG_MAX; // 最短时间
        qint64 maxTime = 0;         // 最长时间
        int callCount = 0;          // 调用次数
        double averageTime = 0.0;   // 平均时间
        
        void update(qint64 time)
        {
            totalTime += time;
            minTime = qMin(minTime, time);
            maxTime = qMax(maxTime, time);
            callCount++;
            averageTime = static_cast<double>(totalTime) / callCount;
        }
    };

public:
    /**
     * @brief 获取单例实例
     */
    static PerformanceProfiler& instance()
    {
        static PerformanceProfiler instance;
        return instance;
    }
    
    /**
     * @brief 报告操作计时
     * @param operation 操作名称
     * @param milliseconds 耗时（毫秒）
     */
    static void reportTiming(const QString& operation, qint64 milliseconds)
    {
        instance().recordTiming(operation, milliseconds);
    }
    
    /**
     * @brief 获取计时报告
     * @return 操作名称到统计信息的映射
     */
    static QMap<QString, PerformanceStats> getTimingReport()
    {
        return instance().getStats();
    }
    
    /**
     * @brief 清除所有统计数据
     */
    static void clearStats()
    {
        instance().clear();
    }
    
    /**
     * @brief 生成性能报告字符串
     * @param sortByTotalTime 是否按总时间排序
     * @return 格式化的性能报告
     */
    static QString generateReport(bool sortByTotalTime = true)
    {
        return instance().formatReport(sortByTotalTime);
    }
    
    /**
     * @brief 启用/禁用性能监控
     * @param enabled 是否启用
     */
    static void setEnabled(bool enabled)
    {
        instance().m_enabled = enabled;
    }
    
    /**
     * @brief 检查性能监控是否启用
     */
    static bool isEnabled()
    {
        return instance().m_enabled;
    }

private:
    PerformanceProfiler() = default;
    ~PerformanceProfiler() = default;
    
    // 禁用拷贝
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;
    
    void recordTiming(const QString& operation, qint64 milliseconds)
    {
        if (!m_enabled) return;
        
        QMutexLocker locker(&m_mutex);
        m_stats[operation].update(milliseconds);
        
        // 记录到调试输出
        qDebug() << "PerformanceProfiler:" << operation << "took" << milliseconds << "ms";
    }
    
    QMap<QString, PerformanceStats> getStats() const
    {
        QMutexLocker locker(&m_mutex);
        return m_stats;
    }
    
    void clear()
    {
        QMutexLocker locker(&m_mutex);
        m_stats.clear();
    }
    
    QString formatReport(bool sortByTotalTime) const
    {
        QMutexLocker locker(&m_mutex);
        
        if (m_stats.isEmpty()) {
            return "暂无性能数据";
        }
        
        QString report;
        report += "=== TinaFlow 性能报告 ===\n\n";
        report += QString("%-30s %8s %8s %8s %8s %8s\n")
            .arg("操作名称")
            .arg("调用次数")
            .arg("总时间(ms)")
            .arg("平均(ms)")
            .arg("最短(ms)")
            .arg("最长(ms)");
        report += QString("-").repeated(80) + "\n";
        
        // 创建排序后的列表
        QList<QPair<QString, PerformanceStats>> sortedStats;
        for (auto it = m_stats.begin(); it != m_stats.end(); ++it) {
            sortedStats.append(qMakePair(it.key(), it.value()));
        }
        
        // 排序
        if (sortByTotalTime) {
            std::sort(sortedStats.begin(), sortedStats.end(),
                [](const QPair<QString, PerformanceStats>& a, const QPair<QString, PerformanceStats>& b) {
                    return a.second.totalTime > b.second.totalTime;
                });
        } else {
            std::sort(sortedStats.begin(), sortedStats.end(),
                [](const QPair<QString, PerformanceStats>& a, const QPair<QString, PerformanceStats>& b) {
                    return a.second.averageTime > b.second.averageTime;
                });
        }
        
        // 生成报告内容
        for (const auto& pair : sortedStats) {
            const QString& operation = pair.first;
            const PerformanceStats& stats = pair.second;
            
            report += QString("%-30s %8d %8lld %8.1f %8lld %8lld\n")
                .arg(operation.left(30))
                .arg(stats.callCount)
                .arg(stats.totalTime)
                .arg(stats.averageTime)
                .arg(stats.minTime == LLONG_MAX ? 0 : stats.minTime)
                .arg(stats.maxTime);
        }
        
        report += "\n";
        report += QString("总操作数: %1\n").arg(m_stats.size());
        
        qint64 totalTime = 0;
        int totalCalls = 0;
        for (const auto& stats : m_stats) {
            totalTime += stats.totalTime;
            totalCalls += stats.callCount;
        }
        
        report += QString("总执行时间: %1 ms\n").arg(totalTime);
        report += QString("总调用次数: %1\n").arg(totalCalls);
        if (totalCalls > 0) {
            report += QString("平均每次调用: %.1f ms\n").arg(static_cast<double>(totalTime) / totalCalls);
        }
        
        return report;
    }

private:
    mutable QMutex m_mutex;
    QMap<QString, PerformanceStats> m_stats;
    bool m_enabled = true;
};

/**
 * @brief 性能监控宏定义
 */
#define PROFILE_SCOPE(name) \
    PerformanceProfiler::ScopedTimer timer(name)

#define PROFILE_FUNCTION() \
    PROFILE_SCOPE(__FUNCTION__)

#define PROFILE_NODE(nodeType) \
    PROFILE_SCOPE(QString("Node::%1").arg(nodeType))

/**
 * @brief 条件性能监控宏（只在Debug模式下启用）
 */
#ifdef QT_DEBUG
    #define PROFILE_SCOPE_DEBUG(name) PROFILE_SCOPE(name)
    #define PROFILE_FUNCTION_DEBUG() PROFILE_FUNCTION()
    #define PROFILE_NODE_DEBUG(nodeType) PROFILE_NODE(nodeType)
#else
    #define PROFILE_SCOPE_DEBUG(name)
    #define PROFILE_FUNCTION_DEBUG()
    #define PROFILE_NODE_DEBUG(nodeType)
#endif
