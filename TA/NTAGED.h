// NTAGED.h
#ifndef NTAGED_H
#define NTAGED_H

#include "TGraph.h"
#include "TADiff.h"
#include <vector>
#include <queue>
#include <string>
#include <unordered_set>
#include <utility>
#include <iostream>
#include <limits>
#include <algorithm>

class NTAGED {
public:
    // 状态结构体定义
    struct State {
        double g = 0; // 实际代价，默认 0
        double h = 0; // 启发式估计代价，默认 0
        std::vector<std::pair<int, int>> mapping; // 顶点映射

        // 计算总代价
        double f() const { return g + h; }

        // 优先队列比较：总代价小的优先
        bool operator<(const State& other) const {
            return f() > other.f();
        }
    };

    // 生成实际子图
    static TGraph generateActualGraph(
        const TGraph& g, bool isSource, int candidateVertex);

    // 计算实际结构差异（含顶点和边），优先从缓存 timeCost 中读取边差异
    static double calculateActualStructureDifference(
        const TGraph& actualSourceGraph,
        const TGraph& actualTargetGraph,
        const std::vector<std::pair<int, int>>& candidateMapping);

    // 主函数：计算图编辑距离
    double calculateGraphEditDistance(
        const TGraph& sourceGraph,
        const TGraph& targetGraph,
        double threshold);

    // 全局记录：最新被 pop 出的状态映射
    static std::vector<std::pair<int, int>> globalMapping;

    //清理内存
    static void clearCaches();
};

#endif // NTAGED_H
