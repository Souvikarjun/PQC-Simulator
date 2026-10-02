#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace v2x::metrics {

struct SampleStatistics {
    std::size_t count{};
    double mean{};
    double median{};
    double minimum{};
    double maximum{};
    double standardDeviation{};
    double confidence95{};
};

SampleStatistics summarize(std::vector<double> values);

struct Metrics {
    std::uint64_t generated{};
    std::uint64_t deliveryAttempts{};
    std::uint64_t rsuDeliveryAttempts{};
    std::uint64_t delivered{};
    std::uint64_t rsuDelivered{};
    std::uint64_t verified{};
    std::uint64_t rejected{};
    std::uint64_t replayDetected{};
    std::uint64_t tamperDetected{};
    std::uint64_t channelLoss{};
    std::uint64_t outOfRange{};
    std::uint64_t bytesOnWire{};
    std::uint64_t verificationOperations{};
    std::uint64_t memoryUsageBytes{};
    std::uint64_t forgeAttempts{};
    std::uint64_t forgedAccepted{};
    std::uint64_t kemSessionsEstablished{};
    std::uint64_t kemSessionFailures{};
    std::uint64_t kemBytesOnWire{};
    double executionTimeUs{};
    double keyGenerationUs{};
    double signingUs{};
    double verificationUs{};
    double kemEncapsulationUs{};
    double kemDecapsulationUs{};
    double networkLatencyUs{};
    std::vector<double> authenticationLatencyUs;
};

}  // namespace v2x::metrics
