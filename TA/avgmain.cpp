#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <memory>
#include <map>
#include "TGraph.h"
#include "NTAGED.h"
#include "Bound.h"

using namespace std;
using Clock = chrono::high_resolution_clock;
using ms = chrono::milliseconds;

// 读取查询图
vector<TGraph> parseQueries(const string& fn) {
    vector<TGraph> Q;
    ifstream in(fn);
    string line;
    TGraph g;
    bool inG = false;
    while (getline(in, line)) {
        if (line.empty()) continue;
        if (line.find("Query") != string::npos || line.find("subgraph") != string::npos) {
            if (inG) {
                Q.push_back(g);
                g = TGraph();
            }
            inG = true;
        }
        else if (line[0] == 'v') {
            int id; string lbl;
            istringstream ss(line.substr(2));
            ss >> id >> lbl;
            g.addVertex(id, lbl);
        }
        else if (line[0] == 'e') {
            int u, v, s, t;
            istringstream ss(line.substr(2));
            ss >> u >> v >> s >> t;
            g.addEdge(u, v, s, t);
        }
    }
    if (inG) Q.push_back(g);
    return Q;
}

// 读取子图
vector<TGraph> parseSubgraphs(const string& fn) {
    vector<TGraph> S;
    S.reserve(40000);
    ifstream in(fn);
    string line;
    auto g = make_unique<TGraph>();
    bool inG = false;
    while (getline(in, line)) {
        if (line.empty()) continue;
        if (line.find("subgraph") != string::npos) {
            if (inG) {
                S.push_back(move(*g));
                g = make_unique<TGraph>();
            }
            inG = true;
        }
        else if (line[0] == 'v') {
            int id; string lbl;
            istringstream ss(line.substr(2));
            ss >> id >> lbl;
            g->addVertex(id, lbl);
        }
        else if (line[0] == 'e') {
            int u, v, s, t;
            istringstream ss(line.substr(2));
            ss >> u >> v >> s >> t;
            g->addEdge(u, v, s, t);
        }
    }
    if (inG) S.push_back(move(*g));
    return S;
}

int main() {
    // 1. 一次性加载 & 预处理
    auto t0 = Clock::now();
    auto queries = parseQueries("E:/YUN/Desktop/TAA/TA/Exp2/AIDS_100_querys(1~2).txt");
    for (auto& q : queries) q.normalize();
    auto subgraphs = parseSubgraphs("E:/YUN/Desktop/TAA/TA/Exp2/AIDS_10k_subgraphs(1~2).txt");
    for (auto& sg : subgraphs) sg.normalize();
    auto t1 = Clock::now();
    cout << "[Init] Loaded Q=" << queries.size()
        << " & S=" << subgraphs.size()
        << " in " << chrono::duration_cast<ms>(t1 - t0).count()
        << " ms\n\n";

    // 2. 阈值列表 & 运行次数
    vector<int> thresholds = { 1, 2, 3, 4, 5, 6,7,8,9,10,11,12 };
    const int runs = 2;

    // 3. 对每个阈值，跑 runs 遍并统计
    for (int th : thresholds) {
        long long sumGedTime = 0;
        long long sumCountChecked = 0;

        cout << "=== Threshold = " << th << " ===\n";
        for (int run = 1; run <= runs; ++run) {
            long long ged_total_time = 0;
            long long countChecked = 0;

            // 对所有 Query vs Subgraph
            for (auto& q : queries) {
                for (auto& s : subgraphs) {
                    NTAGED::globalMapping.clear();

                    int lb = Bound::computeTemporalBounds(q, s);
                    if (lb > th) continue;

                    ++countChecked;
                    auto t_start = Clock::now();
                    NTAGED solver;
                    if (q.V.size() <= s.V.size()) {
                        solver.calculateGraphEditDistance(q, s, th);
                    }
                    else {
                        solver.calculateGraphEditDistance(s, q, th);
                    }
                    auto t_end = Clock::now();
                    ged_total_time += chrono::duration_cast<ms>(t_end - t_start).count();

                    NTAGED::clearCaches();
                }
            }

            sumGedTime += ged_total_time;
            sumCountChecked += countChecked;

            double avgPerGraph = countChecked ? (double)ged_total_time / countChecked : 0.0;
            cout << " Run " << run
                << ": TotalGED=" << ged_total_time << " ms"
                << ", Checked=" << countChecked
                << ", Avg=" << avgPerGraph << " ms/graph\n";
        }

        // 平均趋势
        double avgTotal = (double)sumGedTime / runs;
        double avgPerGraph = sumCountChecked
            ? (double)sumGedTime / sumCountChecked
            : 0.0;
        cout << "--> Avg over " << runs << " runs: TotalGED="
            << avgTotal << " ms, AvgPerGraph="
            << avgPerGraph << " ms/graph\n\n";
    }

    return 0;
}
