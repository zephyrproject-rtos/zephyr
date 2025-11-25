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

#if defined(CONFIG_SOC_MIMX94398_M7_0) || defined(CONFIG_SOC_MIMX94398_M7_1)
#include <zephyr/arch/common/pm_s2ram.h>
#include <cmsis_core.h>

typedef struct {
	uint32_t ISER[8];
	uint32_t ISPR[8];
	uint8_t IPR[240];
} _nvic_context_t;

typedef struct {
	uint32_t ICSR;
	uint32_t VTOR;
	uint32_t AIRCR;
	uint32_t SCR;
	uint32_t CCR;
	uint32_t SHPR[12U];
	uint32_t SHCSR;
	uint32_t CFSR;
	uint32_t HFSR;
	uint32_t DFSR;
	uint32_t MMFAR;
	uint32_t BFAR;
	uint32_t AFSR;
	uint32_t CPACR;
} _scb_context_t;

static _nvic_context_t nvic_context_t;
static _scb_context_t scb_context_t;

static inline void reg32_array_copy(uint32_t *dst,
		volatile uint32_t *src, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		dst[i] = src[i];
	}
}

static void nvic_save(void)
{
	reg32_array_copy(nvic_context_t.ISER, (uint32_t *)NVIC->ISER, ARRAY_SIZE(NVIC->ISER));
	reg32_array_copy(nvic_context_t.ISPR, (uint32_t *)NVIC->ISPR, ARRAY_SIZE(NVIC->ISPR));
	reg32_array_copy((uint32_t *)nvic_context_t.IPR, (volatile uint32_t *)NVIC->IPR,
			sizeof(NVIC->IPR) / sizeof(uint32_t));
}

static void nvic_restore(void)
{
	reg32_array_copy((uint32_t *)NVIC->ISER, nvic_context_t.ISER, ARRAY_SIZE(NVIC->ISER));
	reg32_array_copy((uint32_t *)NVIC->ISPR, nvic_context_t.ISPR, ARRAY_SIZE(NVIC->ISPR));
	reg32_array_copy((uint32_t *)NVIC->IPR, (uint32_t *)nvic_context_t.IPR,
			sizeof(NVIC->IPR) / sizeof(uint32_t));
}

static void scb_save(void)
{
	scb_context_t.ICSR = SCB->ICSR;
	scb_context_t.VTOR = SCB->VTOR;
	scb_context_t.AIRCR = SCB->AIRCR;
	scb_context_t.SCR = SCB->SCR;
	scb_context_t.CCR = SCB->CCR;
	reg32_array_copy(scb_context_t.SHPR, (uint32_t *)SCB->SHPR, ARRAY_SIZE(SCB->SHPR));
	scb_context_t.SHCSR = SCB->SHCSR;
	scb_context_t.CFSR = SCB->CFSR;
	scb_context_t.HFSR = SCB->HFSR;
	scb_context_t.DFSR = SCB->DFSR;
	scb_context_t.MMFAR = SCB->MMFAR;
	scb_context_t.BFAR = SCB->BFAR;
	scb_context_t.AFSR = SCB->AFSR;
	scb_context_t.CPACR = SCB->CPACR;
}

static void scb_restore(void)
{
	SCB->ICSR = scb_context_t.ICSR;
	SCB->VTOR = scb_context_t.VTOR;
	SCB->AIRCR = scb_context_t.AIRCR;
	SCB->SCR = scb_context_t.SCR;
	SCB->CCR = scb_context_t.CCR;
	reg32_array_copy((uint32_t *)SCB->SHPR, scb_context_t.SHPR, ARRAY_SIZE(SCB->SHPR));
	SCB->SHCSR = scb_context_t.SHCSR;
	SCB->CFSR = scb_context_t.CFSR;
	SCB->HFSR = scb_context_t.HFSR;
	SCB->DFSR = scb_context_t.DFSR;
	SCB->MMFAR = scb_context_t.MMFAR;
	SCB->BFAR = scb_context_t.BFAR;
	SCB->AFSR = scb_context_t.AFSR;
	SCB->CPACR = scb_context_t.CPACR;
}

static int pm_suspend_to_ram(void)
{
	struct scmi_cpu_sleep_mode_config cpu_cfg = {0};

	cpu_cfg.cpu_id = cpu_idx;
	cpu_cfg.sleep_mode = CPU_SLEEP_MODE_SUSPEND;
	scmi_cpu_sleep_mode_set(&cpu_cfg);
	__DSB();
	__WFI();

	/* Suspend to ram mode entering fail */
	return -EBUSY;
}

static int pm_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	int ret;
	struct scmi_cpu_vector_config vector_cfg;

	/*
	 * Configure CPU reset vector for S2RAM resume flow.
	 * In S2RAM mode, when CPU resumes from poweroff state:
	 * 1. CPU starts execution from reset_handler (default vector table)
	 * 2. S2RAM framework detects resume condition via pm_s2ram_mark_check_and_clear()
	 * 3. If S2RAM resume is detected, execution jumps to arch_pm_s2ram_resume()
	 * 4. arch_pm_s2ram_resume() restores the suspended context (stack, heap, registers)
	 * 5. Execution continues from the point where pm_s2ram_suspend() was called
	 *
	 * Setting vector_low/high = 0 keeps the default vector table unchanged,
	 * allowing the standard reset flow to handle S2RAM resume detection.
	 * The RESUME flag indicates this vector configuration is for resume scenarios.
	 */
	vector_cfg.cpu_id = cpu_idx;
	vector_cfg.flags = SCMI_CPU_VEC_FLAGS_RESUME;
	vector_cfg.vector_low = 0;
	vector_cfg.vector_high = 0;

	ret = scmi_cpu_reset_vector(&vector_cfg);
	if (ret < 0) {
		return ret;
	}

	nvic_save();
	scb_save();

	ret = arch_pm_s2ram_suspend(system_off);

	nvic_restore();
	scb_restore();

	return ret;
}

void pm_s2ram_mark_set(void)
{
	/* empty */
}

bool pm_s2ram_mark_check_and_clear(void)
{
	int ret;
	struct scmi_cpu_info cpu_info;

	/*
	 * Query CPU status from SCMI to determine boot reason.
	 * During S2RAM suspend, the CPU sleep_mode is set to CPU_SLEEP_MODE_SUSPEND.
	 * On resume from poweroff, this state persists until explicitly cleared,
	 * allowing us to detect S2RAM resume vs normal boot.
	 */
	ret = scmi_cpu_info_get(cpu_idx, &cpu_info);
	if (ret < 0) {
		return false;
	}

	return (cpu_info.sleep_mode == CPU_SLEEP_MODE_SUSPEND);
}
#endif /* CONFIG_SOC_MIMX94398_M7_0) || CONFIG_SOC_MIMX94398_M7_1) */

static void pm_state_before(enum pm_state state)
{
	struct scmi_cpu_pd_lpm_config cpu_pd_lpm_cfg;
	struct scmi_cpu_irq_mask_config cpu_irq_mask_cfg;
	uint32_t lpm_setting;

	/*
	 * 1. Set M33 mix as power on state in suspend mode as it include netc device.
	 *    Todo: add netc device suspend and resume to support the m33 mix poweroff.
	 * 2. Keep wakeupmix power on whatever low power mode, as lpuart(console) in there.
	 */

	cpu_pd_lpm_cfg.cpu_id = cpu_idx;
	cpu_pd_lpm_cfg.num_cfg = 2;

	if (state == PM_STATE_SUSPEND_TO_RAM) {
		lpm_setting = SCMI_CPU_LPM_SETTING_ON_RUN_WAIT_STOP;
	} else {
		lpm_setting = SCMI_CPU_LPM_SETTING_ON_ALWAYS;
	}

	cpu_pd_lpm_cfg.cfgs[0].domain_id = pwr_mix_idx;
	cpu_pd_lpm_cfg.cfgs[0].lpm_setting = lpm_setting;
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

	pm_state_before(state);

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
	case PM_STATE_SUSPEND_TO_RAM:
#if defined(CONFIG_SOC_MIMX94398_M7_0) || defined(CONFIG_SOC_MIMX94398_M7_1)
		pm_s2ram_suspend(pm_suspend_to_ram);
#endif
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
