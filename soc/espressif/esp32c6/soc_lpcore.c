/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "soc/soc_caps.h"
#include "esp_rom_caps.h"
#include "rom/ets_sys.h"
#include "ulp_lp_core_utils.h"
#include "ulp_lp_core_lp_timer_shared.h"
#include "ulp_lp_core_memory_shared.h"
#include "ulp_lp_core_print.h"
#include <soc.h>
#include <kernel_internal.h>

extern void main(void);

/* Initialize lp core related system functions before calling user's main*/
void lp_core_startup(void)
{
#if CONFIG_ULP_HP_UART_CONSOLE_PRINT && ESP_ROM_HAS_LP_ROM
	ets_install_putc1(lp_core_print_char);
#endif

	ulp_lp_core_update_wakeup_cause();

	/* Start Zephyr */
	z_cstart();

	CODE_UNREACHABLE;
}
