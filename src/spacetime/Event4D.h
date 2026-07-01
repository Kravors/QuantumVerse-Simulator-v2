#pragma once
#include <array>

namespace quantumverse {
class Event4D {
public:
    double t, x, y, z;
    Event4D(double t_ = 0, double x_ = 0, double y_ = 0, double z_ = 0) : t(t_), x(x_), y(y_), z(z_) {}
};
}