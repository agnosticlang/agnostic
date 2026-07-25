// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "misc/suggest.hpp"

#include <algorithm>

namespace agn::misc {

size_t editDistance(const std::string& a, const std::string& b) {
    std::vector<std::vector<size_t>> dp(a.size() + 1, std::vector<size_t>(b.size() + 1));
    for (size_t i = 0; i <= a.size(); i++) dp[i][0] = i;
    for (size_t j = 0; j <= b.size(); j++) dp[0][j] = j;

    for (size_t i = 1; i <= a.size(); i++) {
        for (size_t j = 1; j <= b.size(); j++) {
            if (a[i - 1] == b[j - 1]) {
                dp[i][j] = dp[i - 1][j - 1];
            } else {
                dp[i][j] = 1 + std::min({dp[i - 1][j], dp[i][j - 1], dp[i - 1][j - 1]});
            }
        }
    }
    return dp[a.size()][b.size()];
}

std::optional<std::string> suggestClosest(const std::string& name, const std::vector<std::string>& candidates) {
    size_t threshold = name.size() <= 4 ? 1 : (name.size() <= 8 ? 2 : name.size() / 3);

    std::optional<std::string> best;
    size_t bestDistance = threshold + 1;
    for (auto& candidate : candidates) {
        if (candidate == name) continue;
        size_t d = editDistance(name, candidate);
        if (d <= threshold && d < bestDistance) {
            bestDistance = d;
            best = candidate;
        }
    }
    return best;
}

} // namespace agn::misc
