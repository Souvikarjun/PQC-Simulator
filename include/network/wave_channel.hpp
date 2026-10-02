#pragma once

#include "core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace v2x::network {

struct WaveTransmission {
    MessageId id{};
    NodeId sender{};
    std::size_t payloadBytes{};
    double offsetSeconds{};
};

struct WaveReception {
    MessageId message{};
    NodeId receiver{};
    double latencySeconds{};
};

class WaveChannel {
public:
    WaveChannel(std::size_t vehicleCount, double communicationRangeMeters);
    ~WaveChannel();

    WaveChannel(const WaveChannel&) = delete;
    WaveChannel& operator=(const WaveChannel&) = delete;

    std::vector<WaveReception> broadcast(const std::vector<Position>& positions,
                                         const std::vector<WaveTransmission>& transmissions,
                                         double intervalSeconds);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace v2x::network