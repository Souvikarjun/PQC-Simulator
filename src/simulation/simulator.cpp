#include "simulation/simulator.hpp"

#include "crypto/signature_scheme.hpp"
#include "network/message.hpp"
#include "network/vehicle.hpp"

#include <chrono>
#include <cmath>
#include <random>
#include <unordered_map>
#include <vector>

namespace v2x::simulation {

Simulator::Simulator(Configuration configuration) : configuration_(std::move(configuration)) { validate(configuration_); }

RunResult Simulator::run() const {
    const auto startTime = std::chrono::steady_clock::now();
    RunResult result{configuration_, {}, 0};
    const auto signer = crypto::createSignatureScheme(configuration_.algorithm);
    std::mt19937 random(configuration_.seed);
    std::uniform_real_distribution<double> coordinate(0.0, 1000.0);
    std::uniform_real_distribution<double> heading(0.0, 360.0);
    std::uniform_real_distribution<double> probability(0.0, 1.0);
    const auto timing = signer->timing();
    const auto sizes = signer->sizes();
    std::vector<network::Vehicle> vehicles;
    for (int id = 0; id < configuration_.vehicles; ++id) {
        const auto key = static_cast<std::uint64_t>(random()) + 1;
        vehicles.emplace_back(static_cast<NodeId>(id), Position{coordinate(random), coordinate(random)},
                              Velocity{configuration_.mobilityMetersPerSecond * 3.6, heading(random)},
                              signer->generateKeyPair(key));
        result.metrics.keyGenerationUs += timing.keyGenerationUs;
    }

    const double tickSeconds = 1.0 / configuration_.messageRateHz;
    result.ticks = static_cast<std::uint64_t>(std::ceil(configuration_.durationSeconds / tickSeconds));
    std::vector<std::uint64_t> sequence(vehicles.size());
    std::unordered_map<std::uint64_t, std::uint64_t> highestSequence;
    MessageId messageId = 0;
    for (std::uint64_t tick = 0; tick < result.ticks; ++tick) {
        const double timestamp = tick * tickSeconds;
        for (auto& sender : vehicles) {
            network::Message message{++messageId, sender.id(), sequence[sender.id()]++, timestamp,
                                     sender.position(), sender.velocity(), MessageType::Cam,
                                     "cooperative-awareness", {}};
            const auto serialized = message.serialize();
            message.signature = signer->sign(serialized, sender.keys());
            ++result.metrics.generated;
            result.metrics.signingUs += timing.signingUs;
            for (const auto& receiver : vehicles) {
                if (receiver.id() == sender.id()) continue;
                ++result.metrics.deliveryAttempts;
                if (sender.position().distanceTo(receiver.position()) > configuration_.communicationRangeMeters) {
                    ++result.metrics.outOfRange;
                    continue;
                }
                if (probability(random) < configuration_.packetLoss) {
                    ++result.metrics.channelLoss;
                    continue;
                }
                ++result.metrics.delivered;
                result.metrics.bytesOnWire += message.unsignedBytes() + sizes.signatureBytes;
                network::Message received = message;
                const bool forged = configuration_.tamperAttack && message.id % 17 == 0;
                if (forged) {
                    ++result.metrics.forgeAttempts;
                    received.payload = "tampered";
                }
                if (configuration_.replayAttack && message.id % 19 == 0) received.sequence = 0;
                const auto replayKey = (static_cast<std::uint64_t>(receiver.id()) << 32U) | received.sender;
                const auto previous = highestSequence.find(replayKey);
                const bool replay = previous != highestSequence.end() && received.sequence <= previous->second;
                const bool valid = signer->verify(received.serialize(), received.signature, sender.keys().publicKey);
                ++result.metrics.verificationOperations;
                result.metrics.verificationUs += timing.verificationUs;
                if (replay) ++result.metrics.replayDetected;
                if (!valid) {
                    if (configuration_.tamperAttack && message.id % 17 == 0) ++result.metrics.tamperDetected;
                    ++result.metrics.rejected;
                } else if (!replay) {
                    if (forged) ++result.metrics.forgedAccepted;
                    ++result.metrics.verified;
                    highestSequence[replayKey] = received.sequence;
                    result.metrics.authenticationLatencyUs.push_back(
                        timing.verificationUs + configuration_.networkLatencyMs * 1000.0);
                }
            }
            ++result.metrics.deliveryAttempts;
            ++result.metrics.rsuDeliveryAttempts;
            const Position rsuPosition{500.0, 500.0};
            if (sender.position().distanceTo(rsuPosition) <= configuration_.communicationRangeMeters) {
                if (probability(random) >= configuration_.packetLoss) {
                    ++result.metrics.delivered;
                    ++result.metrics.rsuDelivered;
                    result.metrics.bytesOnWire += message.unsignedBytes() + sizes.signatureBytes;
                    const bool valid = signer->verify(message.serialize(), message.signature,
                                                      sender.keys().publicKey);
                    ++result.metrics.verificationOperations;
                    result.metrics.verificationUs += timing.verificationUs;
                    if (valid) {
                        ++result.metrics.verified;
                        result.metrics.authenticationLatencyUs.push_back(
                            timing.verificationUs + configuration_.networkLatencyMs * 1000.0);
                    } else {
                        ++result.metrics.rejected;
                    }
                } else {
                    ++result.metrics.channelLoss;
                }
            } else {
                ++result.metrics.outOfRange;
            }
        }
        for (auto& vehicle : vehicles) vehicle.move(tickSeconds);
    }
    result.metrics.networkLatencyUs = result.metrics.delivered * configuration_.networkLatencyMs * 1000.0;
    result.metrics.memoryUsageBytes = vehicles.size() * (sizes.publicKeyBytes + sizes.privateKeyBytes + sizes.signatureBytes);
    result.metrics.executionTimeUs = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
    return result;
}

}  // namespace v2x::simulation
