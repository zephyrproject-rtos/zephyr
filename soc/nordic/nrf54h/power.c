/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/toolchain.h>
#include <zephyr/pm/policy.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <hal/nrf_resetinfo.h>
#include <hal/nrf_lrcconf.h>
#include <hal/nrf_memconf.h>
#include <zephyr/cache.h>
#include <power.h>
#include "pm_s2ram.h"

static void suspend_common(void)
{

	/* Flush, disable and power down DCACHE */
	sys_cache_data_flush_all();
	sys_cache_data_disable();
	nrf_memconf_ramblock_control_enable_set(NRF_MEMCONF, RAMBLOCK_POWER_ID,
						RAMBLOCK_CONTROL_BIT_DCACHE, false);

	if (IS_ENABLED(CONFIG_ICACHE)) {
		/* Disable and power down ICACHE */
		sys_cache_instr_disable();
		nrf_memconf_ramblock_control_enable_set(NRF_MEMCONF, RAMBLOCK_POWER_ID,
							RAMBLOCK_CONTROL_BIT_ICACHE, false);
	}

	/* Disable retention */
	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, false);
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, false);
}

void nrf_poweroff(void)
{
	nrf_resetinfo_resetreas_local_set(NRF_RESETINFO, 0);
	nrf_resetinfo_restore_valid_set(NRF_RESETINFO, false);

	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN, false);

	/* TODO: Move it around k_cpu_idle() implementation. */
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN, false);
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, false);

	suspend_common();

	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_SYSTEMOFFREADY);

	__set_BASEPRI(0);
	__ISB();
	__DSB();
	__WFI();

	CODE_UNREACHABLE;
}

#if IS_ENABLED(CONFIG_PM_S2RAM)
/* Resume domain after local suspend to RAM. */
static void sys_resume(void)
{
	if (IS_ENABLED(CONFIG_ICACHE)) {
		/* Power up and re-enable ICACHE */
		nrf_memconf_ramblock_control_enable_set(NRF_MEMCONF, RAMBLOCK_POWER_ID,
							RAMBLOCK_CONTROL_BIT_ICACHE, true);
		sys_cache_instr_enable();
	}

	if (IS_ENABLED(CONFIG_DCACHE)) {
		/* Power up and re-enable DCACHE */
		nrf_memconf_ramblock_control_enable_set(NRF_MEMCONF, RAMBLOCK_POWER_ID,
							RAMBLOCK_CONTROL_BIT_DCACHE, true);
		sys_cache_data_enable();
	}

	/* Re-enable domain retention. */
	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, true);

	/* TODO: Move it around k_cpu_idle() implementation. */
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN,
				      !IS_ENABLED(CONFIG_SOC_NRF54H20_CPURAD));
}

/* Function called during local domain suspend to RAM. */
static int sys_suspend_to_ram(void)
{
	/* Set intormation which is used on domain wakeup to determine if resume from RAM shall
	 * be performed.
	 */
	nrf_resetinfo_resetreas_local_set(NRF_RESETINFO,
					  NRF_RESETINFO_RESETREAS_LOCAL_UNRETAINED_MASK);
	nrf_resetinfo_restore_valid_set(NRF_RESETINFO, true);
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, false);
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN, false);

	suspend_common();

	__set_BASEPRI(0);
	__ISB();
	__DSB();
	__WFI();
	/*
	 * We might reach this point is k_cpu_idle returns (there is a pre sleep hook that
	 * can abort sleeping.
	 */
	return -EBUSY;
}

static void do_suspend_to_ram(void)
{
	/*
	 * Save the CPU context (including the return address),set the SRAM
	 * marker and power off the system.
	 */
	if (soc_s2ram_suspend(sys_suspend_to_ram)) {
		return;
	}

	/*
	 * On resuming or error we return exactly *HERE*
	 */

	sys_resume();
}
#endif /* IS_ENABLED(CONFIG_PM_S2RAM) */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	if (state != PM_STATE_SUSPEND_TO_RAM) {
		k_cpu_idle();
		return;
	}
#if IS_ENABLED(CONFIG_PM_S2RAM)
	do_suspend_to_ram();
#endif
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	irq_unlock(0);
}
