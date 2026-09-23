#include "network/vehicle.hpp"

#include <cmath>

namespace v2x::network {

Vehicle::Vehicle(NodeId id, Position position, Velocity velocity,
                 crypto::SignatureKeyPair keys)
    : id_(id), position_(position), velocity_(velocity), keys_(keys) {}

void Vehicle::move(double seconds) {
    const double radians = velocity_.headingDegrees * 3.14159265358979323846 / 180.0;
    const double metersPerSecond = velocity_.speedKph / 3.6;
    position_.x += std::cos(radians) * metersPerSecond * seconds;
    position_.y += std::sin(radians) * metersPerSecond * seconds;
}

}  // namespace v2x::network
