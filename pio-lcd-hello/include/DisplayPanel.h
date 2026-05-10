#pragma once

namespace DisplayPanel {
bool begin();
void reinitialize();
void setBacklight(bool enabled);
bool backlightEnabled();
}
