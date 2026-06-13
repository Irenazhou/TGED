//将图数据txt读入并存储到Graph类中

#include "TGraph.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

void TGraph::normalize() {
    std::vector<int> sortedV = V;
    std::sort(sortedV.begin(), sortedV.end());

    oldToNewId.clear();
    newToOldId.clear();

    int newId = 0;
    for (int oldId : sortedV) {
        oldToNewId[oldId] = newId;
        newToOldId[newId] = oldId;
        newId++;
    }

    std::vector<int> newV;
    std::unordered_map<int, std::string> newLabelOf;
    for (int oldId : V) {
        int newVertexId = oldToNewId[oldId];
        newV.push_back(newVertexId);
        newLabelOf[newVertexId] = labelOf[oldId];
    }
    V = newV;
    labelOf = newLabelOf;

    std::set<std::pair<int, int>> newE;
    std::unordered_map<std::pair<int, int>, std::set<std::pair<int, int>>, hash_pair> newLE;
    for (const auto& edge : E) {
        int u = oldToNewId[edge.first];
        int v = oldToNewId[edge.second];
        std::pair<int, int> newEdge = { std::min(u, v), std::max(u, v) };
        newE.insert(newEdge);
        newLE[newEdge] = LE[edge];
    }
    E = newE;
    LE = newLE;
}

//将顶点及其标签加入图中
void TGraph::addVertex(int id, const std::string& label) {
    if (!labelOf.count(id)) {
        labelOf[id] = label;
        V.push_back(id);
    }
}

//添加边和其时间段信息
void TGraph::addEdge(int u, int v, int start, int end) {
    auto edgeKey = std::make_pair(std::min(u, v), std::max(u, v));
    E.insert(edgeKey);
    LE[edgeKey].insert({ start, end });
}

//从文件中读取图结构
TGraph TGraph::readGraphFromFile(const std::string& filename) {
    TGraph g;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        if (line[0] == 'v') {
            int id;
            std::string label;
            std::istringstream ss(line.substr(2));
            ss >> id >> label;
            g.addVertex(id, label);
        }
        else if (line[0] == 'e') {
            int u, v, start, end;
            std::istringstream ss(line.substr(2));
            ss >> u >> v >> start >> end;
            g.addEdge(u, v, start, end);
        }
    }

    return g;
}

//获取标签集合
std::set<std::string> TGraph::getVertexLabelSet() const {
    std::set<std::string> labelSet;
    for (const auto& pair : labelOf) {
        labelSet.insert(pair.second);
    }
    return labelSet;
}

//打印图的顶点和边信息
//void TGraph::printGraph() const {
//    std::cout << "Vertices: ";
//    for (const auto& v : V) {
//        std::cout << v << " ";
//    }
//    std::cout << "\nEdges: ";
//    for (const auto& e : E) {
//        std::cout << "(" << e.first << ", " << e.second << ") ";
//    }
//
//    //打印顶点标签集合
//     std::cout << "\nVertex Labels:" << " ";
//     for (int vid : V) {
//         std::cout << labelOf.at(vid) << " ";
//     }
//
//     //打印边标签集合
//     std::cout << "\nEdge Labels:" << " ";
//     for (const auto& e : E) {
//         //std::cout << "(" << e.first << ", " << e.second << "): ";
//         const auto& intervals = LE.at(e);
//         for (const auto& interval : intervals) {
//             std::cout << "[" << interval.first << ", " << interval.second << "] ";
//         }
//         std::cout << " ";
//     }
//
//    std::cout << std::endl;
//    
//}
//
// //打印图的详细信息
//void TGraph::printGraphDetails(const std::string& name) const {
//    std::cout << "===== " << name << " Graph =====" << std::endl;
//
//    // 打印顶点集合
//    std::cout << "Vertices: ";
//    for (size_t i = 0; i < V.size(); ++i) {
//        std::cout << V[i] << " ";
//    }
//    std::cout << "\nVertex Labels: ";
//    for (int vid : V) {
//        std::cout << labelOf.at(vid) << " ";
//    }
//    std::cout << std::endl;
//
//    // 打印边集合
//    std::cout << "Edges: ";
//    for (const auto& edge : E) {
//        std::cout << "(" << edge.first << ", " << edge.second << ") ";
//    }
//    std::cout << "\nEdge Labels: ";
//    for (const auto& edge : E) {
//        const auto& intervals = LE.at(edge);
//        std::cout << "[";
//        for (const auto& interval : intervals) {
//            std::cout << "(" << interval.first << ", " << interval.second << ") ";
//        }
//        std::cout << "] ";
//    }
//    std::cout << std::endl;
//}

