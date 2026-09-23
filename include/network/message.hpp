#pragma once

#include "core/types.hpp"
#include "crypto/signature_scheme.hpp"

#include <cstdint>
#include <string>

namespace v2x::network {

struct Message {
    MessageId id{};
    NodeId sender{};
    std::uint64_t sequence{};
    Timestamp timestamp{};
    Position position;
    Velocity velocity;
    MessageType type{MessageType::Cam};
    std::string payload;
    crypto::Signature signature;

    std::string serialize() const;
    std::size_t unsignedBytes() const;
};

}  // namespace v2x::network
