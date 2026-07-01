#pragma once
#include <vector>
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {
class FineStructureConstantDriftObservatory {
public:
    void setEnabled(bool) {}
    void setParameter(const std::string&, double) {}
    std::vector<int> analyze(const MetricTensor&, const Event4D&, const std::vector<Event4D>&) { return {}; }
};
}