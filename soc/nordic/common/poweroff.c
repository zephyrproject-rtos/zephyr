/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/drivers/retained_mem/nrf_retained_mem.h>

#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
#include <hal/nrf_power.h>
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
#include <power.h>
#else
#include <hal/nrf_regulators.h>
#endif
#if defined(CONFIG_SOC_SERIES_NRF54LX)
#include <helpers/nrfx_reset_reason.h>
#endif

#if defined(CONFIG_HAS_NORDIC_RAM_CTRL)
#include <helpers/nrfx_ram_ctrl.h>
#endif

void z_sys_poweroff(void)
{
#if defined(CONFIG_HAS_NORDIC_RAM_CTRL)
	uint8_t *ram_start;
	size_t ram_size;

#if defined(NRF_MEMORY_RAM_BASE)
	ram_start = (uint8_t *)NRF_MEMORY_RAM_BASE;
#else
	ram_start = (uint8_t *)NRF_MEMORY_RAM0_BASE;
#endif

	ram_size = 0;
#if defined(NRF_MEMORY_RAM_SIZE)
	ram_size += NRF_MEMORY_RAM_SIZE;
#endif
#if defined(NRF_MEMORY_RAM0_SIZE)
	ram_size += NRF_MEMORY_RAM0_SIZE;
#endif
#if defined(NRF_MEMORY_RAM1_SIZE)
	ram_size += NRF_MEMORY_RAM1_SIZE;
#endif
#if defined(NRF_MEMORY_RAM2_SIZE)
	ram_size += NRF_MEMORY_RAM2_SIZE;
#endif

	/* Disable retention for all memory blocks */
	nrfx_ram_ctrl_retention_enable_set(ram_start, ram_size, false);
#endif

#if defined(CONFIG_RETAINED_MEM_NRF_RAM_CTRL)
	/* Restore retention for retained_mem driver regions defined in devicetree */
	(void)z_nrf_retained_mem_retention_apply();
#endif

#if defined(CONFIG_SOC_SERIES_NRF54LX)
	nrfx_reset_reason_clear(UINT32_MAX);
#endif
#if defined(CONFIG_SOC_SERIES_NRF51X) || defined(CONFIG_SOC_SERIES_NRF52X)
	nrf_power_system_off(NRF_POWER);
#elif defined(CONFIG_SOC_SERIES_NRF54HX)
	nrf_poweroff();
#else
	nrf_regulators_system_off(NRF_REGULATORS);
#endif

	CODE_UNREACHABLE;
}
