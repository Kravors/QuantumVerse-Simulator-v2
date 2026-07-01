#pragma once

#include <string>
#include <chrono>
#include <sstream>

namespace quantumverse {

class Telemetry {
public:
    static Telemetry& instance() {
        static Telemetry inst;
        return inst;
    }

    void startFrame() { frameStart_ = std::chrono::high_resolution_clock::now(); }
    
    void endFrame() {
        auto end = std::chrono::high_resolution_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - frameStart_).count();
        frameTimes_.push_back(ms);
    }

    void recordMetric(const std::string& name, double value) {
        metrics_[name].push_back(value);
    }

    void recordMemory(size_t bytes) {
        memorySamples_.push_back(bytes);
    }

    void recordAnomaly(const std::string& type) {
        anomalyCounts_[type]++;
    }

    void reset() {
        frameTimes_.clear();
        metrics_.clear();
        memorySamples_.clear();
        anomalyCounts_.clear();
    }

    std::string exportPrometheus() const {
        std::ostringstream oss;
        oss << "# QuantumVerse Metrics\n";
        
        for (const auto& [name, values] : metrics_) {
            if (!values.empty()) {
                oss << "quantumverse_" << name << " " << values.back() << "\n";
            }
        }
        
        if (!frameTimes_.empty()) {
            double avg = 0;
            for (auto ms : frameTimes_) avg += ms;
            avg /= frameTimes_.size();
            oss << "quantumverse_frame_time_ms " << avg << "\n";
        }
        
        for (const auto& [type, count] : anomalyCounts_) {
            oss << "quantumverse_anomalies_total{type=\"" << type << "\"} " << count << "\n";
        }
        
        return oss.str();
    }

private:
    Telemetry() = default;
    std::chrono::high_resolution_clock::time_point frameStart_;
    std::vector<double> frameTimes_;
    std::map<std::string, std::vector<double>> metrics_;
    std::vector<size_t> memorySamples_;
    std::map<std::string, int> anomalyCounts_;
};

#define TELEMETRY Telemetry::instance()

} // namespace quantumverse