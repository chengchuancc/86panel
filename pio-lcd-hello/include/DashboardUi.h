#pragma once

#include "NetStats.h"

namespace DashboardUi {
void build();
void updateClock();
void updateNetworkIdentity();
void setTargetStats(const Stats &target);
void interpolateUpdate(float t);
void updateConnectionView(const Stats &stats, uint32_t reconnect_count);
}
