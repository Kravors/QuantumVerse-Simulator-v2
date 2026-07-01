#pragma once
#include <vector>
#include "../spacetime/MetricTensor.h"
#include "../spacetime/Event4D.h"

namespace quantumverse {
enum class AlertSeverity { LOW, MEDIUM, HIGH, CRITICAL };
struct DiscoveryFinding { AlertSeverity severity; std::string message; };

class ExoplanetaryTTVFifthForceHunter {
public:
    void setEnabled(bool) {}
    void setParameter(const std::string&, double) {}
    std::vector<DiscoveryFinding> analyze(const MetricTensor&, const Event4D&, const std::vector<Event4D>&) { return {}; }
};
}