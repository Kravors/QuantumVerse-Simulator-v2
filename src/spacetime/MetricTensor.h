#pragma once
#include <array>
#include <cmath>
#include "Event4D.h"

namespace quantumverse {
class MetricTensor {
public:
    std::array<std::array<double, 4>, 4> g;
    MetricTensor() {
        g[0][0] = -1.0; g[0][1] = 0.0; g[0][2] = 0.0; g[0][3] = 0.0;
        g[1][0] = 0.0; g[1][1] = 1.0; g[1][2] = 0.0; g[1][3] = 0.0;
        g[2][0] = 0.0; g[2][1] = 0.0; g[2][2] = 1.0; g[2][3] = 0.0;
        g[3][0] = 0.0; g[3][1] = 0.0; g[3][2] = 0.0; g[3][3] = 1.0;
    }
    static MetricTensor schwarzschild(double, double, double, double) { return MetricTensor(); }
    static MetricTensor kerr(double, double, double, double) { return MetricTensor(); }
    void setToMinkowski() { *this = MetricTensor(); }
    double determinant() const { return -1.0; }
    double interval(const Event4D&, const Event4D&) const { return -1.0; }
    double properTime(const Event4D&, const Event4D&) const { return 0.0; }
};
}