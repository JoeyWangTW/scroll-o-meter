#pragma once

#include <stdint.h>

/* Total number of encoder detents observed since last reset.
   Magnitude only — direction is intentionally not tracked because
   the user-facing concept is "how much did I scroll", not net travel. */
uint32_t scroll_counter_get(void);

/* Increment by N detents (called from the sensor event listener).
   Schedules a debounced settings write per CONFIG_ZMK_SCROLL_COUNTER_PERSIST_EVERY. */
void scroll_counter_add(uint32_t delta);

/* Zero the counter and immediately persist. */
void scroll_counter_reset(void);
