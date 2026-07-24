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
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/logging/log.h>

#include <cy_syspm.h>
#include <cy_syspm_pdcm.h>
#include <cy_syspm_ppu.h>
#include <cy_sysclk.h>
#include <cy_syslib.h>
#include <system_edge.h>

#include <soc.h>
#include <power.h>

/*
 * Warm-boot peripheral re-init is compiled only when DeepSleep-RAM can
 * power-cycle the peripheral domain (CONFIG_PM_S2RAM) and the PM device model is
 * present to carry each peripheral's TURN_ON rebuild (CONFIG_PM_DEVICE).
 */
#if defined(CONFIG_PM_S2RAM) && defined(CONFIG_PM_DEVICE)
#define IFX_PM_WARM_BOOT 1
#endif

#if defined(CONFIG_PM_S2RAM)
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/arch/common/init.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <kernel_arch_func.h>
#endif

LOG_MODULE_REGISTER(soc_power, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_PM)
/*
 * Zephyr PM-policy bridge for the MTB deepsleep lock (see the --wrap flags in
 * this SoC's CMakeLists.txt).
 *
 * mtb_hal_syspm_lock_deepsleep()/unlock_deepsleep() are the vendor HAL's "keep
 * the core out of DeepSleep for now" guard: MTB HAL and SRF operations that must
 * not be interrupted by a low-power transition bracket themselves with these
 * calls.  The CM55 SRF/IPC relay's blocking completion wait
 * (mtb_srf_ipc_request_send()) is the most prominent caller, but it is not the
 * only one - any library that uses the vendor deepsleep lock relies on the same
 * behavior.  On their own, though, they only increment/decrement an MTB-internal
 * counter that is consulted solely by the MTB HAL's own deepsleep entry, a path
 * the Zephyr pm_state_set() never takes, so on Zephyr they do nothing to
 * actually keep the core awake.  While such a section is in progress and the
 * calling thread blocks with no other thread ready, the idle thread can let the
 * PM policy enter a CPU-gating state (SUSPEND_TO_IDLE / SUSPEND_TO_RAM), which
 * gates the core and drops the pending completion so the operation never
 * finishes.
 *
 * The linker redirects both symbols here via --wrap: __real_* runs the original
 * vendor bookkeeping, and around it we take/release a Zephyr PM-policy lock so
 * only shallow runtime-idle (WFI, interrupts live) stays available for the
 * duration of the guarded section.  This protects every caller of the vendor
 * deepsleep lock platform-wide, not just the SRF client, without editing the
 * vendor module or touching each call site.  The get/put calls are
 * reference-counted, so nested sections balance correctly.
 */
void __real_mtb_hal_syspm_lock_deepsleep(void);
void __real_mtb_hal_syspm_unlock_deepsleep(void);

void __wrap_mtb_hal_syspm_lock_deepsleep(void)
{
	__real_mtb_hal_syspm_lock_deepsleep();
	ifx_pm_deepsleep_lock();
}

void __wrap_mtb_hal_syspm_unlock_deepsleep(void)
{
	ifx_pm_deepsleep_unlock();
	__real_mtb_hal_syspm_unlock_deepsleep();
}
#endif /* CONFIG_PM */

#if defined(CONFIG_PM_S2RAM)

#if defined(IFX_PM_WARM_BOOT)
/*
 * Set true by ifx_pm_s2ram_enter() when the Zephyr S2RAM marker
 * (pm_s2ram_mark_check_and_clear()) confirms a genuine DeepSleep-RAM warm boot,
 * i.e. arch_pm_s2ram_suspend() returned 0 because the CPU was power-cycled and
 * resumed through the reset handler.  Consumed and cleared by
 * pm_state_exit_post_ops() when it bumps the warm-boot generation counter, so a
 * failed DS-RAM entry (which never powered the domain down) does not trigger a
 * spurious peripheral re-init.
 */
static bool ifx_pm_s2ram_warm_boot;
#endif /* IFX_PM_WARM_BOOT */

/*
 * Custom S2RAM marker (CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING).
 *
 * Warm resume IS supported.  The marker, the saved CPU context (_cpu_context)
 * and the suspend stack all live in the CM55 "RAM" output region, which the
 * linker places at 0x26100000 (SOCMEM-backed system RAM) -- NOT in the CM55
 * tightly-coupled memories.  Only ITCM (0x00000000) and DTCM (0x20000000) are
 * powered down with the CPU subsystem during DeepSleep-RAM; the SOCMEM RAM is
 * retained because pm_state_set() programs the SOCMEM DeepSleep mode to
 * CY_SYSPM_MODE_DEEPSLEEP_RAM before entry.  So everything the reset handler
 * needs to warm-resume survives the DS-RAM power cycle.
 *
 * arch_pm_s2ram_suspend() saves _cpu_context and pushes the callee-saved GPRs
 * onto the (retained) stack, calls pm_s2ram_mark_set() to arm the marker, then
 * enters DS-RAM.  The DS-RAM wake is a full reset: the CM55 reset handler runs
 * arch_pm_s2ram_resume() -> pm_s2ram_mark_check_and_clear() very early (before
 * .bss is zeroed), and when the marker is found set it restores _cpu_context,
 * pops the GPRs and returns into ifx_pm_s2ram_enter() with rc == 0, resuming at
 * the WFI call site instead of re-running kernel init and main().
 *
 * The marker is self-clearing: pm_s2ram_mark_check_and_clear() clears it on
 * every boot, so a genuine cold boot (marker naturally 0 in zeroed/retained
 * RAM after a power cycle, or cleared by the previous resume) falls through to
 * normal init.  A hardware warm-boot witness readable this early from CM55-NS
 * does not exist (SRSS reset-cause is PPC-secured under TF-M and SRF/IPC is not
 * up in the reset handler); if a more robust discriminator than the retained
 * marker is required, the secure core can read Cy_SysPm_GetBootMode() and hand
 * the result to CM55-NS, which validates/clears the marker once the kernel and
 * IPC are up.
 */
#define IFX_S2RAM_MARKER_MAGIC 0xDABBAD00U

static __noinit uint32_t ifx_s2ram_marker;

void pm_s2ram_mark_set(void)
{
	/*
	 * Arm the warm-boot marker just before DS-RAM entry.  It lives in the
	 * retained SOCMEM RAM region (see the note above), so it survives the
	 * DS-RAM power cycle and is visible to arch_pm_s2ram_resume() in the
	 * reset handler.
	 */
	ifx_s2ram_marker = IFX_S2RAM_MARKER_MAGIC;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	/*
	 * Called from arch_pm_s2ram_resume() in the reset handler, before .bss
	 * is zeroed.  If the marker is set this is a DS-RAM warm boot: report it
	 * so the caller restores _cpu_context and returns to the suspend point.
	 * The marker is always cleared (self-clearing) so a later genuine cold
	 * boot is not mistaken for a warm boot.
	 */
	bool was_suspended = (ifx_s2ram_marker == IFX_S2RAM_MARKER_MAGIC);

	/* Data cache is disabled this early after reset, so this write reaches
	 * retained SOCMEM RAM directly; drain the write buffer to be certain the
	 * cleared marker survives even if a later step faults.
	 */
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

/**
 * @brief Low-power mode entry for ds-ram and ds-off.
 *
 * Called by arch_pm_s2ram_suspend() after CPU context has been saved.
 * Cleans the D-cache so the retained warm-boot state is coherent with
 * SOCMEM, then enters DeepSleep-RAM/OFF via Cy_SysPm_CpuEnterDeepSleep(),
 * which sets SLEEPDEEP, brackets the FPU, and executes WFI internally.
 *
 * @return Does not return on a successful warm boot (resumes through the
 *         reset handler); returns -EAGAIN if the CPU wakes without a power
 *         cycle.
 */
static int enter_deepsleep(void)
{
	/*
	 * The warm-boot state that arch_pm_s2ram_resume() reads back in the
	 * reset handler -- the S2RAM marker (ifx_s2ram_marker), the saved CPU
	 * context (_cpu_context) and the callee-saved GPRs pushed onto the
	 * suspend stack -- all live in the retained SOCMEM RAM region but were
	 * written through the CM55 write-back D-cache by arch_pm_s2ram_suspend()
	 * and pm_s2ram_mark_set().  DeepSleep-RAM powers the CPU subsystem (and
	 * therefore the L1 D-cache) down, so any line still dirty in the cache
	 * is lost and never reaches physical SOCMEM.  On warm boot the marker
	 * check would then read a stale value, miss the warm boot and cold-boot
	 * instead.  A bare __DSB() is not enough: it orders accesses but does
	 * not evict dirty write-back lines.  Clean (write back) the D-cache so
	 * the retained copies are coherent with SOCMEM before entry.
	 */
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	SCB_CleanDCache();
#endif
	__DSB();
	__ISB();

	Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
	SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;

	return -EAGAIN;
}

/**
 * @brief Restore system state after DS-RAM warm boot.
 *
 * DS-RAM retains all SRAM/SOCMEM, so the C runtime, kernel state, the
 * SystemCoreClock value and the SRF/IPC client context all survive, and the
 * CM33 companion re-establishes the clock tree before it re-enables this core.
 * The only state physically lost is the CM55 tightly-coupled memory (ITCM,
 * powered down with the CPU subsystem) and the L1 caches.  Both are restored
 * here with purely local operations.
 *
 * Crucially this runs on the resumed CPU context, i.e. with PRIMASK = 1 (the
 * interrupt-masked state that was saved at suspend), so it must NOT call any
 * routine that relays to the secure core over SRF/IPC.  In particular
 * soc_early_init_hook() and SystemCoreClockUpdate() are deliberately NOT called
 * here: SystemCoreClockUpdate() -> Cy_SysClk_ClkHfGetFrequency() is a
 * PPC-secured SRF relay whose blocking IPC semaphore wait cannot complete with
 * interrupts masked and faults (observed as a HardFault in
 * Cy_SysClk_ClkHfGetFrequency).  Neither is needed anyway because RAM (and thus
 * SystemCoreClock) is retained and the clock tree is already live.
 *
 * CM33 does not use ITCM — its ramfunc sections reside in retained SRAM and
 * survive DS-RAM.  When CM33 S2RAM support is added, only the cache
 * re-initialization portion is needed.
 */
static void s2ram_restore_system(void)
{
#if defined(CONFIG_CPU_CORTEX_M55)
	/* ITCM is lost during DS-RAM (CPU subsystem powers down).
	 * Re-copy code/rodata from flash and zero BSS before calling
	 * any ITCM-resident function.
	 */
	extern void data_copy_xip_relocation(void);
	extern void bss_zeroing_relocation(void);
	extern char __itcm_start[];
	extern char __itcm_load_start[];
	extern char __itcm_size[];

	data_copy_xip_relocation();
	bss_zeroing_relocation();
	arch_early_memcpy(&__itcm_start, &__itcm_load_start, (size_t)&__itcm_size);
#endif /* CONFIG_CPU_CORTEX_M55 */

	/* Re-enable the L1 caches, which are invalidated by the DS-RAM power
	 * cycle.  These are local CPU (CMSIS) operations with no secure relay,
	 * so they are safe on this interrupt-masked resume path.
	 * z_arm_restore_scb_context() later restores CCR/cache enables from the
	 * saved SCB context.
	 */
#if defined(__ICACHE_PRESENT) && (__ICACHE_PRESENT == 1U)
	SCB_InvalidateICache();
#endif
#if defined(__DCACHE_PRESENT) && (__DCACHE_PRESENT == 1U)
	SCB_InvalidateDCache();
#endif
}

/**
 * @brief Enter suspend-to-RAM (DeepSleep-RAM) state.
 *
 * Saves FPU, SCB, MPU, and NVIC context, then calls arch_pm_s2ram_suspend
 * which saves CPU registers and invokes enter_deepsleep().  On warm boot
 * the reset handler restores CPU context and returns here with rc == 0.
 *
 * Resume ordering is critical:
 *   1. CPACR — enables FPU/MVE coprocessors so compiler-generated
 *      vector instructions in subsequent C code do not fault (NOCP).
 *   2. System restore — ITCM re-copy, clock tree, caches.
 *   3. Clear bus faults — peripheral re-init may have generated
 *      benign imprecise bus faults; clear them while BUSFAULTENA
 *      is still at its reset default (disabled).
 *   4. Full SCB restore — including SHCSR with BUSFAULTENA.
 *   5. MPU, FP, NVIC restore.
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
#if defined(IFX_PM_WARM_BOOT)
		/* Marker-confirmed warm boot: the peripheral domain was
		 * power-cycled.  Record it so pm_state_exit_post_ops() schedules
		 * the deferred peripheral re-init worker.
		 */
		ifx_pm_s2ram_warm_boot = true;
#endif /* IFX_PM_WARM_BOOT */

		/* Step 1: enable FPU/MVE coprocessors (CPACR defaults to 0
		 * after DS-RAM reset).  Full SCB restore is deferred — its
		 * BUSFAULTENA would escalate pending bus faults from step 2.
		 */
#if defined(CPACR_PRESENT)
		SCB->CPACR = scb_state.cpacr;
		__DSB();
		__ISB();
#endif
		/* Step 2: re-init ITCM, clocks, caches */
		s2ram_restore_system();

		/* Steps 3: restore remaining core context */
		z_arm_restore_scb_context(&scb_state);
#if defined(CONFIG_ARM_MPU)
		z_arm_restore_mpu_context(&mpu_state);
#endif
		z_arm_restore_fp_context(&fpu_state);
		s2ram_restore_nvic_context(&nvic_state);
		__DSB();
		__ISB();
	}

	if (rc != 0) {
		LOG_WRN("S2RAM entry failed: rc=%d ICSR=0x%08x", rc, (uint32_t)SCB->ICSR);
	}
}
#endif /* CONFIG_PM_S2RAM */

#if defined(IFX_PM_WARM_BOOT)
/*
 * DeepSleep-RAM warm-boot re-init, performed lazily in the consuming thread.
 *
 * DS-RAM power-cycles the peripheral domain, so after a warm boot every
 * peripheral has lost its hardware state and must be rebuilt: a power-loss
 * OFF->SUSPENDED transition, expressed by PM_DEVICE_ACTION_TURN_ON (not RESUME,
 * which is for a retained low-power state where the hardware kept its config).
 * The rebuild relays PPC-secured clock/PDL operations to the secure core over
 * the SRF/IPC, which blocks, so it cannot run from pm_state_exit_post_ops()
 * (scheduler-locked) and must not race the waking application.
 *
 * Rather than rebuild everything up front from a worker - which would either race
 * the already-runnable application thread the moment a relay blocks, or force us
 * to suspend the application threads for the duration - each driver rebuilds
 * itself lazily, on its first use after the warm boot, from the thread that is
 * about to use it.  That thread blocks on its own re-init (the idle thread WFIs
 * and the IPC completes by ISR), then proceeds; because it is the same thread
 * that will touch the peripheral, there is no cross-thread race and no console
 * corruption.
 *
 * ifx_pm_warm_boot_gen is bumped once per genuine warm boot (see
 * pm_state_exit_post_ops()).  A driver records the generation it last rebuilt at
 * in its retained driver data and calls ifx_pm_warm_boot_reinit() at each API
 * entry; the counters differ only on the first use after a warm boot.
 */
static atomic_t ifx_pm_warm_boot_gen;

#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
/* Generation whose warm-boot prerequisites have already been re-established. */
static atomic_t ifx_pm_prepared_gen;
static K_MUTEX_DEFINE(ifx_pm_prepare_mutex);

/*
 * Re-establish the global prerequisites every driver's warm-boot rebuild depends
 * on, once per warm-boot generation, before any per-driver TURN_ON runs.
 *
 * DS-RAM power-cycles the peripheral domain, so on the first post-wake use two
 * things must be restored, in order, ahead of any driver rebuild:
 *
 *   1. The base clock tree.  A driver cannot reprogram its own peripheral clock
 *      until the tree it hangs off is running again.  On this SoC the base tree
 *      is restored by the CM33 companion during its own warm boot and the CM55
 *      system clock is republished by the SRF client bring-up below, so there is
 *      no separate CM55-side step here; a SoC without a secure companion would
 *      re-run its clock init at this point.
 *
 *   2. The SRF/IPC client.  The CM33 secure image cold-booted and republished
 *      the IPC/SRF endpoint during the warm boot; without re-establishing the
 *      client the stale pre-sleep handle would fault on the first secure request
 *      (every PPC-secured clock/PDL relay a driver rebuild issues).  Each
 *      consuming driver then re-programs its own peripheral clock divider from
 *      its TURN_ON handler (there are no separate divider devices on this core),
 *      so no explicit divider re-program step is needed here.
 *
 * Single-flight: concurrent first-users serialize on the mutex and only the
 * winner runs the blocking handshake.  Must run in thread context.
 */
static void ifx_pm_warm_boot_prepare(uint32_t gen)
{
	if ((uint32_t)atomic_get(&ifx_pm_prepared_gen) == gen) {
		return;
	}

	k_mutex_lock(&ifx_pm_prepare_mutex, K_FOREVER);

	if ((uint32_t)atomic_get(&ifx_pm_prepared_gen) != gen) {
		/*
		 * Hold a Zephyr PM-policy lock across the client re-init.  Its
		 * blocking IPC semaphore give/take is NOT bracketed by the vendor
		 * deepsleep lock and uses an infinite timeout, so without this the idle
		 * thread could CPU-gate the core mid-handshake and drop the pending
		 * completion.  Only shallow runtime-idle (WFI, interrupts live) stays
		 * available for the duration.
		 */
		ifx_pm_deepsleep_lock();
		ifx_soc_srf_client_reinit();
		ifx_pm_deepsleep_unlock();

		atomic_set(&ifx_pm_prepared_gen, (atomic_val_t)gen);
	}

	k_mutex_unlock(&ifx_pm_prepare_mutex);
}
#endif /* CONFIG_PSOC_EDGE_M55_SRF_SUPPORT */

/*
 * Eager warm-boot rebuild: walk every device in link (initialization) order and
 * invoke its PM_DEVICE_ACTION_TURN_ON handler, restoring all peripherals in a
 * single up-front pass instead of lazily on first use.
 *
 * Public so an application can force a full peripheral restore after a
 * DeepSleep-RAM warm boot from its own thread context.  On builds without the
 * SRF relay this is also the automatic path: pm_state_exit_post_ops() calls it
 * while the scheduler is still locked, because a driver's TURN_ON rebuild is
 * then pure local MMIO (pinctrl, IP init, clock divider, NVIC) and never blocks.
 *
 * On SRF builds each TURN_ON handler relays PPC-secured clock/PDL operations to
 * the secure core over blocking IPC, so this must run in thread context: it
 * first re-establishes the SRF client - once per warm-boot generation, via
 * ifx_pm_warm_boot_prepare(), which also keeps the per-driver lazy path from
 * redundantly re-establishing it - and then walks with the PM-policy lock held
 * so the idle thread cannot CPU-gate the core mid-handshake.  A peripheral
 * touched by a thread before this call rebuilds itself lazily; one touched after
 * it is already live, and its lazy TURN_ON becomes an idempotent repeat.
 *
 * Devices are walked in link order, so each is rebuilt only after the parents it
 * was initialized after; a device with no TURN_ON handler returns -ENOTSUP and
 * is skipped.
 */
void ifx_pm_warm_boot_reinit_all(void)
{
#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
	ifx_pm_warm_boot_prepare((uint32_t)atomic_get(&ifx_pm_warm_boot_gen));
	ifx_pm_deepsleep_lock();
#endif
	STRUCT_SECTION_FOREACH(device, dev) {
		struct pm_device_base *pm = dev->pm_base;

		if ((pm != NULL) && (pm->action_cb != NULL)) {
			(void)pm->action_cb(dev, PM_DEVICE_ACTION_TURN_ON);
		}
	}
#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
	ifx_pm_deepsleep_unlock();
#endif
}

bool ifx_pm_warm_boot_reinit(const struct device *dev, uint32_t *last_gen)
{
#if defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
	uint32_t gen = (uint32_t)atomic_get(&ifx_pm_warm_boot_gen);
	struct pm_device_base *pm;

	if (*last_gen == gen) {
		return false;
	}

	/*
	 * The rebuild relays over blocking IPC, so it needs thread context.  If the
	 * first post-wake use happens in an ISR, skip it: the device stays stale
	 * until a thread-context caller rebuilds it.  In practice the application
	 * touches each peripheral from a thread before any ISR does.
	 */
	if (k_is_in_isr()) {
		return false;
	}

	ifx_pm_warm_boot_prepare(gen);

	/*
	 * Record the generation before invoking TURN_ON, not after.  Some drivers
	 * (e.g. PWM/counter) restore their hardware from their own API entry inside
	 * the TURN_ON handler; that nested call re-enters this function, and marking
	 * the generation first makes it observe an up-to-date value and fall through
	 * to the real work instead of recursing.  A failed rebuild is therefore not
	 * retried, which is acceptable: a warm-boot re-init failure is unrecoverable
	 * for that peripheral regardless.
	 */
	*last_gen = gen;

	pm = dev->pm_base;
	if ((pm != NULL) && (pm->action_cb != NULL)) {
		/*
		 * Invoke TURN_ON directly (not via pm_device_action_run()) so the
		 * rebuild runs regardless of the device's tracked PM state and leaves
		 * the runtime PM bookkeeping untouched.  A driver with no warm-boot
		 * work returns -ENOTSUP, which is ignored here.
		 */
		(void)pm->action_cb(dev, PM_DEVICE_ACTION_TURN_ON);
	}

	return true;
#else  /* !CONFIG_PSOC_EDGE_M55_SRF_SUPPORT */
	/*
	 * Non-SRF builds rebuild every device eagerly in pm_state_exit_post_ops()
	 * (the rebuild never blocks), so the per-driver lazy hook has nothing left
	 * to do.  Kept as a no-op so drivers can call it unconditionally.
	 */
	ARG_UNUSED(dev);
	ARG_UNUSED(last_gen);
	return false;
#endif /* CONFIG_PSOC_EDGE_M55_SRF_SUPPORT */
}
#endif /* IFX_PM_WARM_BOOT */

/*
 * pm_state_set() has two build variants on this SoC:
 *
 *  - CM55 (IFX_PM_VARIANT_CM55, non-secure): the PDL services Cy_SysPm_*
 *    directly without the SRF relay, so it drives the DeepSleep transition.  Its
 *    default mode selectors are configured once in ifx_pm_init(); the whole-SoC
 *    DeepSleep-OFF is driven from the CM33 side.
 *
 *  - CM33 (IFX_PM_VARIANT_CM33_NS): everything that is not CM55.  Under TF-M the
 *    non-secure CM33 cannot enter any DeepSleep - TF-M keeps deep-sleep control
 *    secure-only (SCR.SLEEPDEEPS = 1), so a SLEEPDEEP write from the non-secure
 *    state is ignored and WFI always performs a shallow CPU sleep;
 *    Cy_SysPm_CpuEnter*() would relay to the secure image over a blocking
 *    SRF/IPC round-trip that cannot be issued from pm_state_set() anyway.  Every
 *    low-power state therefore collapses to a plain WFI (ifx_pm_cm33_ns_sleep());
 *    real DeepSleep is driven from the secure side.
 *
 *    The secure-stub CM33 companion (the pse84_boot.c build, which defines
 *    COMPONENT_SECURE_DEVICE) also maps here.  It runs its own boot-and-sleep
 *    loop and does not rely on Zephyr's pm_state_set() for DeepSleep, so the
 *    shallow-WFI fallback keeps that image building and behaving safely if it
 *    enables CONFIG_PM.
 */
#if defined(CONFIG_CPU_CORTEX_M55)
#define IFX_PM_VARIANT_CM55 1
#else
#define IFX_PM_VARIANT_CM33_NS 1
#endif

#if defined(IFX_PM_VARIANT_CM33_NS)
/*
 * CM33 non-secure shallow sleep.
 *
 * TF-M owns deep-sleep control (SCR.SLEEPDEEPS = 1), so this core cannot enter
 * any DeepSleep: clear SLEEPDEEP (a non-secure write to it is ignored anyway)
 * and execute a plain WFI.  Used for every PM state on this build variant.
 */
static void ifx_pm_cm33_ns_sleep(void)
{
	SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
	__DSB();
	__WFI();
}
#endif /* IFX_PM_VARIANT_CM33_NS */

__weak void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	/* Use PRIMASK instead of BASEPRI for wake-up from standby. */
	__disable_irq();
	irq_unlock(0);

	switch (state) {
	case PM_STATE_RUNTIME_IDLE:
		LOG_DBG("Entering PM state runtime idle");
#if defined(IFX_PM_VARIANT_CM33_NS)
		ifx_pm_cm33_ns_sleep();
#else
		Cy_SysPm_CpuEnterSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
#endif
		break;
	case PM_STATE_SUSPEND_TO_IDLE:
		LOG_DBG("Entering PM state suspend to idle");
#if defined(IFX_PM_VARIANT_CM33_NS)
		ifx_pm_cm33_ns_sleep();
#else
		Cy_SysPm_DeepSleepSetup(CY_SYSPM_MODE_DEEPSLEEP);
		Cy_SysPm_SetSOCMEMDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);
		Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
		SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
#endif /* IFX_PM_VARIANT_CM33_NS */
		break;
#if defined(CONFIG_PM_S2RAM)
	case PM_STATE_SUSPEND_TO_RAM:
		LOG_DBG("Entering PM state suspend to RAM (DS-RAM)");
#if defined(IFX_PM_VARIANT_CM33_NS)
		ifx_pm_cm33_ns_sleep();
#else
		Cy_SysPm_DeepSleepSetup(CY_SYSPM_MODE_DEEPSLEEP_RAM);
		Cy_SysPm_SetSOCMEMDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP_RAM);
		ifx_pm_s2ram_enter();
#endif /* IFX_PM_VARIANT_CM33_NS */
		break;
#endif
	case PM_STATE_SOFT_OFF:
		if (substate_id == 1) {
			LOG_DBG("Entering PM state soft-off (DS-OFF)");
#if defined(IFX_PM_VARIANT_CM33_NS)
			ifx_pm_cm33_ns_sleep();
#else
			Cy_SysPm_DeepSleepSetup(CY_SYSPM_MODE_DEEPSLEEP_OFF);
			Cy_SysPm_SetSOCMEMDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP_OFF);
			Cy_SysPm_CpuEnterDeepSleep(CY_SYSPM_WAIT_FOR_INTERRUPT);
			SCB_SCR &= (uint32_t)~SCB_SCR_SLEEPDEEP_Msk;
#endif /* IFX_PM_VARIANT_CM33_NS */
		} else {
			LOG_DBG("Entering PM state soft-off (Hibernate)");
#if defined(CONFIG_POWEROFF)
			/*
			 * Route hibernate through the poweroff driver
			 * (z_sys_poweroff()), the one path valid across every build
			 * variant.  pm_state_set() runs with PRIMASK=1 (see the
			 * __disable_irq() at the top), so it cannot itself complete the
			 * secure SRF/IPC hibernate relay the TF-M builds require; the
			 * poweroff driver re-enables interrupts and either issues that
			 * relay (CM33-NS and CM55 under TF-M, where SRSS_PWR_HIBERNATE is
			 * PPC-secured and cannot be written from the non-secure core) or
			 * writes the register directly (non-TF-M / stub-secure build).
			 * sys_poweroff() never returns.
			 */
			__enable_irq();
			sys_poweroff();
#elif !defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
			/*
			 * Non-TF-M (stub-secure) build: SRSS_PWR_HIBERNATE is directly
			 * accessible, so enter hibernate in place.
			 * Cy_SysPm_SystemEnterHibernate() never returns.
			 */
			Cy_SysPm_SystemEnterHibernate();
#else
			/*
			 * TF-M / SRF build without a poweroff driver: hibernate can only
			 * be entered over the secure SRF relay, which needs thread
			 * context (a blocking IPC round-trip) and so cannot be issued
			 * from here.  Enable CONFIG_POWEROFF to reach it.
			 */
			LOG_ERR("Hibernate needs CONFIG_POWEROFF on TF-M/SRF builds");
#endif
		}
		break;
	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

/*
 * Zephyr PM expects interrupts re-enabled at exit.  pm_state_set() uses
 * PRIMASK=1 / BASEPRI=0 so any enabled interrupt can wake the CPU without
 * entering an ISR.  We clear PRIMASK here to resume normal dispatch.
 */
__weak void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(state);
	ARG_UNUSED(substate_id);

	__enable_irq();

#if defined(IFX_PM_WARM_BOOT)
	/*
	 * A DeepSleep-RAM warm boot power-cycles the peripheral domain, so every
	 * peripheral must be re-initialized by its device TURN_ON pm_action.
	 * ifx_pm_s2ram_warm_boot is set by ifx_pm_s2ram_enter() only on a genuine
	 * warm boot (never on the retained suspend-to-idle path), so the generation
	 * advances only when peripheral state was actually lost.
	 *
	 * How the rebuild is driven depends on whether the SRF relay is in play:
	 *
	 *  - With SRF, the rebuild relays to the secure core (PPC-secured clock/PDL
	 *    calls over blocking IPC), which needs the scheduler running and
	 *    interrupts enabled - neither is true here (this runs under
	 *    k_sched_lock with PRIMASK just cleared) - and rebuilding up front would
	 *    also race the waking application thread.  So just bump the generation
	 *    counter; each driver rebuilds itself lazily on its first post-wake use,
	 *    from the consuming thread's own context (see ifx_pm_warm_boot_reinit()).
	 *
	 *  - Without SRF the rebuild is non-blocking local MMIO, so do it eagerly
	 *    now, while the scheduler is still locked and no application thread has
	 *    run: it completes ahead of the first peripheral use with no race.
	 */
	if (ifx_pm_s2ram_warm_boot) {
		ifx_pm_s2ram_warm_boot = false;
		atomic_inc(&ifx_pm_warm_boot_gen);
#if !defined(CONFIG_PSOC_EDGE_M55_SRF_SUPPORT)
		ifx_pm_warm_boot_reinit_all();
#endif
	}
#endif /* IFX_PM_WARM_BOOT */
}

static int ifx_pm_init(void)
{
#if defined(IFX_PM_VARIANT_CM55)
	/*
	 * On the CM55 the PDL services these calls directly (no SRF relay), so
	 * configure the default DeepSleep modes once here at init rather than
	 * from the application.  Keeping the APP CPU domain in regular DeepSleep
	 * (not OFF) retains this core across a DeepSleep so it wakes on any
	 * enabled interrupt; the whole-SoC DeepSleep-OFF is driven from the
	 * CM33-NS side.
	 */
	Cy_SysPm_DeepSleepSetup(CY_SYSPM_MODE_DEEPSLEEP);

	/* System Domain Idle Power Mode Configuration */
	Cy_SysPm_SetDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

	/* APP CPU Domain Idle Power Mode Configuration */
	Cy_SysPm_SetAppDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);

	/* SoCMEM Idle Power Mode Configuration */
	Cy_SysPm_SetSOCMEMDeepSleepMode(CY_SYSPM_MODE_DEEPSLEEP);
#endif

	return 0;
}

SYS_INIT(ifx_pm_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
