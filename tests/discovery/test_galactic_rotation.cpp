#include <iostream>
#include "discovery/GalacticRotationCurveScanner.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

int main() {
    std::cout << "=== GalacticRotationCurveScanner Test ===" << std::endl;
    int passed = 0, total = 2;
    
    std::cout << "\n[Test 1] Instrument initialization" << std::endl;
    try {
        GalacticRotationCurveScanner scanner;
        scanner.setEnabled(true);
        MetricTensor metric;
        metric.setToMinkowski();
        Event4D location(0, 10.0, 0, 0);
        auto findings = scanner.analyze(metric, location, {});
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\n[Test 2] Parameter setting" << std::endl;
    try {
        GalacticRotationCurveScanner scanner;
        scanner.setParameter("distance_kpc", 10.0);
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\nPassed: " << passed << "/" << total << std::endl;
    return (passed == total) ? 0 : 1;
}