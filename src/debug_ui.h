#pragma once
#include "chrono.h"
#include "result.h"

result_t init_debug_ui(void);
result_t update_debug_ui(microseconds_t frame_time, float delta);