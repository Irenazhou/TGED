#ifndef TADIFF_H
#define TADIFF_H

#include <set>
#include <string>
#include <utility>
#include <vector>
#include <utility> 

// ——— 新增：两个时间区间的时态关系分类函数 ———
int classifyTemporalRelation(
    const std::pair<int, int>&t1,
    const std::pair<int, int>&t2
     );

class TADiff {
public:
    // 计算两个顶点间的差异
    static double calculateVertexDifference(const std::string& label1, const std::string& label2);

    // 计算两条事件边的编辑距离
    static double calculateEdgeDifference(
        const std::vector<std::pair<int, int>>& timeIntervalsA,
        const std::vector<std::pair<int, int>>& timeIntervalsB);
    };

#endif // TADIFF_H
