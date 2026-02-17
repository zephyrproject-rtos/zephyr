/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include "soc/soc_caps.h"
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_lp_timer_shared.h"
#include "ulp_lp_core_memory_shared.h"
#include <soc.h>

extern FUNC_NORETURN void z_cstart(void);

/* Initialize lp core related system functions before calling user's main */
void lp_core_startup(void)
{
	ulp_lp_core_update_wakeup_cause();

	/* Start Zephyr kernel - runs device init (mbox, uart, etc.) then main() */
	z_cstart();

	CODE_UNREACHABLE;
}
