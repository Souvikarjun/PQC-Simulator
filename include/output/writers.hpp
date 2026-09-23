#pragma once

#include "simulation/simulator.hpp"

#include <string>

namespace v2x::output {

void writeCsv(const std::string& path, const simulation::RunResult& result);
void writeJson(const std::string& path, const simulation::RunResult& result);
void printSummary(const simulation::RunResult& result);

}  // namespace v2x::output
