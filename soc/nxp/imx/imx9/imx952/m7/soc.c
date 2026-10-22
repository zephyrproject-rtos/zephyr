/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/init.h>
#include <zephyr/pm/pm.h>
#include <zephyr/arch/arch_interface.h>
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

static int soc_init(void)
{
	int ret = 0;

#if defined(CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS)
	struct scmi_nxp_cpu_sleep_mode_config cpu_cfg = {0};

	/*
	 * SLEEP_HOLD_EN is enabled by default, which gates SysTick.
	 * Set CPU sleep mode to RUN to clear it and allow SysTick to run.
	 */
	cpu_cfg.cpu_id = CPU_IDX_M7P;
	cpu_cfg.sleep_mode = CPU_SLEEP_MODE_RUN;

	ret = scmi_nxp_cpu_sleep_mode_set(&cpu_cfg);
#endif /* CONFIG_NXP_SCMI_CPU_DOMAIN_HELPERS */

	return ret;
}

/*
 * Because platform is using ARM SCMI, drivers like scmi, mbox etc. are
 * initialized during PRE_KERNEL_1. Common init hooks is not able to use.
 * SoC early init and board early init could be run during PRE_KERNEL_2 instead.
 */
SYS_INIT(soc_init, PRE_KERNEL_2, 0);

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

static void enter_low_power(struct scmi_nxp_cpu_sleep_mode_config *cpu_cfg)
{
	unsigned int key;

	scmi_nxp_cpu_sleep_mode_set(cpu_cfg);
	key = arch_pm_state_set_prepare();
	__DSB();
	__WFI();
	arch_pm_state_set_finish(key);
}

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	struct scmi_nxp_cpu_sleep_mode_config cpu_cfg = {0};

	pm_state_before();

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_WAIT;
		enter_low_power(&cpu_cfg);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_STOP;
		enter_low_power(&cpu_cfg);
		break;
	case PM_STATE_STANDBY:
		cpu_cfg.cpu_id = CPU_IDX_M7P;
		cpu_cfg.sleep_mode = CPU_SLEEP_MODE_SUSPEND;
		enter_low_power(&cpu_cfg);
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
}

#endif /* CONFIG_PM */
