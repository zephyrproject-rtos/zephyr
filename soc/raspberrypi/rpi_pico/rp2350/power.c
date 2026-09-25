/*
 * SPDX-FileCopyrightText: 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/pm/pm.h>
#include <zephyr/logging/log.h>

#include <cmsis_core.h>

/* Light sleep needs nothing beyond WFI, and POWMAN is only pulled into the
 * build by PM_S2RAM, so everything below has to stay optional for boards that
 * enable PM without a suspend-to-ram power state.
 */
#if defined(CONFIG_PM_S2RAM)
#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/drivers/clock_control/clock_control_rpi_pico.h>
#include <zephyr/sys/util.h>

#include <hardware/powman.h>
#include <hardware/regs/powman.h>
#include <hardware/structs/powman.h>
#endif /* CONFIG_PM_S2RAM */

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_PM_S2RAM)

#define NVIC_MEMBER_SIZE(member) ARRAY_SIZE(((NVIC_Type *)0)->member)

struct rp2350_nvic_context {
	uint32_t iser[NVIC_MEMBER_SIZE(ISER)];
	uint32_t ispr[NVIC_MEMBER_SIZE(ISPR)];
	uint8_t ipr[NVIC_MEMBER_SIZE(IPR)];
};

static void rp2350_nvic_save(struct rp2350_nvic_context *ctx)
{
	memcpy(ctx->iser, (uint32_t *)NVIC->ISER, sizeof(ctx->iser));
	memcpy(ctx->ispr, (uint32_t *)NVIC->ISPR, sizeof(ctx->ispr));
	memcpy(ctx->ipr, (uint32_t *)NVIC->IPR, sizeof(ctx->ipr));
}

static void rp2350_nvic_restore(const struct rp2350_nvic_context *ctx)
{
	memcpy((uint32_t *)NVIC->ISER, ctx->iser, sizeof(ctx->iser));
	memcpy((uint32_t *)NVIC->ISPR, ctx->ispr, sizeof(ctx->ispr));
	memcpy((uint32_t *)NVIC->IPR, ctx->ipr, sizeof(ctx->ipr));
}

/* Only clocks and CPU core state are restored on resume; peripherals under
 * the powered-down SWCORE domain (UART, GPIO, DMA, ...) come back reset.
 */
static int rp2350_s2ram_off(void)
{
	powman_power_state state = POWMAN_POWER_STATE_NONE;

	state = powman_power_state_with_domain_on(state, POWMAN_POWER_DOMAIN_XIP_CACHE);
	state = powman_power_state_with_domain_on(state, POWMAN_POWER_DOMAIN_SRAM_BANK0);
	state = powman_power_state_with_domain_on(state, POWMAN_POWER_DOMAIN_SRAM_BANK1);

	powman_set_bits(&powman_hw->dbg_pwrcfg, POWMAN_DBG_PWRCFG_IGNORE_BITS);

	if (powman_set_power_state(state) != PICO_OK) {
		LOG_WRN("state req rejected: current_pwrup_req=0x%02x dbg_pwrcfg=0x%02x",
			powman_hw->current_pwrup_req, powman_hw->dbg_pwrcfg);
		powman_clear_bits(&powman_hw->dbg_pwrcfg, POWMAN_DBG_PWRCFG_IGNORE_BITS);
		return -EBUSY;
	}

	__DSB();
	__WFI();

	/* Only reached if the power-down did not take effect. */
	powman_clear_bits(&powman_hw->dbg_pwrcfg, POWMAN_DBG_PWRCFG_IGNORE_BITS);
	return -EBUSY;
}

static void rp2350_restore_clocks(void)
{
	const struct device *clocks = DEVICE_DT_GET(DT_NODELABEL(clocks));
	int ret = clock_control_rpi_pico_reconfigure(clocks);

	if (ret < 0) {
		LOG_ERR("failed to restore clocks after SUSPEND_TO_RAM: %d", ret);
	}
}

static struct scb_context rp2350_scb_context;
static struct rp2350_nvic_context rp2350_nvic_ctx;
#if defined(CONFIG_ARM_MPU)
static struct z_mpu_context_retained rp2350_mpu_context;
#endif
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
static struct fpu_ctx_full rp2350_fpu_context;
#endif

/* arch_pm_s2ram_suspend() only covers the base CPU special registers; NVIC,
 * SCB, MPU, and (unshared) FPU state also need saving here since SWCORE
 * power-down resets them too. ret == 0 means a genuine resume, restored
 * before any device's PM callback runs.
 */
static int rp2350_s2ram_suspend(void)
{
	int ret;

	z_arm_save_scb_context(&rp2350_scb_context);
	rp2350_nvic_save(&rp2350_nvic_ctx);
#if defined(CONFIG_ARM_MPU)
	z_arm_save_mpu_context(&rp2350_mpu_context);
#endif
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
	z_arm_save_fp_context(&rp2350_fpu_context);
#endif

	ret = arch_pm_s2ram_suspend(rp2350_s2ram_off);

	if (ret == 0) {
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
		z_arm_restore_fp_context(&rp2350_fpu_context);
#endif
#if defined(CONFIG_ARM_MPU)
		z_arm_restore_mpu_context(&rp2350_mpu_context);
#endif
		rp2350_nvic_restore(&rp2350_nvic_ctx);
		z_arm_restore_scb_context(&rp2350_scb_context);

		rp2350_restore_clocks();
	}

	return ret;
}

#endif /* CONFIG_PM_S2RAM */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
	case PM_STATE_SUSPEND_TO_IDLE: {
		unsigned int key = __get_PRIMASK();

		/* WFI wakes on a pending IRQ even with PRIMASK set, so masking here
		 * keeps the wake ISR from running before the PM core has finished
		 * resuming. Restored right after, since pm_state_exit_post_ops()
		 * must not unmask interrupts.
		 */
		__disable_irq();
		__WFI();
		__set_PRIMASK(key);
		break;
	}
#if defined(CONFIG_PM_S2RAM)
	case PM_STATE_SUSPEND_TO_RAM: {
		int ret = rp2350_s2ram_suspend();

		if (ret < 0) {
			LOG_WRN("SUSPEND_TO_RAM not entered: %d", ret);
		}
		break;
	}
#endif /* CONFIG_PM_S2RAM */
	default:
		LOG_DBG("PM state not supported: %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

#if defined(CONFIG_PM_S2RAM)
	if (state == PM_STATE_SUSPEND_TO_RAM) {
		/* Cleared here, not in rp2350_s2ram_off(): a real wake resumes past
		 * arch_pm_s2ram_suspend(), never back into that function.
		 */
		powman_clear_bits(&powman_hw->dbg_pwrcfg, POWMAN_DBG_PWRCFG_IGNORE_BITS);
	}
#else
	ARG_UNUSED(state);
#endif /* CONFIG_PM_S2RAM */
}
