#include "simulation/configuration.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace v2x::simulation {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

void apply(Configuration& config, const std::string& key, const std::string& value) {
    if (key == "experiment.name") config.experimentName = value;
    else if (key == "network.vehicles") config.vehicles = std::stoi(value);
    else if (key == "network.duration") config.durationSeconds = std::stod(value);
    else if (key == "network.message_rate") config.messageRateHz = std::stod(value);
    else if (key == "network.communication_range") config.communicationRangeMeters = std::stod(value);
    else if (key == "network.packet_loss") config.packetLoss = std::stod(value);
    else if (key == "network.latency_ms") config.networkLatencyMs = std::stod(value);
    else if (key == "network.bandwidth_mbps") config.bandwidthMbps = std::stod(value);
    else if (key == "network.mobility_mps") config.mobilityMetersPerSecond = std::stod(value);
    else if (key == "network.backend") config.networkBackend = value;
    else if (key == "cryptography.algorithm") config.algorithm = value;
    else if (key == "simulation.seed") config.seed = static_cast<std::uint32_t>(std::stoul(value));
    else if (key == "simulation.repetitions") config.repetitions = std::stoi(value);
    else if (key == "attack.replay") config.replayAttack = value == "true";
    else if (key == "attack.tamper") config.tamperAttack = value == "true";
}

}  // namespace

Configuration loadConfigurationFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open configuration: " + path);
    Configuration config;
    std::string section;
    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (line.back() == ':') { section = line.substr(0, line.size() - 1); continue; }
        const auto separator = line.find(':');
        if (separator == std::string::npos) throw std::invalid_argument("invalid configuration line: " + line);
        std::string key = trim(line.substr(0, separator));
        std::string value = trim(line.substr(separator + 1));
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') value = value.substr(1, value.size() - 2);
        apply(config, section.empty() ? key : section + "." + key, value);
    }
    validate(config);
    return config;
}

void validate(const Configuration& config) {
    if (config.vehicles <= 0 || config.durationSeconds <= 0 || config.messageRateHz <= 0 ||
        config.communicationRangeMeters <= 0 || config.packetLoss < 0 || config.packetLoss > 1 ||
        config.bandwidthMbps <= 0 || config.repetitions <= 0 ||
        (config.networkBackend != "abstract" && config.networkBackend != "ns3_wave")) {
        throw std::invalid_argument("invalid simulation configuration");
    }
}

}  // namespace v2x::simulation
