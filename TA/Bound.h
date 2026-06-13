// Bound.h
#ifndef BOUND_H
#define BOUND_H

#include "TGraph.h"

class Bound {
public:
    // 全局下界：顶点标签差异 + 全图事件序列多重标签差异
    static int computeTemporalBounds(const TGraph& g1, const TGraph& g2);
};

#endif // BOUND_H
