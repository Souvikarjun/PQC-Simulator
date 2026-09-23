#pragma once

#include <cmath>
#include <cstdint>
#include <string>

namespace v2x {

using NodeId = std::uint32_t;
using MessageId = std::uint64_t;
using Timestamp = double;

struct Position {
    double x{};
    double y{};

    double distanceTo(const Position& other) const {
        return std::hypot(x - other.x, y - other.y);
    }
};

struct Velocity {
    double speedKph{};
    double headingDegrees{};
};

enum class MessageType { Cam, Denm, Generic };

enum class CryptoMode { ModeledPqc, ClassicalBaseline, Hybrid };

inline std::string toString(CryptoMode mode) {
    switch (mode) {
    case CryptoMode::ModeledPqc: return "modeled_pqc";
    case CryptoMode::ClassicalBaseline: return "classical_baseline";
    case CryptoMode::Hybrid: return "hybrid";
    }
    return "unknown";
}

inline std::string toString(MessageType type) {
    switch (type) {
    case MessageType::Cam: return "CAM";
    case MessageType::Denm: return "DENM";
    case MessageType::Generic: return "GENERIC";
    }
    return "UNKNOWN";
}

}  // namespace v2x
