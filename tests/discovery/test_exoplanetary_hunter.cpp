#include <iostream>
#include <cmath>
#include <vector>
#include "discovery/ExoplanetaryTTVFifthForceHunter.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

int main() {
    std::cout << "=== Discovery Synthetic Tests ===" << std::endl;
    int passed = 0, total = 2;
    
    std::cout << "\n[Test 1] Instrument initialization" << std::endl;
    try {
        ExoplanetaryTTVFifthForceHunter hunter;
        hunter.setEnabled(true);
        MetricTensor metric;
        metric.setToMinkowski();
        Event4D location(0, 1.0, 0, 0);
        auto findings = hunter.analyze(metric, location, {});
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\n[Test 2] Parameter configuration" << std::endl;
    try {
        ExoplanetaryTTVFifthForceHunter hunter;
        hunter.setParameter("threshold_sigma", 5.0);
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\nPassed: " << passed << "/" << total << std::endl;
    return (passed == total) ? 0 : 1;
}