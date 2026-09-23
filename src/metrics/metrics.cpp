#include "metrics/metrics.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace v2x::metrics {

SampleStatistics summarize(std::vector<double> values) {
    if (values.empty()) return {};
    std::sort(values.begin(), values.end());
    const double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    double squared = 0.0;
    for (const double value : values) squared += (value - mean) * (value - mean);
    const double standardDeviation = values.size() < 2 ? 0.0 : std::sqrt(squared / (values.size() - 1));
    const double median = values.size() % 2 == 0
        ? (values[values.size() / 2 - 1] + values[values.size() / 2]) / 2.0
        : values[values.size() / 2];
    return {values.size(), mean, median, values.front(), values.back(), standardDeviation,
            1.96 * standardDeviation / std::sqrt(static_cast<double>(values.size()))};
}

}  // namespace v2x::metrics
