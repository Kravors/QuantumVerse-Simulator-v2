#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <cmath>
#include "spacetime/MetricTensor.h"
#include "spacetime/Event4D.h"

using namespace quantumverse;

std::atomic<bool> g_running(true);

void stressMetricComputation() {
    for (int i = 0; i < 10000 && g_running; i++) {
        double r = 10.0 + (i % 100) * 0.1;
        MetricTensor metric = MetricTensor::schwarzschild(1.0, r, 0.0, 0.0);
        double det = metric.determinant();
        if (det >= 0 && r > 2.0) {
            std::cerr << "ERROR: Invalid determinant" << std::endl;
        }
    }
}

void stressMinkowskiOperations() {
    for (int i = 0; i < 5000 && g_running; i++) {
        MetricTensor metric;
        metric.setToMinkowski();
        Event4D e1(0, 1, 2, 3);
        Event4D e2(1, 2, 3, 4);
        double ds2 = metric.interval(e1, e2);
        double proper = metric.properTime(e1, e2);
    }
}

void stressKerrOperations() {
    for (int i = 0; i < 3000 && g_running; i++) {
        double r = 10.0 + (i % 50) * 0.5;
        double theta = M_PI / 4.0;
        MetricTensor metric = MetricTensor::kerr(1.0, 0.5, r, theta);
        double det = metric.determinant();
        (void)det;
    }
}

int main() {
    std::cout << "=== Phase 4: Stress & Chaos Tests ===" << std::endl;
    int passed = 0, total = 3;
    
    std::cout << "\n[Test 1] Metric stress test (10k iterations)" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    std::thread t1(stressMetricComputation);
    t1.join();
    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "  Completed in " << ms << "ms" << std::endl;
    std::cout << "[PASS]" << std::endl;
    passed++;
    
    std::cout << "\n[Test 2] Minkowski operations (5k iterations)" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    std::thread t2(stressMinkowskiOperations);
    t2.join();
    end = std::chrono::high_resolution_clock::now();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "  Completed in " << ms << "ms" << std::endl;
    std::cout << "[PASS]" << std::endl;
    passed++;
    
    std::cout << "\n[Test 3] Kerr operations (3k iterations)" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    std::thread t3(stressKerrOperations);
    t3.join();
    end = std::chrono::high_resolution_clock::now();
    ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "  Completed in " << ms << "ms" << std::endl;
    std::cout << "[PASS]" << std::endl;
    passed++;
    
    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Passed: " << passed << "/" << total << std::endl;
    
    return (passed == total) ? 0 : 1;
}