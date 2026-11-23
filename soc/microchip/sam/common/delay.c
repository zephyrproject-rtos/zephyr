/*
 * Copyright (C) 2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include "delay.h"

void busy_delay(uint32_t cycle, uint32_t n)
{
	while (n != 0U) {
		volatile uint32_t i = cycle;

		while (i != 0U) {
			i--;
		}

		n--;
	}
}
