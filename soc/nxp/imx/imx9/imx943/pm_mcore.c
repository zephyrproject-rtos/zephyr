/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/firmware/scmi/nxp/cpu.h>
#include <zephyr/dt-bindings/power/imx943_power.h>
#include <pm_mcore.h>

static void pm_state_before(void)
{
	struct scmi_cpu_pd_lpm_config cpu_pd_lpm_cfg;
	struct scmi_cpu_irq_mask_config cpu_irq_mask_cfg;

	/*
	 * 1. Set CPU mix as power on state in suspend mode
	 * 2. Keep wakeupmix power on whatever low power mode, as lpuart(console) in there.
	 */

	cpu_pd_lpm_cfg.cpu_id = cpu_idx;
	cpu_pd_lpm_cfg.num_cfg = 2;
	cpu_pd_lpm_cfg.cfgs[0].domain_id = pwr_mix_idx;
	cpu_pd_lpm_cfg.cfgs[0].lpm_setting = SCMI_CPU_LPM_SETTING_ON_ALWAYS;
	cpu_pd_lpm_cfg.cfgs[0].ret_mask = 1U << pwr_mem_idx;
	cpu_pd_lpm_cfg.cfgs[1].domain_id = PWR_MIX_SLICE_IDX_WAKEUP;
	cpu_pd_lpm_cfg.cfgs[1].lpm_setting = SCMI_CPU_LPM_SETTING_ON_ALWAYS;
	cpu_pd_lpm_cfg.cfgs[1].ret_mask = 0;

	scmi_cpu_pd_lpm_set(&cpu_pd_lpm_cfg);

	/* Set wakeup mask */
	uint32_t wake_mask[GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT] = {
		[0 ... GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT - 1]  = 0xFFFFFFFFU
	};

	/* IRQs enabled at NVIC level become GPC wake sources */
	for (uint32_t idx = 0; idx <
			MIN(GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT, nvic_iser_nb); idx++) {
		wake_mask[idx] = ~(NVIC->ISER[idx]);
	}

	cpu_irq_mask_cfg.cpu_id = cpu_idx;
	cpu_irq_mask_cfg.mask_idx = 0;
	cpu_irq_mask_cfg.num_mask = GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT;

	for (uint8_t val = 0; val < GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT; val++) {
		cpu_irq_mask_cfg.mask[val] = wake_mask[val];
	}

	scmi_cpu_set_irq_mask(&cpu_irq_mask_cfg);
}

static void pm_state_resume(void)
{
	struct scmi_cpu_irq_mask_config cpu_irq_mask_cfg;

	/* Restore scmi cpu wake mask */
	uint32_t wake_mask[GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT] = {
		[0 ... GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT - 1] = 0x0U
	};

	cpu_irq_mask_cfg.cpu_id = cpu_idx;
	cpu_irq_mask_cfg.mask_idx = 0;
	cpu_irq_mask_cfg.num_mask = GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT;

	for (uint8_t val = 0; val < GPC_CPU_CTRL_CMC_IRQ_WAKEUP_MASK_COUNT; val++) {
		cpu_irq_mask_cfg.mask[val] = wake_mask[val];
	}

	scmi_cpu_set_irq_mask(&cpu_irq_mask_cfg);
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	struct scmi_cpu_sleep_mode_config cpu_cfg = {0};

	pm_state_before();

	__disable_irq();
	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		cpu_cfg.cpu_id = cpu_idx;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_WAIT;
		scmi_cpu_sleep_mode_set(&cpu_cfg);
		__DSB();
		__WFI();
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		cpu_cfg.cpu_id = cpu_idx;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_STOP;
		scmi_cpu_sleep_mode_set(&cpu_cfg);
		__DSB();
		__WFI();
		break;
	case PM_STATE_STANDBY:
		cpu_cfg.cpu_id = cpu_idx;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_SUSPEND;
		scmi_cpu_sleep_mode_set(&cpu_cfg);
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
	struct scmi_cpu_sleep_mode_config cpu_cfg = {0};

	pm_state_resume();

	/* restore Mcore state into ACTIVE. */
	cpu_cfg.cpu_id = cpu_idx;
	cpu_cfg.sleep_mode = CPU_SLEEP_MODE_RUN;
	scmi_cpu_sleep_mode_set(&cpu_cfg);

	/* Clear PRIMASK */
	__enable_irq();
}
