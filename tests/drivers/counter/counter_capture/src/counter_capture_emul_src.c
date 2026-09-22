/*
 * Copyright (c) 2026 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Trigger-source backend for native_sim: the capture input is driven directly
 * through the emulated counter backend API, no physical wiring required.
 */

#include <zephyr/drivers/counter/counter_capture_emul.h>

#include "trigger_src.h"

int trigger_src_setup(size_t idx)
{
	return counter_capture_emul_input_set_dt(&capture_dt_specs[idx], 0);
}

int trigger_src_get(size_t idx)
{
	return counter_capture_emul_input_get_dt(&capture_dt_specs[idx]);
}

int trigger_src_set(size_t idx, int level)
{
	return counter_capture_emul_input_set_dt(&capture_dt_specs[idx], level);
}
