/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <xtensa/xtruntime.h>
#include <zephyr/irq_nextlevel.h>
#include <xtensa/hal.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/device.h>
#include <zephyr/cache.h>
#include <cpu_init.h>

#include <adsp_shim.h>
#include <adsp_clk.h>
#include <cavs-idc.h>
#include "soc.h"

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#include <zephyr/sw_isr_table.h>
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc);

# define SHIM_GPDMA_BASE_OFFSET   0x6500
# define SHIM_GPDMA_BASE(x)       (SHIM_GPDMA_BASE_OFFSET + (x) * 0x100)
# define SHIM_GPDMA_CLKCTL(x)     (SHIM_GPDMA_BASE(x) + 0x4)
# define SHIM_CLKCTL_LPGPDMAFDCGB BIT(0)

#ifdef CONFIG_PM_POLICY_CUSTOM
#define SRAM_ALIAS_BASE		0x9E000000
#define SRAM_ALIAS_MASK		0xFF000000
#define EBB_BANKS_IN_SEGMENT	32
#define SRAM_ALIAS_OFFSET	0x20000000

#define L2_INTERRUPT_NUMBER     4
#define L2_INTERRUPT_MASK       (1<<L2_INTERRUPT_NUMBER)

#define L3_INTERRUPT_NUMBER     6
#define L3_INTERRUPT_MASK       (1<<L3_INTERRUPT_NUMBER)

#define ALL_USED_INT_LEVELS_MASK (L2_INTERRUPT_MASK | L3_INTERRUPT_MASK)

struct core_state {
	uint32_t intenable;
};

static struct core_state core_desc[CONFIG_MP_MAX_NUM_CPUS] = {{0}};

/**
 * @brief Power down procedure.
 *
 * Locks its code in L1 cache and shuts down memories.
 * NOTE: there's no return from this function.
 *
 * @param disable_lpsram        flag if LPSRAM is to be disabled (whole)
 * @param hpsram_pg_mask pointer to memory segments power gating mask
 * (each bit corresponds to one ebb)
 */
extern void power_down_cavs(bool disable_lpsram, uint32_t *hpsram_pg_mask);

static inline void __sparse_cache *uncache_to_cache(void *address)
{
	return (void __sparse_cache *)((uintptr_t)(address) | SRAM_ALIAS_OFFSET);
}

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();

	if (state == PM_STATE_SOFT_OFF) {
		core_desc[cpu].intenable = XTENSA_RSR("INTENABLE");
		z_xt_ints_off(0xffffffff);
		soc_cpus_active[cpu] = false;
		sys_cache_data_flush_and_invd_all();
		if (cpu == 0) {
			uint32_t ebb = EBB_BANKS_IN_SEGMENT;
			/* turn off all HPSRAM banks - get a full bitmap */
			uint32_t hpsram_mask = (1 << ebb) - 1;
			/* do power down - this function won't return */
			power_down_cavs(true, uncache_to_cache(&hpsram_mask));
		} else {
			z_xt_ints_on(core_desc[cpu].intenable);
			k_cpu_idle();
		}
	} else {
		__ASSERT(false, "invalid argument - unsupported power state");
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();

	if (state == PM_STATE_SOFT_OFF) {
		soc_cpus_active[cpu] = true;
		sys_cache_data_flush_and_invd_all();
		z_xt_ints_on(core_desc[cpu].intenable);
	} else {
		__ASSERT(false, "invalid argument - unsupported power state");
	}
}
#endif /* CONFIG_PM_POLICY_CUSTOM */

__imr void power_init(void)
{
	/* Request HP ring oscillator and
	 * wait for status to indicate it's ready.
	 */
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_RHROSCC;
	while ((CAVS_SHIM.clkctl & CAVS_CLKCTL_RHROSCC) != CAVS_CLKCTL_RHROSCC) {
		k_busy_wait(10);
	}

	/* Request HP Ring Oscillator
	 * Select HP Ring Oscillator
	 * High Power Domain PLL Clock Select device by 2
	 * Low Power Domain PLL Clock Select device by 4
	 * Disable Tensilica Core(s) Prevent Local Clock Gating
	 *   - Disabling "prevent clock gating" means allowing clock gating
	 */
	CAVS_SHIM.clkctl = (CAVS_CLKCTL_RHROSCC |
			    CAVS_CLKCTL_OCS |
			    CAVS_CLKCTL_LMCS);

	/* Prevent LP GPDMA 0 & 1 clock gating */
	sys_write32(SHIM_CLKCTL_LPGPDMAFDCGB, SHIM_GPDMA_CLKCTL(0));
	sys_write32(SHIM_CLKCTL_LPGPDMAFDCGB, SHIM_GPDMA_CLKCTL(1));

	/* Disable power gating for first cores */
	CAVS_SHIM.pwrctl |= CAVS_PWRCTL_TCPDSPPG(0);

	/* On cAVS 1.8+, we must demand ownership of the timestamping
	 * and clock generator registers.  Lacking the former will
	 * prevent wall clock timer interrupts from arriving, even
	 * though the device itself is operational.
	 */
	sys_write32(GENO_MDIVOSEL | GENO_DIOPTOSEL, DSP_INIT_GENO);
	sys_write32(IOPO_DMIC_FLAG | IOPO_I2SSEL_MASK, DSP_INIT_IOPO);
}
