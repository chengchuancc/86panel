#pragma once

#include "NetStats.h"

namespace DashboardUi {
void build();
void updateClock();
void updateNetworkIdentity();
void updateStats(const Stats &stats);
void updateConnectionView(const Stats &stats, uint32_t reconnect_count);
}
