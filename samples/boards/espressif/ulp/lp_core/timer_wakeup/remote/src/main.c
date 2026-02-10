/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_memory_shared.h"
#include "ulp_lp_core_lp_timer_shared.h"

int main(void)
{
	ulp_lp_core_wakeup_main_processor();

	ulp_lp_core_memory_shared_cfg_t *cfg = ulp_lp_core_memory_shared_cfg_get();

	if (cfg->sleep_duration_ticks) {
		ulp_lp_core_lp_timer_set_wakeup_ticks(cfg->sleep_duration_ticks);
	}

	ulp_lp_core_halt();

	return 0;
}
