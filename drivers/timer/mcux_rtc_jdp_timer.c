/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rtc_jdp_timer

#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include "fsl_rtc.h"

/* Use the first RTC_JDP instance */
#define RTC_INST DT_INST(0, DT_DRV_COMPAT)

/* Keep math simple and avoid overflow in set_timeout computations */
#define COUNTER_MAX UINT32_MAX

static RTC_Type *base;
static struct k_spinlock lock;

/*
 * Stores the last hardware cycle count boundary that has been announced.
 * This is in hardware cycles (RTC counts), and advances only in multiples of
 * CYCLES_PER_TICK.
 */
static uint32_t announced_cycles;

/*
 * The RTC counter is 32-bit; synthesize a monotonic 64-bit cycle counter by
 * tracking wrap-around.
 */
static uint32_t cycle_last_32;
static uint64_t cycle_high_32;

#define CYCLES_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() \
				 / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

/*
 * RTCVAL is synchronized into the RTC clock domain and RTCCNT reads can lag.
 * If RTCVAL is programmed too close to the current count, the effective
 * compare value may take effect after the counter already passed it, and the
 * next compare interrupt will not occur until wrap-around.
 */
#ifdef CONFIG_MCUX_RTC_JDP_TIMER_SAFETY_WINDOW_CYCLES
#define SAFETY_WINDOW_CYCLES CONFIG_MCUX_RTC_JDP_TIMER_SAFETY_WINDOW_CYCLES
#else
#define SAFETY_WINDOW_CYCLES 32U
#endif

static uint32_t rtc_set_rtcval_safe(uint32_t target)
{
	uint32_t now;

	if (target <= MINIMUM_RTCVAL) {
		target = MINIMUM_RTCVAL + 1U;
	}

	/* Base the programming decision on a fresh counter read. */
	now = RTC_GetCountValue(base);
	/* Adjust target to be outside of the safety window. */
	if (unlikely((int32_t)(target - now) <= (int32_t)SAFETY_WINDOW_CYCLES)) {
		target = now + SAFETY_WINDOW_CYCLES + 1U;
	}

	RTC_SetRTCValue(base, target);
	return target;
}

static void mcux_rtc_jdp_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key;
	uint32_t status;
	uint32_t now;
	uint32_t tick_delta;
	const uint32_t cycles_per_tick = CYCLES_PER_TICK;

	status = RTC_GetInterruptFlags(base);

	if ((status & kRTC_RTCInterruptFlag) == 0U) {
		RTC_ClearInterruptFlags(base, status);
		return;
	}

	RTC_ClearInterruptFlags(base, kRTC_RTCInterruptFlag);

	key = k_spin_lock(&lock);
	now = RTC_GetCountValue(base);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Wrap-safe delta */
		tick_delta = (now - announced_cycles) / cycles_per_tick;
		if (tick_delta == 0U) {
			tick_delta = 1U;
		}
		announced_cycles += tick_delta * cycles_per_tick;
		k_spin_unlock(&lock, key);
		sys_clock_announce(tick_delta);
		return;
	}

	/* Tickful kernel: announce one tick and re-arm for the next tick */
	announced_cycles += cycles_per_tick;
	rtc_set_rtcval_safe(announced_cycles + cycles_per_tick);
	k_spin_unlock(&lock, key);
	sys_clock_announce(1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ARG_UNUSED(ticks);
		ARG_UNUSED(idle);
		return;
	}

	k_spinlock_key_t key;
	uint32_t now;
	uint32_t next;
	uint32_t adj;
	const uint32_t cycles_per_tick = CYCLES_PER_TICK;
	uint32_t max_ticks;

	if ((ticks == K_TICKS_FOREVER) && idle) {
		/* No known deadline: disable RTCVAL interrupt */
		RTC_DisableInterrupts(base, (uint32_t)kRTC_RTCInterruptEnable);
		return;
	}

	max_ticks = (uint32_t)(COUNTER_MAX / cycles_per_tick) - 1U;

	ticks = (ticks == K_TICKS_FOREVER) ? (int32_t)max_ticks : ticks;
	/* Subtract one because we round up to the next tick boundary */
	ticks = CLAMP((ticks - 1), 0, (int32_t)max_ticks);

	key = k_spin_lock(&lock);

	now = RTC_GetCountValue(base);
	adj = (now - announced_cycles) + (cycles_per_tick - 1U);

	/* Convert requested ticks to cycles, then round up to a tick boundary */
	next = (uint32_t)ticks * cycles_per_tick;
	next += adj;
	next = (next / cycles_per_tick) * cycles_per_tick;
	next += announced_cycles;

	RTC_EnableInterrupts(base, (uint32_t)kRTC_RTCInterruptEnable);
	rtc_set_rtcval_safe(next);

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0U;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now = RTC_GetCountValue(base);
	const uint32_t cycles_per_tick = CYCLES_PER_TICK;
	uint32_t elapsed = (now - announced_cycles) / cycles_per_tick;
	k_spin_unlock(&lock, key);

	return elapsed;
}

void sys_clock_idle_exit(void)
{
	/* RTC continues running in low power modes; no companion timer required. */
}

void sys_clock_disable(void)
{
	RTC_DisableInterrupts(base, (uint32_t)kRTC_RTCInterruptEnable);
	RTC_ClearInterruptFlags(base, kRTC_AllInterruptFlags);
	irq_disable(DT_IRQN(RTC_INST));
	NVIC_ClearPendingIRQ(DT_IRQN(RTC_INST));
}

uint32_t sys_clock_cycle_get_32(void)
{
	return RTC_GetCountValue(base);
}

uint64_t sys_clock_cycle_get_64(void)
{
	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now32 = RTC_GetCountValue(base);

	if (now32 < cycle_last_32) {
		cycle_high_32 += (1ULL << 32);
	}
	cycle_last_32 = now32;

	uint64_t now64 = cycle_high_32 + now32;
	k_spin_unlock(&lock, key);
	return now64;
}

static int sys_clock_driver_init(void)
{
	rtc_config_t rtc_config;
	uint32_t now;

	base = (RTC_Type *)DT_REG_ADDR(RTC_INST);

	const uint32_t cycles_per_tick = CYCLES_PER_TICK;

	RTC_GetDefaultConfig(&rtc_config);
	rtc_config.clockSource = (rtc_clock_source_t)DT_PROP(RTC_INST, clock_source);

	/* Map numeric prescaler (1/32/512/16384) to SDK enum */
	if (DT_PROP(RTC_INST, prescaler) == 1) {
		rtc_config.clockDivide = kRTC_ClockDivide1;
	} else if (DT_PROP(RTC_INST, prescaler) == 32) {
		rtc_config.clockDivide = kRTC_ClockDivide32;
	} else if (DT_PROP(RTC_INST, prescaler) == 512) {
		rtc_config.clockDivide = kRTC_ClockDivide512;
	} else {
		rtc_config.clockDivide = kRTC_ClockDivide16384;
	}

	RTC_Init(base, &rtc_config);

	/*
	 * Enable RTC before reading the counter. Note the SDK documents CNTEN as an
	 * async reset of RTC/API logic.
	 */
	RTC_EnableRTC(base);
	RTC_ClearInterruptFlags(base, kRTC_AllInterruptFlags);

	now = RTC_GetCountValue(base);
	announced_cycles = now - (now % cycles_per_tick);
	cycle_last_32 = now;
	cycle_high_32 = 0U;

	IRQ_CONNECT(DT_IRQN(RTC_INST), DT_IRQ(RTC_INST, priority),
		    mcux_rtc_jdp_isr, NULL, 0);
	irq_enable(DT_IRQN(RTC_INST));

	/*
	 * Always arm an initial tick interrupt so the kernel's timeout machinery
	 * starts making progress immediately. Tickless kernels are allowed spurious
	 * announcements.
	 */
	rtc_set_rtcval_safe(announced_cycles + cycles_per_tick);
	RTC_EnableInterrupts(base, (uint32_t)kRTC_RTCInterruptEnable);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
