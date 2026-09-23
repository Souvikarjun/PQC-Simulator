#pragma once

#include "core/types.hpp"
#include "crypto/signature_scheme.hpp"

namespace v2x::network {

class Vehicle {
public:
    Vehicle(NodeId id, Position position, Velocity velocity,
            crypto::SignatureKeyPair keys);

    NodeId id() const { return id_; }
    const Position& position() const { return position_; }
    const Velocity& velocity() const { return velocity_; }
    const crypto::SignatureKeyPair& keys() const { return keys_; }
    void move(double seconds);

private:
    NodeId id_;
    Position position_;
    Velocity velocity_;
    crypto::SignatureKeyPair keys_;
};

}  // namespace v2x::network
