#include <iostream>
#include <cmath>
#include "discovery/FineStructureConstantDriftObservatory.h"
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

int main() {
    std::cout << "=== FineStructureConstantDriftObservatory Test ===" << std::endl;
    int passed = 0, total = 2;
    
    std::cout << "\n[Test 1] Instrument initialization" << std::endl;
    try {
        FineStructureConstantDriftObservatory observatory;
        observatory.setEnabled(true);
        MetricTensor metric;
        metric.setToMinkowski();
        Event4D location(0, 0, 0, 0);
        auto findings = observatory.analyze(metric, location, {});
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\n[Test 2] Redshift configuration" << std::endl;
    try {
        FineStructureConstantDriftObservatory observatory;
        observatory.setParameter("redshift", 2.0);
        std::cout << "[PASS]" << std::endl;
        passed++;
    } catch (...) { std::cout << "[FAIL]" << std::endl; }
    
    std::cout << "\nPassed: " << passed << "/" << total << std::endl;
    return (passed == total) ? 0 : 1;
}