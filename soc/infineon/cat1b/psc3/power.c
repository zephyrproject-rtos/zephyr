/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/irq.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/platform/hooks.h>

#include <cy_syspm.h>
#include <cy_syslib.h>

#include <soc.h>

#include "power.h"

#if defined(CONFIG_PM_S2RAM)
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#endif /* CONFIG_PM_S2RAM */

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

/*
 * DeepSleep-RAM warm-boot entry point. The boot ROM requires one before CPU
 * power-down; point it at the Zephyr reset vector table so wake runs the normal
 * reset handler, which calls arch_pm_s2ram_resume() (before .bss zeroing) to
 * check the retained-SRAM marker and resume in ifx_pm_s2ram_enter().
 */
extern void *_vector_table[];

/*
 * Arm the CPU-off deep-sleep hardware and issue WFI. Caller programs the
 * warm-boot entry and target mode. Returns -EAGAIN only if entry requirements
 * were not met (e.g. active debug) and a plain deep sleep woke on interrupt; a
 * genuine entry does not return (DeepSleep-RAM warm boots, DeepSleep-OFF cold
 * boots through the reset handler).
 */
static int ifx_pm_deep_arm_wfi(void)
{
	if (!Cy_SysPm_IsLpmReady()) {
		return -EAGAIN;
	}

	Cy_SysLib_ClearResetReason();

	SCB_SCR |= SCB_SCR_SLEEPDEEP_Msk;
	__DSB();

#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U)
	SCS_CPPWR |= SCS_ENABLE_CPPWR_SU10_SU11;
	SCB_CPACR &= ~SCB_ENABLE_CPACR_CP10_CP11;
	__DSB();
#endif

	SRSS_PWR_CTL2 |= _VAL2FLD(SRSS_PWR_CTL2_PORBOD_LPMODE, 1U);

	__WFI();

	/* Undo the arming so normal operation resumes after a failed entry. */
	SRSS_PWR_CTL2 &= ~_VAL2FLD(SRSS_PWR_CTL2_PORBOD_LPMODE, 1U);

#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U)
	SCS_CPPWR &= ~SCS_ENABLE_CPPWR_SU10_SU11;
	SCB_CPACR |= SCB_ENABLE_CPACR_CP10_CP11;
	__DSB();
#endif

	SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;

	return -EAGAIN;
}

#if defined(CONFIG_PM_S2RAM)

/*
 * Custom suspend-to-RAM marker (CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING). The marker,
 * saved CPU context and suspend stack live in DeepSleep-RAM-retained SRAM.
 * arch_pm_s2ram_suspend() saves context and arms the marker; on wake the reset
 * handler's arch_pm_s2ram_resume() checks it (before .bss zeroing) and resumes
 * in ifx_pm_s2ram_enter() with rc == 0. Self-clearing, so a genuine cold boot
 * falls through to normal init.
 */
#define IFX_S2RAM_MARKER_MAGIC 0xDABBAD00U

static __noinit uint32_t ifx_s2ram_marker;

void pm_s2ram_mark_set(void)
{
	/* Arm the warm-boot marker (retained SRAM) just before DeepSleep-RAM entry. */
	ifx_s2ram_marker = IFX_S2RAM_MARKER_MAGIC;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	/*
	 * Called from arch_pm_s2ram_resume() before .bss zeroing. A set marker means
	 * a warm boot; always clear it (self-clearing) so a later cold boot is not
	 * mistaken for one. __DSB() flushes the clear to SRAM before any later fault.
	 */
	bool was_suspended = (ifx_s2ram_marker == IFX_S2RAM_MARKER_MAGIC);

	ifx_s2ram_marker = 0U;
	__DSB();

	return was_suspended;
}

#define NVIC_ISER_COUNT DIV_ROUND_UP(CONFIG_NUM_IRQS, 32)
#define NVIC_IPR_COUNT  CONFIG_NUM_IRQS

struct nvic_context {
	uint32_t iser[NVIC_ISER_COUNT];
	uint8_t ipr[NVIC_IPR_COUNT];
};

static struct fpu_ctx_full fpu_state;
static struct scb_context scb_state;
#if defined(CONFIG_ARM_MPU)
static struct z_mpu_context_retained mpu_state;
#endif
static struct nvic_context nvic_state;

static void s2ram_save_nvic_context(struct nvic_context *ctx)
{
	for (uint32_t i = 0; i < NVIC_ISER_COUNT; i++) {
		ctx->iser[i] = NVIC->ISER[i];
	}
	for (uint32_t i = 0; i < NVIC_IPR_COUNT; i++) {
		ctx->ipr[i] = NVIC->IPR[i];
	}
}

static void s2ram_restore_nvic_context(const struct nvic_context *ctx)
{
	for (uint32_t i = 0; i < NVIC_ISER_COUNT; i++) {
		NVIC->ISER[i] = ctx->iser[i];
	}
	for (uint32_t i = 0; i < NVIC_IPR_COUNT; i++) {
		NVIC->IPR[i] = ctx->ipr[i];
	}
}

/*
 * Low-power entry callback from arch_pm_s2ram_suspend() (CPU context saved).
 * Returns -EAGAIN on a no-power-cycle wake; a genuine entry does not return
 * here - wake goes through the reset handler back into ifx_pm_s2ram_enter().
 */
static int enter_deepsleep(void)
{
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	SCB_CleanDCache();
#endif
	__DSB();
	__ISB();

	return ifx_pm_deep_arm_wfi();
}

/*
 * Re-program the global clock tree after a DeepSleep-RAM warm boot. The warm
 * resume bypasses the PRE_KERNEL_1 clock_control init, so without this every
 * peripheral would run off the fallback IMO. ifx_warm_boot_clock_devs is the
 * ordered set of clock devices that program the tree (fixed clocks/DPLLs,
 * clk_hf roots, peripheral dividers). STRUCT_SECTION_FOREACH walks devices in
 * cold-boot init order (ITERABLE_SECTION_ROM_NUMERIC), which the DPLL-lock vs
 * path-mux/HF ordering requires. The init functions are self-contained HW
 * programming with no kernel-object side effects, so re-running them is safe on
 * this interrupt-masked path.
 */
#define IFX_WARM_BOOT_CLOCK_DEV(node_id) DEVICE_DT_GET(node_id),

static const struct device *const ifx_warm_boot_clock_devs[] = {
	DT_FOREACH_STATUS_OKAY(infineon_fixed_clock, IFX_WARM_BOOT_CLOCK_DEV)
		DT_FOREACH_STATUS_OKAY(infineon_fixed_factor_clock, IFX_WARM_BOOT_CLOCK_DEV)
			DT_FOREACH_STATUS_OKAY(infineon_peri_div, IFX_WARM_BOOT_CLOCK_DEV)};

static bool ifx_is_warm_boot_clock_dev(const struct device *dev)
{
	for (size_t i = 0; i < ARRAY_SIZE(ifx_warm_boot_clock_devs); i++) {
		if (ifx_warm_boot_clock_devs[i] == dev) {
			return true;
		}
	}

	return false;
}

static void ifx_pm_warm_boot_reinit_clocks(void)
{
	STRUCT_SECTION_FOREACH(device, dev) {
		if (ifx_is_warm_boot_clock_dev(dev) && (dev->ops.init != NULL)) {
			(void)dev->ops.init(dev);
		}
	}
}

/*
 * Restore system state after a DeepSleep-RAM warm boot. SRAM (C runtime, kernel
 * state, ramfunc) survives; re-invalidate the power-cycled L1 caches, then
 * rebuild the clock tree (bypassed by the warm resume). Cache ops are local
 * CMSIS; z_arm_restore_scb_context() later restores CCR/cache enables.
 */
static void s2ram_restore_system(void)
{
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
	SCB_InvalidateICache();
#endif
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	SCB_InvalidateDCache();
#endif

	/*
	 * Re-run vendor SystemInit (exception handlers), rebuild the clock tree so
	 * the DPLLs relock and peripheral clocks return at their configured rates,
	 * then refresh the cached SystemCoreClock.
	 */
	soc_early_init_hook();
	ifx_pm_warm_boot_reinit_clocks();
	SystemCoreClockUpdate();
}

/*
 * Enter suspend-to-RAM (DeepSleep-RAM).
 *
 * Saves FPU, SCB, MPU and NVIC context, then calls arch_pm_s2ram_suspend(),
 * which saves the CPU registers and invokes enter_deepsleep().  On a warm boot
 * the reset handler restores the CPU registers and returns here with rc == 0.
 *
 * Resume ordering is critical:
 *   1. CPACR first - enables the FPU coprocessor so compiler-generated FPU
 *      instructions in the C code below do not fault (NOCP); CPACR resets to 0.
 *   2. System restore (caches).
 *   3. Full SCB, then MPU, FP and NVIC restore.
 */
void ifx_pm_s2ram_enter(void)
{
	int rc;

	z_arm_save_fp_context(&fpu_state);
	z_arm_save_scb_context(&scb_state);
#if defined(CONFIG_ARM_MPU)
	z_arm_save_mpu_context(&mpu_state);
#endif
	s2ram_save_nvic_context(&nvic_state);

	rc = arch_pm_s2ram_suspend(enter_deepsleep);

	if (rc == 0) {
#if defined(CPACR_PRESENT)
		SCB->CPACR = scb_state.cpacr;
		__DSB();
		__ISB();
#endif
		s2ram_restore_system();

		z_arm_restore_scb_context(&scb_state);

		/*
		 * VTOR == 0 relies on the boot-time alias mirroring _vector_table to
		 * address 0, which a warm boot does not re-establish. Point VTOR at the
		 * absolute vector table address, valid regardless of the alias state.
		 */
		SCB->VTOR = (uint32_t)&_vector_table[0];
		__DSB();
		__ISB();

#if defined(CONFIG_ARM_MPU)
		z_arm_restore_mpu_context(&mpu_state);
#endif
		z_arm_restore_fp_context(&fpu_state);
		s2ram_restore_nvic_context(&nvic_state);
		__DSB();
		__ISB();

		/* Rebuild every power-cycled peripheral (clock tree is back). */
		ifx_pm_warm_boot_reinit_all();

		/* Unfreeze DeepSleep I/O only after the pins are reprogrammed. */
		if (Cy_SysPm_DeepSleepIoIsFrozen()) {
			Cy_SysPm_DeepSleepIoUnfreeze();
		}
	} else {
		LOG_WRN("S2RAM entry failed: rc=%d ICSR=0x%08x", rc, (uint32_t)SCB->ICSR);
	}
}

#if defined(CONFIG_PM_DEVICE)
/*
 * Per-driver lazy warm-boot rebuild hook - no-op on this SoC. The eager
 * ifx_pm_warm_boot_reinit_all() already restores every peripheral before the
 * first post-wake API entry, so nothing is left to do. Kept so drivers can call
 * it unconditionally at their API boundaries.
 */
bool ifx_pm_warm_boot_reinit(const struct device *dev, uint32_t *last_gen)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(last_gen);

	return false;
}

/*
 * Rebuild every device after a DeepSleep-RAM warm boot (eager path). Walks
 * devices in init order and drives PM_DEVICE_ACTION_TURN_ON via
 * pm_device_action_run(), restoring all power-cycled peripherals up front.
 */
void ifx_pm_warm_boot_reinit_all(void)
{
	if (k_is_in_isr()) {
		return;
	}

	STRUCT_SECTION_FOREACH(device, dev) {
		struct pm_device_base *pm = (struct pm_device_base *)dev->pm_base;

		if ((pm == NULL) || (pm->action_cb == NULL)) {
			continue;
		}

		/* Peripheral lost power; force tracked state to OFF so TURN_ON is legal. */
		pm->state = PM_DEVICE_STATE_OFF;
		(void)pm_device_action_run(dev, PM_DEVICE_ACTION_TURN_ON);
	}
}
#endif /* CONFIG_PM_DEVICE */
#endif /* CONFIG_PM_S2RAM */

/*
 * Called from pm_system_suspend(int32_t ticks) in subsys/power.c
 * For deep sleep pm_system_suspend has executed all the driver
 * power management call backs.
 */
__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	/* Switch to using PRIMASK instead of BASEPRI register, since
	 * we are only able to wake up from standby while using PRIMASK.
	 */
	/* Set PRIMASK */
	__disable_irq();

	/* Set BASEPRI to 0 */
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("Entering PM state runtime idle");
		Cy_SysPm_CpuEnterSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("Entering PM state suspend to idle");
		Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);
		Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
		SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
		break;
	case PM_STATE_SUSPEND_TO_RAM:
		LOG_DBG("Entering PM state suspend to RAM (DeepSleep-RAM)");
#if defined(CONFIG_PM_S2RAM)
		Cy_Syslib_SetWarmBootEntryPoint((uint32_t *)_vector_table, false);
		Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP_RAM);
		ifx_pm_s2ram_enter();
		SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
#else
		LOG_WRN("DeepSleep-RAM requires CONFIG_PM_S2RAM");
#endif /* CONFIG_PM_S2RAM */
		break;
	case PM_STATE_SOFT_OFF:
		if (substate_id == 1) {
			LOG_DBG("Entering PM state soft-off (DeepSleep-OFF)");
			Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP_OFF);
			(void)ifx_pm_deep_arm_wfi();
			SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
		} else {
			LOG_DBG("Entering PM state soft-off (Hibernate)");
			Cy_SysPm_SystemEnterHibernate();
		}
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/*
 * Zephyr PM code expects us to enabled interrupts at post op exit. Zephyr used
 * arch_irq_lock() which sets BASEPRI to a non-zero value masking all interrupts
 * preventing wake. MCHP z_power_soc_(deep)_sleep sets PRIMASK=1 and BASEPRI=0
 * allowing wake from any enabled interrupt and prevents the CPU from entering
 * an ISR on wake except for faults. We re-enable interrupts by setting PRIMASK
 * to 0.
 */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	/* Clear PRIMASK */
	__enable_irq();
}

/*
 * Release the Hibernate I/O freeze and arm a Hibernate wakeup source. Without an
 * armed source the chip enters Hibernate with no wake and needs a reset/power
 * cycle to recover.
 */
#if DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup)

#define IFX_HIB_WAKEUP_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(infineon_hibernate_wakeup)

/* Build the wakeup-source mask from the devicetree */
static uint32_t ifx_hib_wakeup_source_mask(void)
{
	uint32_t src = 0U;

#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_pin0_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_pin0_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_PIN0_LOW
		       : CY_SYSPM_HIBERNATE_PIN0_HIGH;
#endif
#if DT_NODE_HAS_PROP(IFX_HIB_WAKEUP_NODE, wakeup_pin1_trigger)
	src |= (DT_ENUM_IDX(IFX_HIB_WAKEUP_NODE, wakeup_pin1_trigger) == 0)
		       ? CY_SYSPM_HIBERNATE_PIN1_LOW
		       : CY_SYSPM_HIBERNATE_PIN1_HIGH;
#endif
#if DT_PROP(IFX_HIB_WAKEUP_NODE, wakeup_wdt)
	src |= CY_SYSPM_HIBERNATE_WDT;
#endif
#if DT_PROP(IFX_HIB_WAKEUP_NODE, wakeup_rtc_alarm)
	src |= CY_SYSPM_HIBERNATE_RTC_ALARM;
#endif

	return src;
}

/* Arm the Hibernate wakeup source(s) described by the devicetree node */
static void ifx_hib_wakeup_arm(void)
{
	uint32_t src = ifx_hib_wakeup_source_mask();

	if (src == 0U) {
		return;
	}

	if (Cy_SysPm_IoIsFrozen()) {
		Cy_SysPm_IoUnfreeze();
	}

	Cy_SysPm_SetHibernateWakeupSource(src);
}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup) */

static int ifx_pm_init(void)
{
	Cy_SysPm_Init();

	if (Cy_SysPm_DeepSleepIoIsFrozen()) {
		Cy_SysPm_DeepSleepIoUnfreeze();
	}
	if (Cy_SysPm_IoIsFrozen()) {
		Cy_SysPm_IoUnfreeze();
	}

	Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

#if DT_HAS_COMPAT_STATUS_OKAY(infineon_hibernate_wakeup)
	ifx_hib_wakeup_arm();
#endif

	return 0;
}

SYS_INIT(ifx_pm_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
