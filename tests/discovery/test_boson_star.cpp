#include <iostream>
#include "discovery/BosonStarCollisionPredictor.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

int main() {
    std::cout << "=== BosonStarCollisionPredictor Test ===" << std::endl;
    int passed = 0, total = 2;
    
    std::cout << "\n[Test 1] Instrument initialization" << std::endl;
    try {
        BosonStarCollisionPredictor predictor;
        predictor.setEnabled(true);
        MetricTensor metric;
        metric.setToMinkowski();
        Event4D location(0, 0, 0, 0);
        auto findings = predictor.analyze(metric, location, {});
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\n[Test 2] Mass parameter configuration" << std::endl;
    try {
        BosonStarCollisionPredictor predictor;
        predictor.setParameter("mass_msun", 100.0);
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\nPassed: " << passed << "/" << total << std::endl;
    return (passed == total) ? 0 : 1;
}