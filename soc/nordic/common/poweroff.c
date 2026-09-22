/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/retained_mem/nrf_retained_mem.h>

#if defined(CONFIG_SOC_SERIES_NRF51) || defined(CONFIG_SOC_SERIES_NRF52)
#include <hal/nrf_power.h>
#elif defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF92)
#include <soc_power.h>
#else
#include <hal/nrf_regulators.h>
#endif
#if defined(CONFIG_SOC_SERIES_NRF54L)
#include <helpers/nrfx_reset_reason.h>
#include <hal/nrf_memconf.h>
#endif

#if defined(CONFIG_HAS_NORDIC_RAM_CTRL)
#include <helpers/nrfx_ram_ctrl.h>
#endif

#if defined(CONFIG_SOC_SERIES_NRF54L)
#define VPR_POWER_IDX 1
#define VPR_RET_BIT   MEMCONF_POWER_RET_MEM0_Pos
#endif

void z_sys_poweroff(void)
{
#if defined(CONFIG_HAS_NORDIC_RAM_CTRL)
	/* nRF71 disables RAM retention during secure early initialization. */
#if !defined(CONFIG_SOC_SERIES_NRF71)
	/* Disable retention for all memory blocks */
	nrfx_ram_ctrl_retention_enable_all_set(false);
#endif /* !defined(CONFIG_SOC_SERIES_NRF71) */

#endif /* defined(CONFIG_HAS_NORDIC_RAM_CTRL) */

#if defined(CONFIG_RETAINED_MEM_NRF_RAM_CTRL)
	/* Retention is best-effort because z_sys_poweroff() cannot return an
	 * error. Continue powering off if applying the devicetree configuration
	 * fails, in which case the configured retained data may not survive.
	 */
	(void)z_nrf_retained_mem_retention_apply();
#endif

#if defined(CONFIG_SOC_SERIES_NRF54L)
	/* Set VPR to remain in its reset state when waking from OFF */
	nrf_memconf_ramblock_ret_enable_set(NRF_MEMCONF, VPR_POWER_IDX, VPR_RET_BIT, false);

	nrfx_reset_reason_clear(UINT32_MAX);
#endif
#if defined(CONFIG_SOC_SERIES_NRF51) || defined(CONFIG_SOC_SERIES_NRF52)
	nrf_power_system_off(NRF_POWER);
#elif defined(CONFIG_SOC_SERIES_NRF54H) || defined(CONFIG_SOC_SERIES_NRF92)
	nrf_poweroff();
#else
	nrf_regulators_system_off(NRF_REGULATORS);
#endif

	CODE_UNREACHABLE;
}
