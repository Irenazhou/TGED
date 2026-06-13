#ifndef TGRAPH_H
#define TGRAPH_H

#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <utility>

struct hash_pair {
    template <class T1, class T2>
    size_t operator()(const std::pair<T1, T2>& p) const {
        auto hash1 = std::hash<T1>{}(p.first);
        auto hash2 = std::hash<T2>{}(p.second);
        return hash1 ^ (hash2 << 1);
    }
};

class TGraph {
public:
    std::vector<int> V; // 顶点集
    std::unordered_map<int, std::string> labelOf; // 顶点标签
    std::set<std::pair<int, int>> E; // 边集合
    std::unordered_map<std::pair<int, int>, std::set<std::pair<int, int>>, hash_pair> LE; // 边时序信息

    std::unordered_map<int, int> oldToNewId;
    std::unordered_map<int, int> newToOldId;

    void addVertex(int id, const std::string& label);
    void addEdge(int u, int v, int start, int end);
    //void printGraph() const;
    //void printGraphDetails(const std::string& name) const; // 打印详细信息(判别源图与目标图)

    static TGraph readGraphFromFile(const std::string& filename);

    // 新增：对图进行顶点重编号，使顶点编号从 0 开始且连续
    void normalize();
    //顶点标签集合
    std::set<std::string> getVertexLabelSet() const;
};

#endif // TGRAPH_H