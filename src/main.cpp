#include "crypto/signature_scheme.hpp"
#include "output/writers.hpp"
#include "simulation/configuration.hpp"
#include "simulation/simulator.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void usage(const char* executable) {
    std::cout << "Usage: " << executable << " [options]\n"
              << "  --config FILE       Load simple YAML configuration\n"
              << "  --vehicles N        Number of vehicles\n"
              << "  --duration S        Simulation duration in seconds\n"
              << "  --rate HZ           Message generation rate (default 10)\n"
              << "  --range M           Communication range in meters\n"
              << "  --loss P            Packet loss probability [0,1]\n"
              << "  --algorithm NAME    ML-DSA-44, ML-DSA-65, ML-DSA-87, SLH-DSA-SHA2-128s, Ed25519, ECDSA-P256\n"
              << "  --seed N            Reproducibility seed\n"
              << "  --repetitions N     Number of independent runs\n"
              << "  --replay            Enable replay attack model\n"
              << "  --tamper            Enable message tampering model\n"
              << "  --csv FILE          Write CSV result\n"
              << "  --json FILE         Write JSON result\n"
              << "  --help              Show this help\n";
}

bool value(int& index, int argc, char** argv, std::string& output) {
    if (index + 1 >= argc) return false;
    output = argv[++index];
    return true;
}

bool parse(int argc, char** argv, v2x::simulation::Configuration& config) {
    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        if (option == "--help") { usage(argv[0]); return false; }
        if (option == "--replay") { config.replayAttack = true; continue; }
        if (option == "--tamper") { config.tamperAttack = true; continue; }
        std::string argument;
        if (!value(index, argc, argv, argument)) throw std::invalid_argument("missing value for " + option);
        if (option == "--config") config = v2x::simulation::loadConfigurationFile(argument);
        else if (option == "--vehicles") config.vehicles = std::stoi(argument);
        else if (option == "--duration") config.durationSeconds = std::stod(argument);
        else if (option == "--rate") config.messageRateHz = std::stod(argument);
        else if (option == "--range") config.communicationRangeMeters = std::stod(argument);
        else if (option == "--loss") config.packetLoss = std::stod(argument);
        else if (option == "--algorithm") config.algorithm = argument;
        else if (option == "--seed") config.seed = static_cast<std::uint32_t>(std::stoul(argument));
        else if (option == "--repetitions") config.repetitions = std::stoi(argument);
        else if (option == "--csv") config.csvPath = argument;
        else if (option == "--json") config.jsonPath = argument;
        else throw std::invalid_argument("unknown option: " + option);
    }
    v2x::simulation::validate(config);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    try {
        v2x::simulation::Configuration config;
        if (!parse(argc, argv, config)) return 0;
        const auto supported = v2x::crypto::supportedSignatureSchemes();
        if (std::find(supported.begin(), supported.end(), config.algorithm) == supported.end()) {
            throw std::invalid_argument("unsupported algorithm: " + config.algorithm);
        }
        const auto result = v2x::simulation::Simulator(config).run();
        v2x::output::printSummary(result);
        if (!config.csvPath.empty()) v2x::output::writeCsv(config.csvPath, result);
        if (!config.jsonPath.empty()) v2x::output::writeJson(config.jsonPath, result);
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\nUse --help for options.\n";
        return 1;
    }
}
