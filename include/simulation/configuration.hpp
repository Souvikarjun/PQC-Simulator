#pragma once

#include "core/types.hpp"

#include <cstdint>
#include <string>

namespace v2x::simulation {

struct Configuration {
    std::string experimentName{"baseline"};
    int vehicles{20};
    double durationSeconds{10.0};
    double messageRateHz{10.0};
    double communicationRangeMeters{250.0};
    double packetLoss{0.02};
    double networkLatencyMs{2.0};
    double bandwidthMbps{6.0};
    double mobilityMetersPerSecond{0.0};
    std::string networkBackend{"abstract"};
    std::uint32_t seed{42};
    int repetitions{1};
    std::string algorithm{"ML-DSA-44"};
    CryptoMode mode{CryptoMode::LiboqsPqc};
    std::string csvPath;
    std::string jsonPath;
    bool replayAttack{false};
    bool tamperAttack{false};
};

Configuration loadConfigurationFile(const std::string& path);
void validate(const Configuration& configuration);

}  // namespace v2x::simulation
