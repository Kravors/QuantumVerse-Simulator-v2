#include <iostream>
#include "discovery/NeutronStarGlitchPhaseDetector.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

int main() {
    std::cout << "=== NeutronStarGlitchPhaseDetector Test ===" << std::endl;
    int passed = 0, total = 2;
    
    std::cout << "\n[Test 1] Instrument initialization" << std::endl;
    try {
        NeutronStarGlitchPhaseDetector detector;
        detector.setEnabled(true);
        MetricTensor metric;
        metric.setToMinkowski();
        Event4D location(0, 0, 0, 0);
        auto findings = detector.analyze(metric, location, {});
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\n[Test 2] Glitch amplitude parameter" << std::endl;
    try {
        NeutronStarGlitchPhaseDetector detector;
        detector.setParameter("glitch_amplitude", 1e-9);
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\nPassed: " << passed << "/" << total << std::endl;
    return (passed == total) ? 0 : 1;
}