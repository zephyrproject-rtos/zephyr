/*
 * Copyright (c) 2025 Ambiq Micro Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/cache.h>
#include <zephyr/logging/log.h>
#include <zephyr/mem_mgmt/mem_attr.h>
#ifdef CONFIG_CACHE_MANAGEMENT
#include <zephyr/dt-bindings/memory-attr/memory-attr-arm.h>
#endif /* CONFIG_CACHE_MANAGEMENT */

#ifdef CONFIG_NOCACHE_MEMORY
#include <zephyr/linker/linker-defs.h>
#endif /* CONFIG_NOCACHE_MEMORY */

#include <soc.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define SCRATCH0_OEM_RCV_RETRY_MAGIC 0xA86

void soc_early_init_hook(void)
{
	/* Enable Loop and branch info cache */
	SCB->CCR |= SCB_CCR_LOB_Msk;
	__DSB();
	__ISB();

	if ((MCUCTRL->SCRATCH0 >> 20) == SCRATCH0_OEM_RCV_RETRY_MAGIC) {
		/*
		 * Clear the scratch register
		 */
		MCUCTRL->SCRATCH0 = 0x00;
	}

	/* Internal timer15 for SPOT manager */
	IRQ_CONNECT(82, 0, hal_internal_timer_isr, 0, 0);

	/* Initialize for low power in the power control block */
	am_hal_pwrctrl_low_power_init();

	/* Enable SIMOBUCK for the apollo5 Family */
	am_hal_pwrctrl_control(AM_HAL_PWRCTRL_CONTROL_SIMOBUCK_INIT, NULL);

	/*
	 * Set default temperature for spotmgr to room temperature
	 */
	am_hal_pwrctrl_temp_thresh_t dummy;

	am_hal_pwrctrl_temp_update(25.0f, &dummy);

	/* Enable Icache*/
	sys_cache_instr_enable();

	/* Enable Dcache */
	sys_cache_data_enable();

#ifdef CONFIG_CORTEX_M_DWT
	am_hal_pwrctrl_periph_enable(AM_HAL_PWRCTRL_PERIPH_DEBUG);
#endif
}

#if CONFIG_CACHE_MANAGEMENT
bool buf_in_nocache(uintptr_t buf, size_t len_bytes)
{
	bool buf_within_nocache = false;

	if (buf == 0 || len_bytes == 0) {
		return buf_within_nocache;
	}

#if CONFIG_NOCACHE_MEMORY
	/* Check if buffer is in nocache region defined by the linker */
	buf_within_nocache = (buf >= ((uintptr_t)_nocache_ram_start)) &&
			     ((buf + len_bytes - 1) <= ((uintptr_t)_nocache_ram_end));
	if (buf_within_nocache) {
		return true;
	}
#endif /* CONFIG_NOCACHE_MEMORY */

	/* Check if buffer is in nocache memory region defined in DT */
	buf_within_nocache = mem_attr_check_buf((void *)buf, len_bytes,
						DT_MEM_ARM(ATTR_MPU_RAM_NOCACHE)) == 0;

	return buf_within_nocache;
}
#endif
