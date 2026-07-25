// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace agn::misc {

size_t editDistance(const std::string& a, const std::string& b);

std::optional<std::string> suggestClosest(const std::string& name, const std::vector<std::string>& candidates);

} // namespace agn::misc
