/*
 * Copyright (c) 2024 Microchip Technology Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mec5_ktimer

#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <soc.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>
#include <cmsis_core.h>
#include <zephyr/irq.h>

#include <device_mec5.h>
#include <mec_btimer_api.h>
#include <mec_rtimer_api.h>

BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP), "MCHP MEC5 ktimer doesn't support SMP");
BUILD_ASSERT(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC == 32768,
	     "MCHP MEC5 ktimer HW frequency is fixed at 32768");

#ifndef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
BUILD_ASSERT(0, "MCHP MEC5 ktimer requires ARCH_HAS_CUSTOM_BUSY_WAIT");
#endif

#ifdef CONFIG_SOC_MEC_DEBUG_AND_TRACING
#define RTIMER_START_VAL MEC_RTIMER_START_EXT_HALT
#else
#define RTIMER_START_VAL MEC_RTIMER_START
#endif

/*
 * Overview:
 *
 * This driver enables the Microchip XEC 32KHz based RTOS timer as the Zephyr
 * system timer. It supports both legacy ("tickful") mode as well as
 * TICKLESS_KERNEL. The XEC RTOS timer is a down counter with a fixed
 * frequency of 32768 Hz. The driver is based upon the Intel local APIC
 * timer driver.
 * Configuration:
 *
 * CONFIG_MCHP_XEC_RTOS_TIMER=y
 *
 * CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC=<hz> must be set to 32768.
 *
 * To reduce truncation errors from accumulating due to conversion
 * to/from time, ticks, and HW cycles set ticks per second equal to
 * the frequency. With tickless kernel mode enabled the kernel will not
 * program a periodic timer at this fast rate.
 * CONFIG_SYS_CLOCK_TICKS_PER_SEC=32768
 */

#define CYCLES_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* Mask off bits[31:28] of 32-bit count */
#define RTIMER_MAX        0x0fffffffu
#define RTIMER_COUNT_MASK 0x0fffffffu
#define RTIMER_STOPPED    0xf0000000u

/* Adjust cycle count programmed into timer for HW restart latency */
#define RTIMER_ADJUST_LIMIT  2
#define RTIMER_ADJUST_CYCLES 1

/* max number of ticks we can load into the timer in one shot */
#define MAX_TICKS (RTIMER_MAX / CYCLES_PER_TICK)

#define RTIMER_NODE_ID   DT_INST(0, DT_DRV_COMPAT)
#define RTIMER_NVIC_NO   DT_INST_IRQN(0)
#define RTIMER_NVIC_PRIO DT_INST_IRQ(0, priority)

static struct mec_rtmr_regs *const rtimer = (struct mec_rtmr_regs *)DT_INST_REG_ADDR(0);

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
#define BTIMER_NODE_ID   DT_CHOSEN(rtimer_busy_wait_timer)
#define MEC5_BTIMER_FDIV (MEC5_BTIMER_MAX_FREQ_HZ / 1000000u)

static struct mec_btmr_regs *const btimer = (struct mec_btmr_regs *)DT_REG_ADDR(BTIMER_NODE_ID);
#endif

/*
 * The spinlock protects all access to the RTIMER registers, as well as
 * 'total_cycles', 'last_announcement', and 'cached_icr'.
 *
 * One important invariant that must be observed: `total_cycles` + `cached_icr`
 * is always an integral multiple of CYCLE_PER_TICK; this is, timer interrupts
 * are only ever scheduled to occur at tick boundaries.
 */

static struct k_spinlock lock;
static uint32_t total_cycles;
static uint32_t cached_icr = CYCLES_PER_TICK;

/*
 * Read the RTOS timer counter handling the case where the timer
 * has been reloaded within 1 32KHz clock of reading its count register.
 * The RTOS timer hardware must synchronize the write to its control register
 * on the AHB clock domain with the 32KHz clock domain of its internal logic.
 * This synchronization can take from nearly 0 time up to 1 32KHz clock as it
 * depends upon which 48MHz AHB clock with a 32KHz period the register write
 * was on. We detect the timer is in the load state by checking the read-only
 * count register and the START bit in the control register. If count register
 * is 0 and the START bit is set then the timer has been started and is in the
 * process of moving the preload register value into the count register.
 */
static inline uint32_t rtimer_count(void)
{
	uint32_t ccr = mec_hal_rtimer_count(rtimer);

	if ((ccr == 0) && mec_hal_rtimer_is_started(rtimer)) {
		ccr = cached_icr;
	}

	return ccr;
}

#ifdef CONFIG_TICKLESS_KERNEL

static uint32_t last_announcement; /* last time we called sys_clock_announce() */

/*
 * Request a timeout n Zephyr ticks in the future from now.
 * Requested number of ticks in the future of n <= 1 means the kernel wants
 * the tick announced as soon as possible, ideally no more than one tick
 * in the future.
 *
 * Per comment below we don't clear RTMR pending interrupt.
 * RTMR counter register is read-only and is loaded from the preload
 * register by a 0->1 transition of the control register start bit.
 * Writing a new value to preload only takes effect once the count
 * register reaches 0.
 */
void sys_clock_set_timeout(int32_t n, bool idle)
{
	ARG_UNUSED(idle);

	uint32_t ccr, temp;
	int full_ticks;          /* number of complete ticks we'll wait */
	uint32_t full_cycles;    /* full_ticks represented as cycles */
	uint32_t partial_cycles; /* number of cycles to first tick boundary */

	if (idle && (n == K_TICKS_FOREVER)) {
		/*
		 * We are not in a locked section. Are writes to two
		 * global objects safe from pre-emption?
		 */
		mec_hal_rtimer_stop(rtimer);
		cached_icr = RTIMER_STOPPED;
		return;
	}

	if (n < 1) {
		full_ticks = 0;
	} else if ((n == K_TICKS_FOREVER) || (n > MAX_TICKS)) {
		full_ticks = MAX_TICKS - 1;
	} else {
		full_ticks = n - 1;
	}

	full_cycles = full_ticks * CYCLES_PER_TICK;

	k_spinlock_key_t key = k_spin_lock(&lock);

	ccr = rtimer_count();

	/* turn off to clear any pending interrupt status */
	mec_hal_rtimer_stop(rtimer);
	mec_hal_rtimer_status_clear_all(rtimer);
	NVIC_ClearPendingIRQ(RTIMER_NVIC_NO);

	temp = total_cycles;
	temp += (cached_icr - ccr);
	temp &= RTIMER_COUNT_MASK;
	total_cycles = temp;

	partial_cycles = CYCLES_PER_TICK - (total_cycles % CYCLES_PER_TICK);
	cached_icr = full_cycles + partial_cycles;
	/* adjust for up to one 32KHz cycle startup time */
	temp = cached_icr;
	if (temp > RTIMER_ADJUST_LIMIT) {
		temp -= RTIMER_ADJUST_CYCLES;
	}

	mec_hal_rtimer_stop_and_load(rtimer, temp, RTIMER_START_VAL);

	k_spin_unlock(&lock, key);
}

/*
 * Return the number of Zephyr ticks elapsed from last call to
 * sys_clock_announce in the ISR. The caller casts uint32_t to int32_t.
 * We must make sure bit[31] is 0 in the return value.
 */
uint32_t sys_clock_elapsed(void)
{
	uint32_t ccr;
	uint32_t ticks;
	int32_t elapsed;

	k_spinlock_key_t key = k_spin_lock(&lock);

	ccr = rtimer_count();

	/* It may not look efficient but the compiler does a good job */
	elapsed = (int32_t)total_cycles - (int32_t)last_announcement;
	if (elapsed < 0) {
		elapsed = -1 * elapsed;
	}
	ticks = (uint32_t)elapsed;
	ticks += cached_icr - ccr;
	ticks /= CYCLES_PER_TICK;
	ticks &= RTIMER_COUNT_MASK;

	k_spin_unlock(&lock, key);

	return ticks;
}

static void mec5_ktimer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t cycles;
	int32_t ticks;

	k_spinlock_key_t key = k_spin_lock(&lock);

	mec_hal_rtimer_status_clear_all(rtimer);

	/* Restart the timer as early as possible to minimize drift... */
	mec_hal_rtimer_stop_and_load(rtimer, MAX_TICKS * CYCLES_PER_TICK, RTIMER_START_VAL);

	cycles = cached_icr;
	cached_icr = MAX_TICKS * CYCLES_PER_TICK;

	total_cycles += cycles;
	total_cycles &= RTIMER_COUNT_MASK;

	/* handle wrap by using (power of 2) - 1 mask */
	ticks = total_cycles - last_announcement;
	ticks &= RTIMER_COUNT_MASK;
	ticks /= CYCLES_PER_TICK;

	last_announcement = total_cycles;

	k_spin_unlock(&lock, key);
	sys_clock_announce(ticks);
}

#else
/* Non-tickless kernel build. */
static void mec5_ktimer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	mec_hal_rtimer_status_clear_all(rtimer);

	/* Restart the timer as early as possible to minimize drift... */
	mec_hal_rtimer_stop_and_load(rtimer, cached_icr, RTIMER_START_VAL);

	uint32_t temp = total_cycles + CYCLES_PER_TICK;

	total_cycles = temp & RTIMER_COUNT_MASK;
	k_spin_unlock(&lock, key);

	sys_clock_announce(1);
}

uint32_t sys_clock_elapsed(void)
{
	return 0U;
}
#endif /* CONFIG_TICKLESS_KERNEL */

/*
 * Warning RTOS timer resolution is 30.5 us.
 * This is called by two code paths:
 * 1. Kernel call to k_cycle_get_32() -> arch_k_cycle_get_32() -> here.
 *    The kernel is casting return to (int) and using it uncasted in math
 *    expressions with int types. Expression result is stored in an int.
 * 2. If CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT is not defined then
 *    z_impl_k_busy_wait calls here. This code path uses the value as uint32_t.
 *
 */
uint32_t sys_clock_cycle_get_32(void)
{
	uint32_t ret;
	uint32_t ccr;

	k_spinlock_key_t key = k_spin_lock(&lock);

	ccr = rtimer_count();
	ret = (total_cycles + (cached_icr - ccr)) & RTIMER_COUNT_MASK;

	k_spin_unlock(&lock, key);

	return ret;
}

void sys_clock_idle_exit(void)
{
	if (cached_icr == RTIMER_STOPPED) {
		cached_icr = CYCLES_PER_TICK;
		mec_hal_rtimer_stop_and_load(rtimer, cached_icr, RTIMER_START_VAL);
	}
}

void sys_clock_disable(void)
{
	mec_hal_rtimer_stop(rtimer);
}

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
/* Custom kernel busy wait API implementation using a 48MHz based
 * 32-bit basic timer divided down to 1 MHz. Basic timer configured
 * for count up, auto-reload, and no interrupt mode.
 */
void arch_busy_wait(uint32_t usec_to_wait)
{
	if (usec_to_wait == 0) {
		return;
	}

	uint32_t start = mec_hal_btimer_count(btimer);

	for (;;) {
		uint32_t curr = mec_hal_btimer_count(btimer);

		if ((curr - start) >= usec_to_wait) {
			break;
		}
	}
}

/* k_busy_wait parameter is the number of microseconds to wait.
 * Configure basic timer for 1 MHz (1 us tick) operation.
 */
static int config_custom_busy_wait(void)
{
	uint32_t bflags =
		(BIT(MEC5_BTIMER_CFG_FLAG_START_POS) | BIT(MEC5_BTIMER_CFG_FLAG_AUTO_RELOAD_POS) |
		 BIT(MEC5_BTIMER_CFG_FLAG_COUNT_UP_POS));
	uint32_t count = 0;

	mec_hal_btimer_init(btimer, MEC5_BTIMER_FDIV, count, bflags);

	return 0;
}

void soc_ktimer_pm_entry(bool is_deep_sleep)
{
	if (is_deep_sleep) {
		mec_hal_btimer_disable(btimer);
	}
}

void soc_ktimer_pm_exit(bool is_deep_sleep)
{
	if (is_deep_sleep) {
		mec_hal_btimer_enable(btimer);
	}
}
#else
void soc_ktimer_pm_entry(void)
{
}
void soc_ktimer_pm_exit(void)
{
}
#endif /* CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT */

static int sys_clock_driver_init(void)
{
	uint32_t rtmr_cfg = BIT(MEC_RTMR_CFG_EN_POS) | BIT(MEC_RTMR_CFG_IEN_POS);

	if (IS_ENABLED(CONFIG_SOC_MEC_DEBUG_AND_TRACING)) {
		rtmr_cfg |= BIT(MEC_RTMR_CFG_DBG_HALT_POS);
	}

#ifdef CONFIG_TICKLESS_KERNEL
	cached_icr = MAX_TICKS;
#endif

	mec_hal_rtimer_init(rtimer, rtmr_cfg, cached_icr);

	IRQ_CONNECT(RTIMER_NVIC_NO, RTIMER_NVIC_PRIO, mec5_ktimer_isr, 0, 0);
	irq_enable(RTIMER_NVIC_NO);

#ifdef CONFIG_ARCH_HAS_CUSTOM_BUSY_WAIT
	config_custom_busy_wait();
#endif

	mec_hal_rtimer_start(rtimer);
	while (!mec_hal_rtimer_is_counting(rtimer)) {
		;
	}

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
