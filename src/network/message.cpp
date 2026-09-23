#include "network/message.hpp"

#include <iomanip>
#include <sstream>

namespace v2x::network {

std::string Message::serialize() const {
    std::ostringstream output;
    output << id << '|' << sender << '|' << sequence << '|'
           << std::fixed << std::setprecision(6) << timestamp << '|'
           << position.x << '|' << position.y << '|'
           << velocity.speedKph << '|' << velocity.headingDegrees << '|'
           << toString(type) << '|' << payload;
    return output.str();
}

std::size_t Message::unsignedBytes() const { return serialize().size(); }

}  // namespace v2x::network
