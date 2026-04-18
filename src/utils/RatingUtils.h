#pragma once

// Shared rating maths used by ChartCatalog, UserStatScene, and
// AdminStatScene.  Keeping the formula in a single place guarantees
// all three computation sites stay in sync.

namespace rating_utils {

// Converts a raw accuracy value to the [0, 1] range.
// Values already in [0, 1] are returned unchanged; values > 1 are divided by
// 100 (percent form); negative values are clamped to 0.
inline double normalizeAccuracy(float accuracy) {
    if (accuracy > 1.0f) return static_cast<double>(accuracy) / 100.0;
    if (accuracy < 0.0f) return 0.0;
    return accuracy;
}

// Evaluates a single chart play: difficulty * (a - a^2 + a^4), where a is the
// normalised accuracy.  Higher difficulty and accuracy both raise the score, but
// the polynomial shape rewards near-perfect runs disproportionately.
inline double singleChartEvaluation(float difficulty, float accuracy) {
    const double a  = normalizeAccuracy(accuracy);
    const double a2 = a * a;
    const double a4 = a2 * a2;
    return static_cast<double>(difficulty) * (a - a2 + a4);
}

} // namespace rating_utils
