/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include "hal/cntr.h"

void hal_ticker_instance0_trigger_set(uint32_t value)
{
	cntr_cmp_set(0U, value);
}
