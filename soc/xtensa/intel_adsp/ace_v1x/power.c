/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <zephyr/pm/pm.h>

#include <ace_v1x-regs.h>

#define LPSCTL_BATTR_MASK       GENMASK(16, 12)
#define SRAM_ALIAS_BASE         0xA0000000
#define SRAM_ALIAS_MASK         0xF0000000

__imr void power_init(void)
{
	/* Disable idle power gating */
	DFDSPBRCP.bootctl[0].bctl |= DFDSPBRCP_BCTL_WAITIPCG | DFDSPBRCP_BCTL_WAITIPPG;
}

#ifdef CONFIG_PM

#define uncache_to_cache(address) \
				((__typeof__(address))(((uint32_t)(address) &  \
				~SRAM_ALIAS_MASK) | SRAM_ALIAS_BASE))

/**
 * @brief Power down procedure.
 *
 * Locks its code in L1 cache and shuts down memories.
 * NOTE: there's no return from this function.
 *
 * @param disable_lpsram        flag if LPSRAM is to be disabled (whole)
 * @param hpsram_pg_mask pointer to memory segments power gating mask
 * (each bit corresponds to one ebb)
 * @param response_to_ipc       flag if ipc response should be send during power down
 */
extern void power_down(bool disable_lpsram, uint32_t *hpsram_pg_mask,
			   bool response_to_ipc);

/* NOTE: This struct will grow with all values that have to be stored for
 * proper cpu restore after PG.
 */
struct core_state {
	uint32_t vecbase;
	uint32_t excsave2;
	uint32_t excsave3;
	uint32_t thread_ptr;
	uint32_t intenable;
};

static struct core_state core_desc[CONFIG_MP_NUM_CPUS] = { 0 };

static ALWAYS_INLINE void _save_core_context(uint32_t core_id)
{
	core_desc[core_id].vecbase = RSR("VECBASE");
	core_desc[core_id].excsave2 = RSR("EXCSAVE2");
	core_desc[core_id].excsave3 = RSR("EXCSAVE3");
	core_desc[core_id].thread_ptr = RUR("THREADPTR");
}

void power_gate_entry(uint32_t core_id)
{
	_save_core_context(core_id);
	soc_cpus_active[core_id] = false;
	z_xtensa_cache_flush_inv_all();
	k_cpu_idle();
}

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();

	if (state == PM_STATE_SOFT_OFF) {
		DFDSPBRCP.bootctl[cpu].wdtcs = DFDSPBRCP_WDT_RESTART_COMMAND;
		DFDSPBRCP.bootctl[cpu].bctl &= ~DFDSPBRCP_BCTL_WAITIPCG;
		soc_cpus_active[cpu] = false;
		z_xtensa_cache_flush_inv_all();
		if (cpu == 0) {
			/* FIXME: this value should come from MM */
			uint32_t hpsram_mask[1] = { 0x3FFFFF };

			power_down(true, uncache_to_cache(&hpsram_mask[0]),
				   true);
		} else {
			k_cpu_idle();
		}
	} else if (state == PM_STATE_RUNTIME_IDLE) {
		core_desc[cpu].intenable = RSR("INTENABLE");
		z_xt_ints_off(0xffffffff);
		DFDSPBRCP.bootctl[cpu].bctl &= ~DFDSPBRCP_BCTL_WAITIPPG;
		DFDSPBRCP.bootctl[cpu].bctl &= ~DFDSPBRCP_BCTL_WAITIPCG;
		ACE_PWRCTL->wpdsphpxpg &= ~BIT(cpu);
		if (cpu == 0) {
			uint32_t battr = DFDSPBRCP.bootctl[cpu].battr & (~LPSCTL_BATTR_MASK);

			battr |= (DFDSPBRCP_BATTR_LPSCTL_RESTORE_BOOT & LPSCTL_BATTR_MASK);
			DFDSPBRCP.bootctl[cpu].battr = battr;
		}

		power_gate_entry(cpu);
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
		DFDSPBRCP.bootctl[cpu].wdtcs = DFDSPBRCP_WDT_RESUME;
		/* TODO: move clock gating prevent to imr restore vector when it will be ready. */
		DFDSPBRCP.bootctl[cpu].bctl |= DFDSPBRCP_BCTL_WAITIPCG;
		soc_cpus_active[cpu] = true;
		z_xtensa_cache_flush_inv_all();
	} else if (state == PM_STATE_RUNTIME_IDLE) {
		if (cpu != 0) {
			/* NOTE: HW should support dynamic power gating on secondary cores.
			 * But since there is no real profit from it, functionality is not
			 * fully implemented.
			 * SOF PM policy will not allowed primary core to enter d0i3 state
			 * when secondary cores are active.
			 */
			__ASSERT(false, "state not supported on secondary core");
			return;
		}

		ACE_PWRCTL->wpdsphpxpg |= BIT(cpu);

		while ((ACE_PWRSTS->dsphpxpgs & BIT(cpu)) == 0) {
			k_busy_wait(HW_STATE_CHECK_DELAY);
		}

		DFDSPBRCP.bootctl[cpu].bctl |=
			DFDSPBRCP_BCTL_WAITIPCG | DFDSPBRCP_BCTL_WAITIPPG;
		if (cpu == 0) {
			DFDSPBRCP.bootctl[cpu].battr &= (~LPSCTL_BATTR_MASK);
		}

		soc_cpus_active[cpu] = true;
		z_xtensa_cache_flush_inv_all();
		z_xt_ints_on(core_desc[cpu].intenable);
	} else {
		__ASSERT(false, "invalid argument - unsupported power state");
	}
}

#endif
