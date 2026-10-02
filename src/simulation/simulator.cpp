#include "simulation/simulator.hpp"

#include "crypto/signature_scheme.hpp"
#include "network/message.hpp"
#include "network/vehicle.hpp"
#ifdef V2X_WITH_NS3
#include "network/wave_channel.hpp"
#endif

#include <chrono>
#include <cmath>
#include <memory>
#include <random>
#include <set>
#include <stdexcept>
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
    const auto sizes = signer->sizes();
    std::vector<network::Vehicle> vehicles;
    for (int id = 0; id < configuration_.vehicles; ++id) {
        const auto key = static_cast<std::uint64_t>(random()) + 1;
        const auto keyGenerationStart = std::chrono::steady_clock::now();
        auto keyPair = signer->generateKeyPair(key);
        result.metrics.keyGenerationUs += std::chrono::duration<double, std::micro>(
            std::chrono::steady_clock::now() - keyGenerationStart).count();
        vehicles.emplace_back(static_cast<NodeId>(id), Position{coordinate(random), coordinate(random)},
                              Velocity{configuration_.mobilityMetersPerSecond * 3.6, heading(random)},
                              std::move(keyPair));
    }

    const double tickSeconds = 1.0 / configuration_.messageRateHz;
    result.ticks = static_cast<std::uint64_t>(std::ceil(configuration_.durationSeconds / tickSeconds));
    std::vector<std::uint64_t> sequence(vehicles.size());
    std::unordered_map<std::uint64_t, std::uint64_t> highestSequence;
    MessageId messageId = 0;
#ifdef V2X_WITH_NS3
    std::unique_ptr<network::WaveChannel> waveChannel;
    if (configuration_.networkBackend == "ns3_wave") {
        waveChannel = std::make_unique<network::WaveChannel>(
            vehicles.size(), configuration_.communicationRangeMeters);
    }
#else
    if (configuration_.networkBackend == "ns3_wave") {
        throw std::runtime_error("ns3_wave backend requested; rebuild with -DV2X_ENABLE_NS3=ON and ns-3 installed");
    }
#endif
    for (std::uint64_t tick = 0; tick < result.ticks; ++tick) {
        const double timestamp = tick * tickSeconds;
        std::vector<network::Message> tickMessages;
        tickMessages.reserve(vehicles.size());
#ifdef V2X_WITH_NS3
        std::vector<network::WaveTransmission> waveTransmissions;
        waveTransmissions.reserve(vehicles.size());
#endif
        for (auto& sender : vehicles) {
            network::Message message{++messageId, sender.id(), sequence[sender.id()]++, timestamp,
                                     sender.position(), sender.velocity(), MessageType::Cam,
                                     "cooperative-awareness", {}};
            const auto serialized = message.serialize();
            const auto signingStart = std::chrono::steady_clock::now();
            message.signature = signer->sign(serialized, sender.keys());
            result.metrics.signingUs += std::chrono::duration<double, std::micro>(
                std::chrono::steady_clock::now() - signingStart).count();
            ++result.metrics.generated;
            tickMessages.push_back(message);
#ifdef V2X_WITH_NS3
            if (waveChannel) {
                waveTransmissions.push_back({message.id, sender.id(),
                    message.unsignedBytes() + sizes.signatureBytes,
                    std::uniform_real_distribution<double>(0.0, tickSeconds)(random)});
                continue;
            }
#endif
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
                const auto verificationStart = std::chrono::steady_clock::now();
                const bool valid = signer->verify(received.serialize(), received.signature, sender.keys().publicKey);
                const double verificationDurationUs = std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - verificationStart).count();
                result.metrics.verificationUs += verificationDurationUs;
                ++result.metrics.verificationOperations;
                if (replay) ++result.metrics.replayDetected;
                if (!valid) {
                    if (configuration_.tamperAttack && message.id % 17 == 0) ++result.metrics.tamperDetected;
                    ++result.metrics.rejected;
                } else if (!replay) {
                    if (forged) ++result.metrics.forgedAccepted;
                    ++result.metrics.verified;
                    highestSequence[replayKey] = received.sequence;
                    result.metrics.authenticationLatencyUs.push_back(
                        verificationDurationUs + configuration_.networkLatencyMs * 1000.0);
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
                    const auto verificationStart = std::chrono::steady_clock::now();
                    const bool valid = signer->verify(message.serialize(), message.signature,
                                                      sender.keys().publicKey);
                    const double verificationDurationUs = std::chrono::duration<double, std::micro>(
                        std::chrono::steady_clock::now() - verificationStart).count();
                    result.metrics.verificationUs += verificationDurationUs;
                    ++result.metrics.verificationOperations;
                    if (valid) {
                        ++result.metrics.verified;
                        result.metrics.authenticationLatencyUs.push_back(
                            verificationDurationUs + configuration_.networkLatencyMs * 1000.0);
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
#ifdef V2X_WITH_NS3
        if (waveChannel) {
            std::vector<Position> positions;
            positions.reserve(vehicles.size() + 1);
            for (const auto& vehicle : vehicles) positions.push_back(vehicle.position());
            positions.push_back({500.0, 500.0});

            const auto receptions = waveChannel->broadcast(positions, waveTransmissions, tickSeconds);
            std::set<std::pair<MessageId, NodeId>> received;
            for (const auto& reception : receptions) {
                const auto& message = tickMessages.at(reception.message - tickMessages.front().id);
                if (reception.receiver == message.sender) continue;
                if (!received.emplace(reception.message, reception.receiver).second) continue;
                if (probability(random) < configuration_.packetLoss) {
                    ++result.metrics.channelLoss;
                    continue;
                }

                ++result.metrics.delivered;
                result.metrics.bytesOnWire += message.unsignedBytes() + sizes.signatureBytes;
                const bool isRsu = reception.receiver == vehicles.size();
                const double networkLatencyUs = reception.latencySeconds * 1000000.0;
                result.metrics.networkLatencyUs += networkLatencyUs;
                if (isRsu) ++result.metrics.rsuDelivered;
                network::Message receivedMessage = message;
                const bool forged = !isRsu && configuration_.tamperAttack && message.id % 17 == 0;
                if (forged) {
                    ++result.metrics.forgeAttempts;
                    receivedMessage.payload = "tampered";
                }
                if (!isRsu && configuration_.replayAttack && message.id % 19 == 0) {
                    receivedMessage.sequence = 0;
                }

                bool replay = false;
                if (!isRsu) {
                    const auto replayKey = (static_cast<std::uint64_t>(reception.receiver) << 32U) |
                                           receivedMessage.sender;
                    const auto previous = highestSequence.find(replayKey);
                    replay = previous != highestSequence.end() &&
                             receivedMessage.sequence <= previous->second;
                    if (replay) ++result.metrics.replayDetected;
                }
                const auto verificationStart = std::chrono::steady_clock::now();
                const bool valid = signer->verify(receivedMessage.serialize(), receivedMessage.signature,
                                                  vehicles[message.sender].keys().publicKey);
                const double verificationDurationUs = std::chrono::duration<double, std::micro>(
                    std::chrono::steady_clock::now() - verificationStart).count();
                result.metrics.verificationUs += verificationDurationUs;
                ++result.metrics.verificationOperations;
                if (!valid) {
                    if (forged) ++result.metrics.tamperDetected;
                    ++result.metrics.rejected;
                } else if (!replay) {
                    if (forged) ++result.metrics.forgedAccepted;
                    ++result.metrics.verified;
                    if (!isRsu) {
                        const auto replayKey = (static_cast<std::uint64_t>(reception.receiver) << 32U) |
                                               receivedMessage.sender;
                        highestSequence[replayKey] = receivedMessage.sequence;
                    }
                    result.metrics.authenticationLatencyUs.push_back(
                        verificationDurationUs + networkLatencyUs);
                }
            }

            for (const auto& message : tickMessages) {
                for (NodeId receiver = 0; receiver <= vehicles.size(); ++receiver) {
                    if (receiver == message.sender) continue;
                    ++result.metrics.deliveryAttempts;
                    if (receiver == vehicles.size()) ++result.metrics.rsuDeliveryAttempts;
                    if (!received.contains({message.id, receiver})) {
                        const auto& senderPosition = positions[message.sender];
                        if (senderPosition.distanceTo(positions[receiver]) >
                            configuration_.communicationRangeMeters) {
                            ++result.metrics.outOfRange;
                        } else {
                            ++result.metrics.channelLoss;
                        }
                    }
                }
            }
        }
#endif
        for (auto& vehicle : vehicles) vehicle.move(tickSeconds);
    }
    if (configuration_.networkBackend == "abstract") {
        result.metrics.networkLatencyUs = result.metrics.delivered * configuration_.networkLatencyMs * 1000.0;
    }
    result.metrics.memoryUsageBytes = vehicles.size() * (sizes.publicKeyBytes + sizes.privateKeyBytes + sizes.signatureBytes);
    result.metrics.executionTimeUs = static_cast<double>(
        std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - startTime).count());
    return result;
}

}  // namespace v2x::simulation
