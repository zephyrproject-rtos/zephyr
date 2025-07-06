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
#include <hal/nrf_memconf.h>
#include <zephyr/cache.h>
#include <power.h>
#include <soc_lrcconf.h>
#include "soc.h"
#include "pm_s2ram.h"

extern sys_snode_t soc_node;

static void nrf_power_down_cache(void)
{
	static const uint32_t msk =
		(IS_ENABLED(CONFIG_DCACHE) ? BIT(RAMBLOCK_CONTROL_BIT_DCACHE) : 0) |
		(IS_ENABLED(CONFIG_ICACHE) ? BIT(RAMBLOCK_CONTROL_BIT_ICACHE) : 0);

	if (msk == 0) {
		return;
	}

	/* Functions are non-empty only if cache is enabled.
	 * Data cache disabling include flushing.
	 */
	sys_cache_data_disable();
	sys_cache_instr_disable();
	nrf_memconf_ramblock_control_mask_enable_set(NRF_MEMCONF, RAMBLOCK_POWER_ID, msk, false);
}

void nrf_power_up_cache(void)
{
	static const uint32_t msk =
		(IS_ENABLED(CONFIG_DCACHE) ? BIT(RAMBLOCK_CONTROL_BIT_DCACHE) : 0) |
		(IS_ENABLED(CONFIG_ICACHE) ? BIT(RAMBLOCK_CONTROL_BIT_ICACHE) : 0);

	if (msk == 0) {
		return;
	}

	nrf_memconf_ramblock_control_mask_enable_set(NRF_MEMCONF, RAMBLOCK_POWER_ID, msk, true);
	sys_cache_instr_enable();
	sys_cache_data_enable();
}

static void common_suspend(void)
{
	soc_lrcconf_poweron_release(&soc_node, NRF_LRCCONF_POWER_DOMAIN_0);
	nrf_power_down_cache();
}

static void common_resume(void)
{
	/* Common part does not include cache enabling. In case of s2ram it is done
	 * as early as possible to speed up the process.
	 */
	soc_lrcconf_poweron_request(&soc_node, NRF_LRCCONF_POWER_DOMAIN_0);
}

void nrf_poweroff(void)
{
	nrf_resetinfo_resetreas_local_set(NRF_RESETINFO, 0);
	nrf_resetinfo_restore_valid_set(NRF_RESETINFO, false);

#if !defined(CONFIG_SOC_NRF54H20_CPURAD)
	/* Disable retention */
	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN, false);
	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, false);
#endif
	common_suspend();

	nrf_lrcconf_task_trigger(NRF_LRCCONF010, NRF_LRCCONF_TASK_SYSTEMOFFREADY);

	__set_BASEPRI(0);
	__ISB();
	__DSB();
	__WFI();

	CODE_UNREACHABLE;
}

static void s2idle_enter(uint8_t substate_id)
{
	switch (substate_id) {
	case 0:
		/* Substate for idle with cache powered on - not implemented yet. */
		break;
	case 1: /* Substate for idle with cache retained - not implemented yet. */
		break;
	case 2: /* Substate for idle with cache disabled. */
#if !defined(CONFIG_SOC_NRF54H20_CPURAD)
		soc_lrcconf_poweron_request(&soc_node, NRF_LRCCONF_POWER_MAIN);
#endif
		common_suspend();
		break;
	default: /* Unknown substate. */
		return;
	}

	__set_BASEPRI(0);
	__ISB();
	__DSB();
	__WFI();
}

static void s2idle_exit(uint8_t substate_id)
{
	switch (substate_id) {
	case 0:
		/* Substate for idle with cache powered on - not implemented yet. */
		break;
	case 1: /* Substate for idle with cache retained - not implemented yet. */
		break;
	case 2: /* Substate for idle with cache disabled. */
		nrf_power_up_cache();
		common_resume();
#if !defined(CONFIG_SOC_NRF54H20_CPURAD)
		soc_lrcconf_poweron_release(&soc_node, NRF_LRCCONF_POWER_MAIN);
#endif
	default: /* Unknown substate. */
		return;
	}
}

#if defined(CONFIG_PM_S2RAM)
/* Resume domain after local suspend to RAM. */
static void s2ram_exit(void)
{
	common_resume();
#if !defined(CONFIG_SOC_NRF54H20_CPURAD)
	/* Re-enable domain retention. */
	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, true);
#endif
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

#if !defined(CONFIG_SOC_NRF54H20_CPURAD)
	/* Disable retention */
	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, false);
#endif
	common_suspend();

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

static void s2ram_enter(void)
{
	/*
	 * Save the CPU context (including the return address),set the SRAM
	 * marker and power off the system.
	 */
	if (soc_s2ram_suspend(sys_suspend_to_ram)) {
		return;
	}
}
#endif /* defined(CONFIG_PM_S2RAM) */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	if (state == PM_STATE_SUSPEND_TO_IDLE) {
		__disable_irq();
		sys_trace_idle();
		s2idle_enter(substate_id);
		/* Resume here. */
		s2idle_exit(substate_id);
		sys_trace_idle_exit();
		__enable_irq();
	}
#if defined(CONFIG_PM_S2RAM)
	else if (state == PM_STATE_SUSPEND_TO_RAM) {
		__disable_irq();
		sys_trace_idle();
		s2ram_enter();
		/* On resuming or error we return exactly *HERE* */
		s2ram_exit();
		sys_trace_idle_exit();
		__enable_irq();
	}
#endif
	else {
		k_cpu_idle();
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	irq_unlock(0);
}
