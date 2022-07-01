/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/zephyr.h>
#include <zephyr/pm/pm.h>

#include <ace_v1x-regs.h>

#define SRAM_ALIAS_BASE         0xA0000000
#define SRAM_ALIAS_MASK         0xF0000000

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
extern void ace_power_down(bool disable_lpsram, uint32_t *hpsram_pg_mask,
			   bool response_to_ipc);


__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();

	switch (state) {
	case PM_STATE_SOFT_OFF:/* D3 */
		DFDSPBRCP.bootctl[cpu].bctl &= ~DFDSPBRCP_BCTL_WAITIPCG;
		soc_cpus_active[cpu] = false;
		z_xtensa_cache_flush_inv_all();
		if (cpu == 0) {
			/* FIXME: this value should come from MM */
			uint32_t hpsram_mask[1] = { 0x3FFFFF };

			ace_power_down(true, uncache_to_cache(&hpsram_mask[0]),
				       true);
		} else {
			k_cpu_idle();
		}

		break;
	case PM_STATE_SUSPEND_TO_IDLE: /* D0ix */
		__fallthrough;
	case PM_STATE_RUNTIME_IDLE:/* D0 */
		k_cpu_idle();
		break;
	default:
		__ASSERT(false, "invalid argument - unsupported power state");
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();

	switch (state) {
	case PM_STATE_SOFT_OFF:/* D3 */
		/* TODO: move clock gating prevent to imr restore vector when it will be ready. */
		DFDSPBRCP.bootctl[cpu].bctl |= DFDSPBRCP_BCTL_WAITIPCG;
		soc_cpus_active[cpu] = true;
		z_xtensa_cache_flush_inv_all();
		__fallthrough;
	case PM_STATE_SUSPEND_TO_IDLE: /* D0ix */
		__fallthrough;
	case PM_STATE_RUNTIME_IDLE:/* D0 */
		break;
	default:
		__ASSERT(false, "invalid argument - unsupported power state");
		break;
	}
}
