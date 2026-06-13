// NTAGED.cpp
#include "NTAGED.h"
#include "TGraph.h"
#include "Bound.h"
#include "TADiff.h"

#include <queue>
#include <iostream>
#include <limits>
#include <algorithm>
#include <vector>
#include <set>
#include <tuple>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <numeric>
#include <functional> // for PairHash

// —— 全局映射缓存 —— 
std::vector<std::pair<int, int>> NTAGED::globalMapping;

// —— 静态缓存 —— 
static std::vector<std::vector<double>> timeCost;
static std::vector<std::vector<std::pair<int, int>>> cachedIntervalsSrc;
static std::vector<std::vector<std::pair<int, int>>> cachedIntervalsTgt;

static std::vector<std::vector<std::pair<int, int>>> cachedConnS;
static std::vector<std::vector<std::pair<int, int>>> cachedNonConnS;
static std::unordered_set<int> lastMappedS;

// —— 边→索引，用以切子矩阵 —— 
struct PairHash {
    size_t operator()(std::pair<int, int> const& p) const noexcept {
        return (uint64_t(p.first) << 32) ^ uint64_t(p.second);
    }
};
static std::unordered_map<std::pair<int, int>, size_t, PairHash> srcEdgeIndex;
static std::unordered_map<std::pair<int, int>, size_t, PairHash> tgtEdgeIndex;
static std::vector<std::pair<int, int>> edgesSrcGlobal, edgesTgtGlobal;

// 按需计算并缓存 timeCost[i][j]
static double getTimeCost(size_t i, size_t j) {
    if (timeCost[i][j] < 0) {
        timeCost[i][j] = TADiff::calculateEdgeDifference(
            cachedIntervalsSrc[i], cachedIntervalsTgt[j]
        );

        // 2) 每次写入时输出变化
        /*std::cout << "[timeCost 更新] timeCost["
            << i << "][" << j << "] = "
            << timeCost[i][j] << std::endl;*/

    }
    return timeCost[i][j];
}

// 获取未映射顶点
static std::vector<int> getUnmappedVertices(
    const std::vector<std::pair<int, int>>& mapping,
    const std::vector<int>& vertices,
    bool source)
{
    std::unordered_set<int> mapped;
    for (auto& p : mapping) mapped.insert(source ? p.first : p.second);
    std::vector<int> out;
    for (int v : vertices) if (!mapped.count(v)) out.push_back(v);
    return out;
}

// —— getTimeCost 按需读取 timeCost —— 
static double computeEdgeSetDifferenceCached(
    const std::vector<size_t>& Aidx,
    const std::vector<size_t>& Bidx)
{
    size_t m = Aidx.size(), n = Bidx.size();
    size_t K = std::max(m, n);
    std::vector<std::vector<double>> cost(K, std::vector<double>(K, 0.0));

    for (size_t i = 0; i < K; ++i) {
        for (size_t j = 0; j < K; ++j) {
            size_t ri = (i < m ? Aidx[i] : edgesSrcGlobal.size() - 1);
            size_t cj = (j < n ? Bidx[j] : edgesTgtGlobal.size() - 1);
            cost[i][j] = getTimeCost(ri, cj);
        }
    }

    // --- 匈牙利算法实现部分 ---
    std::vector<double> u(K + 1, 0), v(K + 1, 0);
    std::vector<size_t> p(K + 1, 0), way(K + 1, 0);

    for (size_t i = 1; i <= K; ++i) {
        p[0] = i;
        std::vector<double> minv(K + 1, std::numeric_limits<double>::infinity());
        std::vector<bool> used(K + 1, false);
        size_t j0 = 0;

        do {
            used[j0] = true;
            size_t i0 = p[j0], j1 = 0;
            double delta = std::numeric_limits<double>::infinity();
            for (size_t j = 1; j <= K; ++j) {
                if (!used[j]) {
                    double cur = cost[i0 - 1][j - 1] - u[i0] - v[j];
                    if (cur < minv[j]) {
                        minv[j] = cur;
                        way[j] = j0;
                    }
                    if (minv[j] < delta) {
                        delta = minv[j];
                        j1 = j;
                    }
                }
            }

            for (size_t j = 0; j <= K; ++j) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                }
                else {
                    minv[j] -= delta;
                }
            }

            j0 = j1;
        } while (p[j0] != 0);

        // 回溯路径，更新匹配
        do {
            size_t j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0);
    }

    // 最优匹配总代价
    return -v[0];
}


// 修改后的启发式：全部通过索引版本，不再预先构建大矩阵
static double computeHeuristic(
    const TGraph& sg,
    const TGraph& tg,
    const std::vector<std::pair<int, int>>& mapping,
    double& labelDiffOut,
    double& connDiffOut,
    double& nonConnDiffOut)
{
    // 1) 标签差
    auto unmS = getUnmappedVertices(mapping, sg.V, true);
    auto unmT = getUnmappedVertices(mapping, tg.V, false);
    std::unordered_map<std::string, int> cntS, cntT;
    for (int u : unmS) cntS[sg.labelOf.at(u)]++;
    for (int v : unmT) cntT[tg.labelOf.at(v)]++;
    int tot = 0;
    for (auto& kv : cntS) {
        tot += std::abs(kv.second - cntT[kv.first]);
        cntT.erase(kv.first);
    }
    for (auto& kv : cntT) tot += kv.second;
    labelDiffOut = tot / 2.0;

    // 2) 更新源图连通/非连通边索引缓存
    std::unordered_set<int> mS;
    for (auto& p : mapping) mS.insert(p.first);
    static std::vector<size_t> cachedConnSIdx, cachedNonConnSIdx;
    if (mS != lastMappedS) {
        cachedConnSIdx.clear();
        cachedNonConnSIdx.clear();
        for (int u : mS) for (int w : unmS) {
            auto e = std::minmax(u, w);
            auto it = srcEdgeIndex.find(e);
            if (it != srcEdgeIndex.end()) cachedConnSIdx.push_back(it->second);
        }
        for (auto& e : sg.E) {
            if (!mS.count(e.first) && !mS.count(e.second)) {
                auto it = srcEdgeIndex.find(e);
                if (it != srcEdgeIndex.end()) cachedNonConnSIdx.push_back(it->second);
            }
        }
        lastMappedS = mS;
    }

    // 3) 连通边差 hc
    connDiffOut = 0.0;
    std::unordered_set<int> mT;
    for (auto& p : mapping) mT.insert(p.second);
    for (auto& mp : mapping) {
        int u = mp.first, v = mp.second;
        std::vector<size_t> uIdx, vIdx;
        for (int w : unmS) {
            auto e = std::minmax(u, w);
            if (auto it = srcEdgeIndex.find(e); it != srcEdgeIndex.end())
                uIdx.push_back(it->second);
        }
        for (int w : unmT) {
            auto e = std::minmax(v, w);
            if (auto it = tgtEdgeIndex.find(e); it != tgtEdgeIndex.end())
                vIdx.push_back(it->second);
        }
        connDiffOut += computeEdgeSetDifferenceCached(uIdx, vIdx);
    }

    // 4) 非连通边差 hi
    std::vector<size_t> nonConnTIdx;
    for (auto& e : tg.E) {
        if (!mT.count(e.first) && !mT.count(e.second)) {
            if (auto it = tgtEdgeIndex.find(e); it != tgtEdgeIndex.end())
                nonConnTIdx.push_back(it->second);
        }
    }
    nonConnDiffOut = computeEdgeSetDifferenceCached(cachedNonConnSIdx, nonConnTIdx);

    // 调试输出
    /*std::cout << "[HEURISTIC] mapping =";
    for (auto& p : mapping) std::cout << " " << p.first << "→" << p.second;
    std::cout << " | hl=" << labelDiffOut
        << " hc=" << connDiffOut
        << " hi=" << nonConnDiffOut
        << " h=" << (labelDiffOut + connDiffOut + nonConnDiffOut)
        << std::endl;*/

    return labelDiffOut + connDiffOut + nonConnDiffOut;
}

// 重载简版
static double computeHeuristic(
    const TGraph& sg,
    const TGraph& tg,
    const std::vector<std::pair<int, int>>& mapping)
{
    double ld, cd, nd;
    return computeHeuristic(sg, tg, mapping, ld, cd, nd);
}

// 保持原版：实际子图生成
TGraph NTAGED::generateActualGraph(
    const TGraph& g,
    bool isSource,
    int candidateVertex)
{
    TGraph actual;
    std::vector<int> U{ candidateVertex };
    for (auto& p : globalMapping) {
        int v = isSource ? p.first : p.second;
        if (std::find(U.begin(), U.end(), v) == U.end())
            U.push_back(v);
    }
    for (int v : U)
        actual.addVertex(v, g.labelOf.at(v));
    for (size_t i = 0; i < U.size(); ++i) {
        for (size_t j = i + 1; j < U.size(); ++j) {
            auto e = std::minmax(U[i], U[j]);
            if (g.E.count(e)) {
                actual.E.insert(e);
                actual.LE[e] = g.LE.at(e);
            }
        }
    }
    return actual;
}

// 实际结构差异：用 getTimeCost 替代直接 timeCost[]
double NTAGED::calculateActualStructureDifference(
    const TGraph& sG,
    const TGraph& tG,
    const std::vector<std::pair<int, int>>& mapping)
{
    double diff = 0.0;
    std::set<std::pair<int, int>> M(globalMapping.begin(), globalMapping.end());
    M.insert(mapping.begin(), mapping.end());
    std::vector<std::pair<int, int>> merged(M.begin(), M.end());

    // 顶点差
    for (auto& p : merged) {
        diff += TADiff::calculateVertexDifference(
            sG.labelOf.at(p.first),
            tG.labelOf.at(p.second)
        );
    }
    // 边差
    for (size_t i = 0; i < merged.size(); ++i) {
        for (size_t j = i + 1; j < merged.size(); ++j) {
            auto u = merged[i].first, v = merged[i].second;
            auto w = merged[j].first, x = merged[j].second;
            auto e1 = std::minmax(u, w), e2 = std::minmax(v, x);
            std::vector<std::pair<int, int>> A, B;
            if (sG.E.count(e1))
                A.assign(sG.LE.at(e1).begin(), sG.LE.at(e1).end());
            if (tG.E.count(e2))
                B.assign(tG.LE.at(e2).begin(), tG.LE.at(e2).end());
            auto it1 = std::find(cachedIntervalsSrc.begin(), cachedIntervalsSrc.end(), A);
            auto it2 = std::find(cachedIntervalsTgt.begin(), cachedIntervalsTgt.end(), B);
            if (it1 != cachedIntervalsSrc.end() && it2 != cachedIntervalsTgt.end()) {
                size_t i1 = it1 - cachedIntervalsSrc.begin();
                size_t i2 = it2 - cachedIntervalsTgt.begin();
                diff += getTimeCost(i1, i2);
            }
        }
    }
    return diff;
}

// A* 主流程
double NTAGED::calculateGraphEditDistance(
    const TGraph& sourceGraph,
    const TGraph& targetGraph,
    double threshold)
{
    // 构建边对 & 时间区间缓存(缓存到了定义在 NTAGED 类中的两个成员变量里)
    std::vector<std::pair<int, int>> edgesSrc(sourceGraph.E.begin(), sourceGraph.E.end());
    std::vector<std::pair<int, int>> edgesTgt(targetGraph.E.begin(), targetGraph.E.end());
    edgesSrc.emplace_back(-1, -1);
    edgesTgt.emplace_back(-1, -1);

    size_t mSrc = edgesSrc.size(), mTgt = edgesTgt.size();

    cachedIntervalsSrc.assign(mSrc, {});
    cachedIntervalsTgt.assign(mTgt, {});
    for (size_t i = 0; i < mSrc; ++i) {
        auto [u, v] = edgesSrc[i];
        if (u != -1)
            cachedIntervalsSrc[i] = { sourceGraph.LE.at({u,v}).begin(),
                                      sourceGraph.LE.at({u,v}).end() };
    }
    for (size_t j = 0; j < mTgt; ++j) {
        auto [x, y] = edgesTgt[j];
        if (x != -1)
            cachedIntervalsTgt[j] = { targetGraph.LE.at({x,y}).begin(),
                                      targetGraph.LE.at({x,y}).end() };
    }

    // 保存全局 edges & 构建索引
    edgesSrcGlobal = edgesSrc;
    edgesTgtGlobal = edgesTgt;
    srcEdgeIndex.clear();
    for (size_t i = 0; i < mSrc; ++i) srcEdgeIndex[edgesSrcGlobal[i]] = i;
    tgtEdgeIndex.clear();
    for (size_t j = 0; j < mTgt; ++j) tgtEdgeIndex[edgesTgtGlobal[j]] = j;

    // **懒初始化 timeCost**：全部设为 -1
    timeCost.assign(mSrc, std::vector<double>(mTgt, -1.0));

    // 补 Null 顶点
    TGraph modSrc = sourceGraph;
    if (modSrc.V.size() < targetGraph.V.size()) {
        size_t miss = targetGraph.V.size() - modSrc.V.size();
        int maxId = *std::max_element(modSrc.V.begin(), modSrc.V.end());
        for (size_t i = 0; i < miss; ++i)
            modSrc.addVertex(++maxId, "Null");
    }

    // 初始化 openSet & A* 搜索
    std::priority_queue<State> openSet;
    for (int v : targetGraph.V) {
        State st;
        st.g = TADiff::calculateVertexDifference(
            modSrc.labelOf.at(modSrc.V[0]),
            targetGraph.labelOf.at(v)
        );
        st.mapping = { {modSrc.V[0], v} };

        st.h = computeHeuristic(modSrc, targetGraph,
            st.mapping = { {modSrc.V[0],v} });
        if (st.g <= threshold && st.f() <= threshold) {
            openSet.push(st);
        }
    }

    size_t totalV = std::max(modSrc.V.size(), targetGraph.V.size());
    while (!openSet.empty()) {
        State cur = openSet.top(); openSet.pop();

        // 全部后续状态 f 都 ≥ cur.f(); 若 cur.f() > threshold，可全部剪掉
        if (cur.f() > threshold) {
            continue;
        }
        // 达到完整映射，返回结果（必 ≤ threshold）
        if (cur.mapping.size() == totalV) {
            return cur.f();
        }
        globalMapping = cur.mapping;

        auto unmS = getUnmappedVertices(cur.mapping, modSrc.V, true);
        auto unmT = getUnmappedVertices(cur.mapping, targetGraph.V, false);
        int u = unmS.front();
        for (int v : unmT) {
            State nxt = cur;
            nxt.mapping.emplace_back(u, v);
            // 计算实际代价和估计代价
            nxt.g = calculateActualStructureDifference(modSrc, targetGraph, nxt.mapping);
            nxt.h = computeHeuristic(modSrc, targetGraph, nxt.mapping);

            // 剪枝：若实际成本或 f 超过阈值，则跳过
            if (nxt.g > threshold || nxt.f() > threshold) {
                continue;
            }

            openSet.push(nxt);
        }
    }

    return std::numeric_limits<double>::infinity();
}

void NTAGED::clearCaches() {
    // 1) 时间代价矩阵 & 区间列表
    timeCost.clear();            timeCost.shrink_to_fit();
    cachedIntervalsSrc.clear();  cachedIntervalsSrc.shrink_to_fit();
    cachedIntervalsTgt.clear();  cachedIntervalsTgt.shrink_to_fit();

    // 2) 连通/非连通边缓存
    cachedConnS.clear();         cachedConnS.shrink_to_fit();
    cachedNonConnS.clear();      cachedNonConnS.shrink_to_fit();
    lastMappedS.clear();

    // 3) 全局边列表 & 索引
    edgesSrcGlobal.clear();      edgesSrcGlobal.shrink_to_fit();
    edgesTgtGlobal.clear();      edgesTgtGlobal.shrink_to_fit();
    srcEdgeIndex.clear();
    tgtEdgeIndex.clear();
}
