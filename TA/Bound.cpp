#include "Bound.h"
#include "TADiff.h"
#include "TGraph.h"

#include <vector>
#include <algorithm>
#include <unordered_map>

// ——— 1) 计算顶点标签多重集差异 ———
static int computeVertexLabelMultisetDiff(
    const TGraph& g1,
    const TGraph& g2
) {
    std::unordered_map<std::string, int> cnt1, cnt2;
    for (int v : g1.V) cnt1[g1.labelOf.at(v)]++;
    for (int v : g2.V) cnt2[g2.labelOf.at(v)]++;

    int common = 0;
    for (auto& kv : cnt1) {
        auto it = cnt2.find(kv.first);
        if (it != cnt2.end()) {
            common += std::min(kv.second, it->second);
        }
    }
    return std::max((int)g1.V.size(), (int)g2.V.size()) - common;
}

// ——— 2) 全局下界：时间序列 + 顶点标签 ———
int Bound::computeTemporalBounds(
    const TGraph& g1,
    const TGraph& g2
) {
    // 1) 提取两图所有边的时间区间列表
    std::vector<std::vector<std::pair<int, int>>> seq1, seq2;
    seq1.reserve(g1.E.size());
    seq2.reserve(g2.E.size());
    for (auto const& e : g1.E)
        seq1.emplace_back(g1.LE.at(e).begin(), g1.LE.at(e).end());
    for (auto const& e : g2.E)
        seq2.emplace_back(g2.LE.at(e).begin(), g2.LE.at(e).end());

    // 2) 预排序：对每条边的区间序列各自一次性排序
    for (auto& v : seq1) std::sort(v.begin(), v.end());
    for (auto& v : seq2) std::sort(v.begin(), v.end());

    // 3) 高效字典序比较器
    auto lexLess = [](auto const& A, auto const& B) {
        return std::lexicographical_compare(
            A.begin(), A.end(),
            B.begin(), B.end(),
            [](auto const& p1, auto const& p2) {
                if (p1.first != p2.first) return p1.first < p2.first;
                return p1.second < p2.second;
            }
        );
        };

    // 4) 整体排序
    std::sort(seq1.begin(), seq1.end(), lexLess);
    std::sort(seq2.begin(), seq2.end(), lexLess);

    // 5) 双指针匹配，统计“完全相等”的对数（交集大小）
    std::size_t i = 0, j = 0, inter = 0;
    while (i < seq1.size() && j < seq2.size()) {
        // 用 TADiff: 差为 0 则视为“相等”
        if (TADiff::calculateEdgeDifference(seq1[i], seq2[j]) == 0.0) {
            ++inter; ++i; ++j;
        }
        else if (lexLess(seq1[i], seq2[j])) {
            ++i;
        }
        else {
            ++j;
        }
    }
    int temporalDiff = static_cast<int>(
        std::max(seq1.size(), seq2.size()) - inter
        );

    // 6) 顶点标签多重集差异
    int vertexDiff = computeVertexLabelMultisetDiff(g1, g2);

    // 7) 全局下界 = 边时间标签差异 + 顶点标签差异
    return temporalDiff + vertexDiff;
}
