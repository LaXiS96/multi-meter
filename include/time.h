#pragma once

#include <stdint.h>

/** Milliseconds counter */
static volatile uint32_t time_ms = 0;

/** Do not use official SDK's `Delay_Init` and related functions at the same time as this */
void time_init(void);

void time_delay_ms(uint32_t ms);
