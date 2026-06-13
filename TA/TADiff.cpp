//计算顶点标签差异和事件边标签差异规则

#include "TADiff.h"
#include <algorithm>
#include <iostream>

// Hardcoded cost values
const double C1 = 1.0; // Cost for completely different (1, 2, 9, 10)
const double C2 = 1.0; // Cost for overlapping but not subset (3, 4)
const double Cti = 1.0; // Cost for insertion
const double Ctd = 1.0; // Cost for deletion

// 判断两个时间区间的时态关系
int classifyTemporalRelation(const std::pair<int, int>& t1, const std::pair<int, int>& t2) {

    //调试语句在分类函数中打印输入和输出
    //std::cout << "Comparing: (" << t1.first << ", " << t1.second << ") with ("
        //<< t2.first << ", " << t2.second << ")" << std::endl;

    if (t1.second < t2.first) return 1;  // t1 早于 t2
    if (t2.second < t1.first) return 2;  // t2 早于 t1
    if (t1.first < t2.first && t1.second > t2.first && t1.second < t2.second) return 3;  // t1 相交 t2
    if (t2.first < t1.first && t2.second > t1.first && t2.second < t1.second) return 4;  // t2 相交 t1
    if (t1.first == t2.first && t1.second > t2.second) return 5;  // t1 启动 t2
    if (t2.first == t1.first && t2.second > t1.second) return 6;  // t2 启动 t1
    if (t1.second == t2.second && t1.first < t2.first) return 7;  // t1 结束 t2
    if (t2.second == t1.second && t2.first < t1.first) return 8;  // t2 结束 t1
    if (t1.second == t2.first) return 9;  // t1 相接 t2
    if (t2.second == t1.first) return 10; // t2 相接 t1
    if (t1.first < t2.first && t1.second > t2.second) return 11; // t1 包含 t2
    if (t2.first < t1.first && t2.second > t1.second) return 12; // t2 包含 t1
    if (t1 == t2) return 13; // t1 等于 t2
    return -1; // 未知关系

}

//顶点差异
double TADiff::calculateVertexDifference(const std::string& label1, const std::string& label2) {
    if (label1.empty() && !label2.empty()) return Cti; // 空顶点对应有效顶点，插入代价
    if (!label1.empty() && label2.empty()) return Ctd; // 有效顶点对应空顶点，删除代价
    return label1 == label2 ? 0 : 1;
}

//事件边差异
double TADiff::calculateEdgeDifference(
    const std::vector<std::pair<int, int>>& timeIntervalsA,
    const std::vector<std::pair<int, int>>& timeIntervalsB) {

    // 处理空边的情况
    // 处理两边都是空边的情况
    if (timeIntervalsA.empty() && timeIntervalsB.empty()) {
        return 0.0; // 如果两边都是空边，返回 0
    }
    if (timeIntervalsA.empty() && !timeIntervalsB.empty()) {
        return Cti * timeIntervalsB.size(); // 空边对有效边，插入操作
    }
    if (!timeIntervalsA.empty() && timeIntervalsB.empty()) {
        return Ctd * timeIntervalsA.size(); // 有效边对空边，删除操作
    }
    if (!timeIntervalsA.empty() && !timeIntervalsB.empty()) {
        // 排序时间区间集合
        auto sortedA = timeIntervalsA;
        auto sortedB = timeIntervalsB;
        std::sort(sortedA.begin(), sortedA.end());
        std::sort(sortedB.begin(), sortedB.end());

        /*打印排序后结果
        std::cout << "Sorted A: ";
        for (const auto& t : sortedA) {
            std::cout << "(" << t.first << ", " << t.second << ") ";
        }
        std::cout << std::endl;

        std::cout << "Sorted B: ";
        for (const auto& t : sortedB) {
            std::cout << "(" << t.first << ", " << t.second << ") ";
        }
        std::cout << std::endl;
        */


        std::vector<std::pair<int, int>> TunmatchedA, TunmatchedB;
        std::vector<bool> matchedB(sortedB.size(), false); // 标记B中的区间是否被匹配
        double totalCost = 0;

        // 生成 TunmatchedA 集合
        for (const auto& t1 : sortedA) {
            bool matched = false;
            for (const auto& t2 : sortedB) {
                int relation = classifyTemporalRelation(t1, t2);
                if (relation == 5 || relation == 6 || relation == 7 ||
                    relation == 8 || relation == 11 || relation == 12 || relation == 13) {
                    matched = true; // Found a match
                    break;
                }
            }
            if (!matched) {
                TunmatchedA.push_back(t1);
            }
        }

        // 生成 TunmatchedB 集合
         // Compare intervals in B against A
        for (const auto& t2 : sortedB) {
            bool matched = false;
            for (const auto& t1 : sortedA) {
                int relation = classifyTemporalRelation(t1, t2);
                if (relation == 5 || relation == 6 || relation == 7 ||
                    relation == 8 || relation == 11 || relation == 12 || relation == 13) {
                    matched = true; // Found a match
                    break;
                }
            }
            if (!matched) {
                TunmatchedB.push_back(t2);
            }
        }

        /* 打印 TunmatchedA
        std::cout << "Tunmatched A: ";
        for (const auto& interval : TunmatchedA) {
            std::cout << "(" << interval.first << ", " << interval.second << ") ";
        }
        std::cout << std::endl;

        // 打印 TunmatchedB
        std::cout << "Tunmatched B: ";
        for (const auto& interval : TunmatchedB) {
            std::cout << "(" << interval.first << ", " << interval.second << ") ";
        }
        std::cout << std::endl;
        */

        // 处理未匹配的区间
        size_t minSize = std::min(TunmatchedA.size(), TunmatchedB.size());
        size_t maxSize = std::max(TunmatchedA.size(), TunmatchedB.size());

        for (size_t i = 0; i < minSize; ++i) {
            int relation = classifyTemporalRelation(TunmatchedA[i], TunmatchedB[i]);
            if (relation == 1 || relation == 2 || relation == 9 || relation == 10) {
                totalCost += C1; // 完全不同
            }
            else if (relation == 3 || relation == 4) {
                totalCost += C2; // 部分相交
            }
        }

        // 处理多余区间
        if (TunmatchedA.size() > TunmatchedB.size()) {
            totalCost += (TunmatchedA.size() - TunmatchedB.size()) * Ctd; // 删除操作
        }
        else if (TunmatchedA.size() < TunmatchedB.size()) {
            totalCost += (TunmatchedB.size() - TunmatchedA.size()) * Cti; // 插入操作
        }

        return totalCost;
    }
}
