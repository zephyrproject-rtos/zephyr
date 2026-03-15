/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include "soc/soc_caps.h"
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_interrupts.h"
#include "ulp_lp_core_lp_timer_shared.h"
#include "ulp_lp_core_memory_shared.h"
#include <soc.h>

extern FUNC_NORETURN void z_cstart(void);

void lp_core_startup(void)
{
	ulp_lp_core_update_wakeup_cause();

	ulp_lp_core_intr_enable();

	z_cstart();

	CODE_UNREACHABLE;
}
