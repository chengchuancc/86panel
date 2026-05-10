#pragma once

#include "NetStats.h"

namespace ControlServer {
void begin();
void handle(const Stats &stats);
}
