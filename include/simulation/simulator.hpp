#pragma once

#include "metrics/metrics.hpp"
#include "simulation/configuration.hpp"

namespace v2x::simulation {

struct RunResult {
    Configuration configuration;
    metrics::Metrics metrics;
    std::uint64_t ticks{};
};

class Simulator {
public:
    explicit Simulator(Configuration configuration);
    RunResult run() const;

private:
    Configuration configuration_;
};

}  // namespace v2x::simulation
