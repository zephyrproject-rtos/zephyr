/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/pm/pm.h>
#include <zephyr/drivers/firmware/scmi/nxp/cpu.h>
#include <zephyr/drivers/firmware/scmi/power.h>
#include <soc.h>

void soc_early_init_hook(void)
{
#ifdef CONFIG_CACHE_MANAGEMENT
	sys_cache_data_enable();
	sys_cache_instr_enable();
#endif
}

#ifdef CONFIG_PM
void pm_state_before(void)
{
	struct scmi_nxp_cpu_pd_lpm_config cpu_pd_lpm_cfg;
	struct scmi_nxp_cpu_irq_mask_config cpu_irq_mask_cfg;

	/*
	 * 1. Set M7 mix as power on state in suspend mode
	 * 2. Keep wakeupmix power on whatever low power mode, as lpuart3(console) in there.
	 * To do: in order to reduce power consumption, the M7 core in the i.MX952
	 * should be powered down during suspend.
	 * However, after being woken up by a wakeup source, the M7 CPU will restart
	 * execution from the vector table address, which is not the desired behavior.
	 * Instead, the vector value should be set to the address where the CPU
	 * was before entering suspend, and the CPU state should be restored to
	 * what it was prior to suspend.
	 */

	cpu_pd_lpm_cfg.cpu_id = CPU_IDX_M7P;
	cpu_pd_lpm_cfg.num_cfg = 2;
	cpu_pd_lpm_cfg.cfgs[0].domain_id = PWR_MIX_SLICE_IDX_M7;
	cpu_pd_lpm_cfg.cfgs[0].lpm_setting = SCMI_CPU_LPM_SETTING_ON_ALWAYS;
	cpu_pd_lpm_cfg.cfgs[0].ret_mask = 1U << PWR_MEM_SLICE_IDX_M7;
	cpu_pd_lpm_cfg.cfgs[1].domain_id = PWR_MIX_SLICE_IDX_WAKEUP;
	cpu_pd_lpm_cfg.cfgs[1].lpm_setting = SCMI_CPU_LPM_SETTING_ON_ALWAYS;
	cpu_pd_lpm_cfg.cfgs[1].ret_mask = 0;

	scmi_nxp_cpu_pd_lpm_set(&cpu_pd_lpm_cfg);

	/* Set wakeup mask */
	uint32_t wake_mask[GPC_CMC_IRQ_WAKEUP_MASK_COUNT] = {
		[0 ... GPC_CMC_IRQ_WAKEUP_MASK_COUNT - 1]  = 0xFFFFFFFFU
	};

	/* IRQs enabled at NVIC level become GPC wake sources */
	for (uint32_t idx = 0; idx < 8; idx++) {
		wake_mask[idx] = ~(NVIC->ISER[idx]);
	}

	cpu_irq_mask_cfg.cpu_id = CPU_IDX_M7P;
	cpu_irq_mask_cfg.mask_idx = 0;
	cpu_irq_mask_cfg.num_mask = GPC_CMC_IRQ_WAKEUP_MASK_COUNT;

	for (uint8_t val = 0; val < GPC_CMC_IRQ_WAKEUP_MASK_COUNT; val++) {
		cpu_irq_mask_cfg.mask[val] = wake_mask[val];
	}

	scmi_nxp_cpu_set_irq_mask(&cpu_irq_mask_cfg);
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	struct scmi_nxp_cpu_sleep_mode_config cpu_cfg = {0};

	pm_state_before();

	/* iMX952 M7 core is based on ARMv7-M architecture. For this architecture,
	 * the current implementation of arch_irq_lock of zephyr is based on BASEPRI,
	 * which will only retain abnormal interrupts such as NMI,
	 * and all other interrupts from the CPU(including systemtick) will be masked,
	 * which makes the CORE unable to be woken up from WFI.
	 * Set PRIMASK as workaround, Shield the CPU from responding to interrupts,
	 * the CPU will not jump to the interrupt service routine (ISR).
	 */
	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_WAIT;
		scmi_nxp_cpu_sleep_mode_set(&cpu_cfg);
		__DSB();
		__WFI();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_STOP;
		scmi_nxp_cpu_sleep_mode_set(&cpu_cfg);
		__DSB();
		__WFI();
		break;
	case PM_STATE_STANDBY:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_SUSPEND;
		scmi_nxp_cpu_sleep_mode_set(&cpu_cfg);
		__DSB();
		__WFI();
		break;
	default:
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);

	struct scmi_nxp_cpu_irq_mask_config cpu_irq_mask_cfg;

	/* Restore scmi cpu wake mask */
	uint32_t wake_mask[GPC_CMC_IRQ_WAKEUP_MASK_COUNT] = {
		[0 ... GPC_CMC_IRQ_WAKEUP_MASK_COUNT - 1] = 0x0U
	};

	cpu_irq_mask_cfg.cpu_id = CPU_IDX_M7P;
	cpu_irq_mask_cfg.mask_idx = 0;
	cpu_irq_mask_cfg.num_mask = GPC_CMC_IRQ_WAKEUP_MASK_COUNT;

	for (uint8_t val = 0; val < GPC_CMC_IRQ_WAKEUP_MASK_COUNT; val++) {
		cpu_irq_mask_cfg.mask[val] = wake_mask[val];
	}

	scmi_nxp_cpu_set_irq_mask(&cpu_irq_mask_cfg);

	struct scmi_nxp_cpu_sleep_mode_config cpu_cfg = {0};
	/* restore M7 core state into ACTIVE. */
	cpu_cfg.cpu_id = CPU_IDX_M7P;
	cpu_cfg.sleep_mode = CPU_SLEEP_MODE_RUN;
	scmi_nxp_cpu_sleep_mode_set(&cpu_cfg);

	/* Clear PRIMASK */
	__enable_irq();
}

#endif /* CONFIG_PM */
