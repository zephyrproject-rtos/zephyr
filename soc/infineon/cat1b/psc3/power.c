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
 * DeepSleep-RAM warm-boot entry point.
 *
 * The boot ROM requires a warm-boot entry point to be installed before the CPU
 * enters DeepSleep-RAM; without one the entry sequence faults instead of
 * powering the CPU down.  Point it at the normal Zephyr reset vector table so
 * the wake runs the ordinary reset handler exactly like a cold boot: the ROM
 * loads _vector_table[0] into MSP and branches to _vector_table[1]
 * (z_arm_reset), which calls arch_pm_s2ram_resume() very early - before .bss is
 * zeroed - which checks the S2RAM marker in retained SRAM and returns execution
 * into ifx_pm_s2ram_enter() at the point it suspended.
 */
extern void *_vector_table[];

/*
 * Arm the CPU-off deep-sleep hardware and issue WFI.
 *
 * DeepSleep-RAM and DeepSleep-OFF both power the CPU subsystem down; the boot
 * ROM warm-boot entry and the target power mode are programmed by the caller.
 *
 * Returns -EAGAIN only if the entry requirements were not met (for example an
 * active debug session) so the device performed a plain deep sleep and woke on
 * a pending interrupt; a genuine entry does not return here (DeepSleep-RAM warm
 * boots through the reset handler, DeepSleep-OFF cold boots).
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
 * Custom suspend-to-RAM marker (CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING).
 *
 * The marker, the saved CPU context (_cpu_context) and the suspend stack all
 * live in SRAM, which DeepSleep-RAM keeps powered (only the CPU subsystem is
 * power-gated), so everything arch_pm_s2ram_resume() needs to warm-resume
 * survives the power cycle.  arch_pm_s2ram_suspend() saves _cpu_context, pushes
 * the callee-saved GPRs onto the retained stack and calls pm_s2ram_mark_set()
 * to arm the marker before DeepSleep-RAM entry.  On wake the device cold-boots;
 * the reset handler calls arch_pm_s2ram_resume() -> pm_s2ram_mark_check_and_clear()
 * before .bss is zeroed, and when the marker is set it restores the context and
 * returns into ifx_pm_s2ram_enter() with rc == 0 instead of re-running kernel
 * init and main().  The marker is self-clearing so a genuine cold boot (marker
 * naturally 0, or cleared by a previous resume) falls through to normal init.
 */
#define IFX_S2RAM_MARKER_MAGIC 0xDABBAD00U

static __noinit uint32_t ifx_s2ram_marker;

void pm_s2ram_mark_set(void)
{
	/*
	 * Arm the warm-boot marker just before DeepSleep-RAM entry.  It lives in
	 * retained SRAM, so it survives the power cycle and is visible to
	 * arch_pm_s2ram_resume() in the reset handler.
	 */
	ifx_s2ram_marker = IFX_S2RAM_MARKER_MAGIC;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	/*
	 * Called from arch_pm_s2ram_resume() in the reset handler, before .bss is
	 * zeroed.  A set marker means this is a DeepSleep-RAM warm boot: report it
	 * so the caller restores the CPU context and returns to the suspend point.
	 * Always clear the marker (self-clearing) so a later genuine cold boot is
	 * not mistaken for a warm boot.  Drain the write buffer so the cleared
	 * marker reaches SRAM even if a later step faults.
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
 * Low-power entry callback invoked by arch_pm_s2ram_suspend() after the CPU
 * context has been saved.
 *
 * Returns -EAGAIN if the CPU wakes without a power cycle (pending interrupt);
 * on a genuine DeepSleep-RAM entry it does not return here - the device wakes
 * through the reset handler and arch_pm_s2ram_resume() resumes in
 * ifx_pm_s2ram_enter() instead.
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
 * Re-program the global clock tree after a DeepSleep-RAM warm boot.
 *
 * The warm resume returns from the reset handler before the SYS_INIT and
 * device-init phases run, so the Infineon clock_control drivers - which build
 * the DPLL(s), the clk_hf roots and the peripheral dividers at PRE_KERNEL_1 -
 * never re-run.  soc_early_init_hook() only runs the vendor SystemInit (the
 * system exception handlers); it does not touch the clock tree, so without this
 * every peripheral would run off the fallback IMO.
 *
 * ifx_warm_boot_clock_devs is the set of clock devices whose init functions
 * program the tree (the fixed clocks and DPLLs, the clk_hf fixed-factor roots,
 * and the peripheral dividers).  The path-mux/HF ordering versus the DPLL lock
 * matters, so the rebuild must reproduce the cold-boot order exactly.  The
 * device iterable section is sorted by init level and priority
 * (ITERABLE_SECTION_ROM_NUMERIC), so STRUCT_SECTION_FOREACH(device) walks
 * devices in their original initialization order; re-running only the clock
 * members' init functions there rebuilds the tree in the same order the
 * cold-boot init used.  These init functions are self-contained hardware
 * programming with no kernel-object side effects, so re-invoking them is safe
 * on this interrupt-masked resume path.
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
 * Restore system state after a DeepSleep-RAM warm boot.
 *
 * DeepSleep-RAM retains all SRAM, so the C runtime, kernel state and the
 * ramfunc sections all survive.  The L1 caches, which the power cycle
 * invalidates, are re-initialized, then the clock tree is re-programmed because
 * the warm resume bypasses the cold-boot init that normally runs it.  The cache
 * operations are local CMSIS operations with no secure relay, so they are safe
 * on this interrupt-masked resume path; z_arm_restore_scb_context() later
 * restores CCR/cache enables from the saved SCB context.
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
	 * Re-run the vendor SystemInit (exception handlers) and then rebuild the
	 * global clock tree the clock_control drivers program at cold boot, so the
	 * DPLLs relock and the peripheral clocks (system timer, console) return at
	 * their configured rates.  Refresh the cached SystemCoreClock afterwards.
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
		 * The running system uses VTOR == 0, relying on the boot-time alias
		 * that mirrors the vector table (linked at _vector_table) down to
		 * address 0.  A DeepSleep-RAM warm boot does not re-establish that
		 * alias, so the saved VTOR of 0 now points at unmapped memory and the
		 * first exception would fault.  Point VTOR at the absolute vector
		 * table address, which is valid regardless of the alias state.
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

		if (Cy_SysPm_DeepSleepIoIsFrozen()) {
			Cy_SysPm_DeepSleepIoUnfreeze();
		}

#if defined(CONFIG_PM_DEVICE)
		/*
		 * Rebuild every power-cycled peripheral eagerly, here on the resume
		 * path, before control returns to the PM subsystem and the application.
		 * The clock tree is back (s2ram_restore_system) and the I/O cells are
		 * live, so each driver's TURN_ON handler can reprogram its block now and
		 * the console is usable immediately.
		 */
		ifx_pm_warm_boot_reinit_all();
#endif /* CONFIG_PM_DEVICE */
	} else {
		LOG_WRN("S2RAM entry failed: rc=%d ICSR=0x%08x", rc, (uint32_t)SCB->ICSR);
	}
}

#if defined(CONFIG_PM_DEVICE)
/*
 * Per-driver warm-boot rebuild hook (no-op on this SoC).
 *
 * DeepSleep-RAM warm boots rebuild every peripheral eagerly from the resume
 * path: ifx_pm_s2ram_enter() calls ifx_pm_warm_boot_reinit_all() while
 * interrupts are still masked and before any application thread has run, so the
 * rebuild - pure local MMIO on this single secure core, with no SRF/IPC relay
 * that would have to block - completes ahead of the first peripheral use with no
 * race.  By the time a driver reaches its first post-wake API entry its block is
 * already restored, so this per-driver lazy hook has nothing left to do.
 *
 * It is kept so drivers can call it unconditionally at their API boundaries
 * without knowing how the SoC drives the rebuild; here it simply reports
 * "nothing to do".
 */
bool ifx_pm_warm_boot_reinit(const struct device *dev, uint32_t *last_gen)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(last_gen);

	return false;
}

/*
 * Rebuild every device after a DeepSleep-RAM warm boot (eager path).
 *
 * Walks every device in link (initialization) order and invokes its
 * PM_DEVICE_ACTION_TURN_ON handler directly, restoring all power-cycled
 * peripherals in a single up-front pass.  ifx_pm_s2ram_enter() calls this
 * automatically on every warm boot, so peripherals are live again before
 * control returns to the PM subsystem and the application.
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

		(void)pm->action_cb(dev, PM_DEVICE_ACTION_TURN_ON);
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
		Cy_Syslib_SetWarmBootEntryPoint((uint32_t *)_vector_table, true);
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
			Cy_Syslib_SetWarmBootEntryPoint((uint32_t *)_vector_table, true);
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
 * Release the Hibernate I/O freeze and arm a Hibernate wakeup source.
 *
 * The unfreeze releases any freeze latched by a previous Hibernate wakeup; the arm
 * selects the source(s) that will wake the next Hibernate entry.  Without it the
 * chip enters Hibernate with no wakeup source and can only be recovered by a
 * reset or power cycle.
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
