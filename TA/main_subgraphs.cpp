#include <map> 
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include "TGraph.h"
#include "NTAGED.h"
#include "Bound.h"
#include <chrono>
using namespace std;

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
            ss >> id >> lbl; g.addVertex(id, lbl);
        }
        else if (line[0] == 'e') {
            int u, v, s, t;
            istringstream ss(line.substr(2));
            ss >> u >> v >> s >> t; g.addEdge(u, v, s, t);
        }
    }
    if (inG) Q.push_back(g);
    return Q;
}

// 读取子图
vector<TGraph> parseSubgraphs(const string& fn) {
    vector<TGraph> S; S.reserve(40000);
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
            ss >> id >> lbl; g->addVertex(id, lbl);
        }
        else if (line[0] == 'e') {
            int u, v, s, t;
            istringstream ss(line.substr(2));
            ss >> u >> v >> s >> t; g->addEdge(u, v, s, t);
        }
    }
    if (inG) S.push_back(move(*g));
    return S;
}


int main() {
    using namespace std::chrono;

    // 1. 读入查询图
    auto t1 = high_resolution_clock::now();
    auto queries = parseQueries("E:/YUN/Desktop/TAA/TA/A/q1.txt");
    auto t2 = high_resolution_clock::now();
    std::cout << "[Time] Loading queries: "
        << duration_cast<milliseconds>(t2 - t1).count()
        << " ms\n";

    // 2. 读入数据/子图
    t1 = high_resolution_clock::now();
    auto subs = parseSubgraphs("E:/YUN/Desktop/TAA/TA/A/g1.txt");
    for (auto& sg : subs) sg.normalize();
    t2 = high_resolution_clock::now();
    std::cout << "[Time] Loading subgraphs: "
        << duration_cast<milliseconds>(t2 - t1).count()
        << " ms\n";

    // 3. 打印加载信息
    std::cout << "#Q=" << queries.size() << ", #S=" << subs.size() << "\n";

    int Threshold = 25;
    int countChecked = 0;
    long long ged_total_time = 0;
    long long total_lb_time_us = 0;


    // 新增：参与 GED 计算的图对计数
    //long long eligibleCount = 0;
    // 新增：下界值分布统计
    //std::map<int, long long> lbCount;     // 记录每个 lb 值出现的次数


     //4. 逐 Query vs Data 计算并格式化输出
    for (size_t qi = 0; qi < queries.size(); ++qi) {
        TGraph q = queries[qi];
        q.normalize();

        for (size_t di = 0; di < subs.size(); ++di) {
            // 清空上一次的全局映射
            NTAGED::globalMapping.clear();

            // 计算下界
            auto t_lb_start = high_resolution_clock::now();
            auto lb = Bound::computeTemporalBounds(q, subs[di]);
            auto t_lb_end = high_resolution_clock::now();
            auto lb_us = duration_cast<milliseconds>(t_lb_end - t_lb_start).count();
            total_lb_time_us += lb_us;

            // 统计下界分布
            //++lbCount[lb];

            //std::cout << "Lower bound = " << lb << std::endl;

            // 过滤
            double ged;
            long long ged_time = 0;

            if (lb <= Threshold) {  // 验证

                ++countChecked;      // 计数 ++

                auto gs = high_resolution_clock::now();
                NTAGED solver;
                // 较小图先入参
                if (q.V.size() <= subs[di].V.size()) {
                    ged = solver.calculateGraphEditDistance(q, subs[di], Threshold);
                }
                else {
                    ged = solver.calculateGraphEditDistance(subs[di], q, Threshold);
                }
                auto ge = high_resolution_clock::now();
                ged_time = duration_cast<milliseconds>(ge - gs).count();
                ged_total_time += ged_time;
            }
            NTAGED::clearCaches();

            // 打印: Query #x vs Data #y -> GED = z (time: t ms)
            std::cout << "Query #" << qi + 1
                << " vs Data #" << di + 1
                << " -> GED = " << ged
                << "  (time: " << ged_time << " ms)\n";
        }
        // 每个 Query 间空一行
        cout << "-----------------\n";
    }

    // 4. 按索引匹配：第i个Query只与第i个Data计算
    //for (size_t i = 0; i < queries.size() && i < subs.size(); ++i) {  // 索引同步，取较小长度避免越界
    //    TGraph q = queries[i];
    //    q.normalize();
    //    TGraph& sub = subs[i];  // 直接取对应索引的子图

    //    // 清空上一次的全局映射
    //    NTAGED::globalMapping.clear();

    //    // 计算下界
    //    auto t_lb_start = high_resolution_clock::now();
    //    auto lb = Bound::computeTemporalBounds(q, sub);
    //    auto t_lb_end = high_resolution_clock::now();
    //    auto lb_us = duration_cast<microseconds>(t_lb_end - t_lb_start).count();
    //    total_lb_time_us += lb_us;

    //    // 过滤
    //    double ged = -1;  // 初始化默认值（未计算时）
    //    long long ged_time = 0;

    //    if (lb <= Threshold) {  // 验证
    //        ++countChecked;      // 计数 ++

    //        auto gs = high_resolution_clock::now();
    //        NTAGED solver;
    //        // 较小图先入参
    //        if (q.V.size() <= sub.V.size()) {
    //            ged = solver.calculateGraphEditDistance(q, sub, Threshold);
    //        }
    //        else {
    //            ged = solver.calculateGraphEditDistance(sub, q, Threshold);
    //        }
    //        auto ge = high_resolution_clock::now();
    //        ged_time = duration_cast<microseconds>(ge - gs).count();
    //        ged_total_time += ged_time;
    //    }
    //    NTAGED::clearCaches();

    //    // 打印一行：Query #x vs Data #x -> GED = z (time: t ms)
    //    std::cout << "Query #" << i + 1
    //        << " vs Data #" << i + 1
    //        << " -> GED = " << ged
    //        << "  (LB = " << lb << ", LB time: " << lb_us << " ms, GED time: " << ged_time << " us)\n";
    //}
    //// 每个匹配对间空一行
    //cout << "-----------------\n";

    //*****************

    /*std::cout << "—— 下界值分布 ——\n";
    for (auto const& [lbValue, count] : lbCount) {
        std::cout << "  lb = " << lbValue
            << "  出现次数 = " << count << "\n";
    }*/

    // 循环外：输出统计结果
    /*std::cout << "Threshold = " << Threshold
        << " 时，共有 " << eligibleCount
        << " 对图会参与 GED 计算。\n";*/

    std::cout << "Total LB time: " << total_lb_time_us << " ms\n";
    std::cout << "=== Total GED-only time: " << ged_total_time << " ms ===\n";
    std::cout << "Threshold = " << Threshold << " -> GED图对数量 = " << countChecked
        << ", 总时间 = " << ged_total_time
        << ", 平均时间 = " << (countChecked > 0 ? ged_total_time / countChecked : 0)
        << " ms\n";

    return 0;
}