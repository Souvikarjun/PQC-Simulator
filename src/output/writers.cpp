#include "output/writers.hpp"

#include "metrics/metrics.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace v2x::output {

void writeCsv(const std::string& path, const simulation::RunResult& result) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot open CSV output: " + path);
    const auto& c = result.configuration;
    const auto& m = result.metrics;
    output << "experiment,algorithm,vehicles,duration_s,message_rate_hz,seed,ticks,generated,"
              "attempts,rsu_attempts,delivered,rsu_delivered,verified,rejected,replay_detected,tamper_detected,channel_loss,"
              "out_of_range,bytes_on_wire,execution_type,timeframe_seconds,total_vehicles,execution_time_us,"
              "key_generation_time_us,encryption_time_us,decryption_time_us,signing_time_us,memory_usage_bytes,"
              "forge_attempts,forged_accepted,forgeability_ratio,delivery_ratio,auth_ratio\n";
    const double deliveryRatio = m.deliveryAttempts == 0 ? 0.0 : static_cast<double>(m.delivered) / m.deliveryAttempts;
    const double authRatio = m.delivered == 0 ? 0.0 : static_cast<double>(m.verified) / m.delivered;
    const double forgeabilityRatio = m.forgeAttempts == 0 ? 0.0 :
        static_cast<double>(m.forgedAccepted) / m.forgeAttempts;
    output << c.experimentName << ',' << c.algorithm << ',' << c.vehicles << ',' << c.durationSeconds << ','
           << c.messageRateHz << ',' << c.seed << ',' << result.ticks << ',' << m.generated << ','
           << m.deliveryAttempts << ',' << m.rsuDeliveryAttempts << ',' << m.delivered << ',' << m.rsuDelivered << ','
           << m.verified << ',' << m.rejected << ','
           << m.replayDetected << ',' << m.tamperDetected << ',' << m.channelLoss << ',' << m.outOfRange << ','
           << m.bytesOnWire << ',' << v2x::toString(c.mode) << ',' << c.durationSeconds << ',' << c.vehicles << ','
           << m.executionTimeUs << ',' << m.keyGenerationUs << ',' << m.signingUs << ',' << m.verificationUs << ','
           << m.signingUs << ',' << m.memoryUsageBytes << ',' << m.forgeAttempts << ',' << m.forgedAccepted << ','
           << forgeabilityRatio << ',' << deliveryRatio << ',' << authRatio << '\n';
}

void writeJson(const std::string& path, const simulation::RunResult& result) {
    std::ofstream output(path);
    if (!output) throw std::runtime_error("cannot open JSON output: " + path);
    const auto& c = result.configuration;
    const auto& m = result.metrics;
    const double forgeabilityRatio = m.forgeAttempts == 0 ? 0.0 :
        static_cast<double>(m.forgedAccepted) / m.forgeAttempts;
    output << std::fixed << std::setprecision(6)
           << "{\n  \"experiment\": \"" << c.experimentName << "\",\n"
           << "  \"algorithm\": \"" << c.algorithm << "\",\n"
           << "  \"vehicles\": " << c.vehicles << ",\n"
           << "  \"duration_seconds\": " << c.durationSeconds << ",\n"
           << "  \"message_rate_hz\": " << c.messageRateHz << ",\n"
           << "  \"seed\": " << c.seed << ",\n"
           << "  \"generated\": " << m.generated << ",\n"
           << "  \"delivery_attempts\": " << m.deliveryAttempts << ",\n"
           << "  \"rsu_delivery_attempts\": " << m.rsuDeliveryAttempts << ",\n"
           << "  \"delivered\": " << m.delivered << ",\n"
           << "  \"rsu_delivered\": " << m.rsuDelivered << ",\n"
           << "  \"verified\": " << m.verified << ",\n"
           << "  \"rejected\": " << m.rejected << ",\n"
           << "  \"replay_detected\": " << m.replayDetected << ",\n"
           << "  \"tamper_detected\": " << m.tamperDetected << ",\n"
           << "  \"channel_loss\": " << m.channelLoss << ",\n"
           << "  \"out_of_range\": " << m.outOfRange << ",\n"
           << "  \"bytes_on_wire\": " << m.bytesOnWire << ",\n"
           << "  \"result\": {\n"
           << "    \"execution_type\": \"" << v2x::toString(c.mode) << "\",\n"
           << "    \"timeframe_seconds\": " << c.durationSeconds << ",\n"
           << "    \"total_vehicles\": " << c.vehicles << ",\n"
           << "    \"execution_time_us\": " << m.executionTimeUs << ",\n"
           << "    \"key_generation_time_us\": " << m.keyGenerationUs << ",\n"
           << "    \"encryption_time_us\": " << m.signingUs << ",\n"
           << "    \"decryption_time_us\": " << m.verificationUs << ",\n"
           << "    \"signing_time_us\": " << m.signingUs << ",\n"
           << "    \"memory_usage_bytes\": " << m.memoryUsageBytes << ",\n"
           << "    \"forge_attempts\": " << m.forgeAttempts << ",\n"
           << "    \"forged_accepted\": " << m.forgedAccepted << ",\n"
           << "    \"forgeability_ratio\": " << forgeabilityRatio << "\n  },\n"
           << "  \"signing_us\": " << m.signingUs << ",\n"
           << "  \"verification_us\": " << m.verificationUs << "\n}\n";
}

void printSummary(const simulation::RunResult& result) {
    const auto& c = result.configuration;
    const auto& m = result.metrics;
    const double deliveryRatio = m.deliveryAttempts == 0 ? 0.0 : 100.0 * m.delivered / m.deliveryAttempts;
    const double authenticationRatio = m.delivered == 0 ? 0.0 : 100.0 * m.verified / m.delivered;
    std::cout << std::fixed << std::setprecision(2)
              << "PQC V2X research simulator\n"
              << "Experiment:              " << c.experimentName << "\n"
              << "Algorithm:               " << c.algorithm << "\n"
              << "Vehicles / duration:     " << c.vehicles << " / " << c.durationSeconds << " s\n"
              << "Message rate:             " << c.messageRateHz << " Hz\n"
              << "Generated messages:      " << m.generated << "\n"
              << "Delivery attempts:       " << m.deliveryAttempts << "\n"
              << "RSU attempts/delivered:   " << m.rsuDeliveryAttempts << "/" << m.rsuDelivered << "\n"
              << "Delivered:                " << m.delivered << " (" << deliveryRatio << "%)\n"
              << "Authenticated:            " << m.verified << " (" << authenticationRatio << "%)\n"
              << "Rejected:                 " << m.rejected << "\n"
              << "Replay detected:          " << m.replayDetected << "\n"
              << "Tampering detected:       " << m.tamperDetected << "\n"
              << "Channel loss:             " << m.channelLoss << "\n"
              << "Out of range:             " << m.outOfRange << "\n"
              << "Bytes on wire:            " << m.bytesOnWire << "\n"
              << "Execution type:           " << v2x::toString(c.mode) << "\n"
              << "Execution time:           " << m.executionTimeUs / 1000.0 << " ms\n"
              << "Key generation time:      " << m.keyGenerationUs / 1000.0 << " ms\n"
              << "Modeled signing time:     " << m.signingUs / 1000.0 << " ms\n"
              << "Modeled verification:     " << m.verificationUs / 1000.0 << " ms\n";
    if (!m.authenticationLatencyUs.empty()) {
        const auto stats = metrics::summarize(m.authenticationLatencyUs);
        std::cout << "Authentication latency:   " << stats.mean << " us mean, "
                  << stats.confidence95 << " us 95% CI\n";
    }
}

}  // namespace v2x::output
