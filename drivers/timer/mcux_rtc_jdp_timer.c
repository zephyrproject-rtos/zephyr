/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>
#include <fsl_rtc.h>

#define RTC_NODE DT_CHOSEN(zephyr_system_timer)

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an nxp,rtc-jdp node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(RTC_NODE, nxp_rtc_jdp),
	     "zephyr,system-timer must point to an nxp,rtc-jdp compatible node");

#define RTC_JDP_BASE  ((RTC_Type *)DT_REG_ADDR(RTC_NODE))
#define RTC_IRQN      DT_IRQN(RTC_NODE)
#define RTC_IRQ_PRIO  DT_IRQ(RTC_NODE, priority)
#define RTC_CLK_SRC   DT_PROP(RTC_NODE, clock_source)
#define RTC_PRESCALER DT_PROP(RTC_NODE, prescaler)
#define RTC_CLK_FREQ  DT_PROP(RTC_NODE, clock_frequency)

/*
 * The RTC counter rate (clock-frequency / prescaler) is published at runtime
 * through z_clock_hw_cycles_per_sec (the driver selects
 * TIMER_READS_ITS_FREQUENCY_AT_RUNTIME), so the devicetree prescaler is the
 * single source of truth for the tick rate.
 */
extern unsigned int z_clock_hw_cycles_per_sec;

/* Map the numeric prescaler (1/32/512/16384) to the SDK clock-divide enum */
#define RTC_JDP_DIV_ENUM						\
	(RTC_PRESCALER == 1     ? kRTC_ClockDivide1			\
	 : RTC_PRESCALER == 32  ? kRTC_ClockDivide32			\
	 : RTC_PRESCALER == 512 ? kRTC_ClockDivide512			\
				: kRTC_ClockDivide16384)

/*
 * Bounded spin waiting for an in-flight RTCVAL synchronization to finish (see
 * rtc_jdp_set_compare). A sync takes a few RTC cycles (~100 us at 32 kHz); this
 * bound is far larger and only prevents an unbounded spin on a stuck peripheral.
 */
#define RTC_JDP_SYNC_SPIN 100000U

/*
 * Cycles the compare must stay ahead of the current count when programmed.
 * RTCVAL is an equality compare that synchronizes into the slow RTC clock
 * domain after the CPU samples the count; this margin absorbs that sync
 * latency so the counter cannot reach the compare before it is armed.
 */
#define RTC_JDP_COMPARE_MARGIN (2U * MINIMUM_RTCVAL)

/* Runtime cycle-domain values derived from the RTC rate in init. */
static uint32_t cycles_per_tick;
static uint32_t cycles_max;

/* Absolute RTC count at the last announced tick boundary. */
static uint32_t last_count;
/* Ticks last reported by sys_clock_elapsed(), consumed by sys_clock_set_timeout(). */
static uint32_t last_elapsed;

static inline void rtc_jdp_wait_inv_clear(void)
{
	for (uint32_t i = 0U; i < RTC_JDP_SYNC_SPIN; i++) {
		if ((RTC_GetStatusFlags(RTC_JDP_BASE) & (uint32_t)kRTC_InvalidRTCFlag) == 0U) {
			break;
		}
	}
}

/*
 * Program the next compare (RTCVAL). RTCVAL is a one-shot equality compare
 * against the free-running 32-bit counter, so it must be programmed strictly
 * ahead of the count, otherwise the match is missed and only recurs after a
 * full 32-bit wrap (~36 h at 32 kHz).
 */
static void rtc_jdp_set_compare(uint32_t compare)
{
	uint32_t bump = RTC_JDP_COMPARE_MARGIN + 1U;
	uint32_t now;

	for (;;) {
		/* RTCVAL must be greater than MINIMUM_RTCVAL. */
		if (compare <= MINIMUM_RTCVAL) {
			compare = MINIMUM_RTCVAL + 1U;
		}

		/*
		 * An RTCVAL write starts a synchronization into the RTC clock
		 * domain and sets INV_RTC; while INV_RTC is set the hardware
		 * ignores further RTCVAL writes (RM ch. 66 "Invalid RTC write").
		 * Wait for a previous write to synchronize before programming.
		 */
		rtc_jdp_wait_inv_clear();
		RTC_ClearInterruptFlags(RTC_JDP_BASE, kRTC_RTCInterruptFlag);
		RTC_SetRTCValue(RTC_JDP_BASE, compare);

		/*
		 * Re-read the counter and, like the retry in mcux_stm_timer.c,
		 * bump the compare forward if the count has already reached (or is
		 * within the sync margin of) it, so the equality match is not lost
		 * to the sync latency sampled above.
		 */
		now = RTC_GetCountValue(RTC_JDP_BASE);
		if (likely((int32_t)(compare - now) > (int32_t)RTC_JDP_COMPARE_MARGIN)) {
			break;
		}

		compare = now + bump;
		bump *= 2U;
	}
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	uint32_t cycles;

	if (IS_ENABLED(CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE) && ticks == SYS_CLOCK_MAX_WAIT) {
		/*
		 * No pending timeout and no future timer interrupt required:
		 * wait as long as the hardware allows.
		 */
		cycles = cycles_max;
	} else {
		uint64_t wait_ticks = (uint64_t)last_elapsed + (uint64_t)ticks;
		uint64_t wait_cycles = wait_ticks * (uint64_t)cycles_per_tick;

		cycles = (wait_cycles > cycles_max) ? cycles_max : (uint32_t)wait_cycles;
	}

	rtc_jdp_set_compare(last_count + cycles);
}

uint32_t sys_clock_elapsed(void)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	uint32_t now = RTC_GetCountValue(RTC_JDP_BASE);
	uint32_t delta_ticks = (now - last_count) / cycles_per_tick;

	last_elapsed = delta_ticks;
	return delta_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* The free-running 32-bit RTC counter is the hardware cycle counter. */
	return RTC_GetCountValue(RTC_JDP_BASE);
}

void sys_clock_disable(void)
{
	/*
	 * Terminal teardown only (e.g. before reboot/power-off). Disabling the
	 * counter resets the RTC logic, so neither the tick nor the cycle counter
	 * resumes afterwards.
	 */
	RTC_DisableInterrupts(RTC_JDP_BASE, kRTC_RTCInterruptEnable);
	RTC_DisableRTC(RTC_JDP_BASE);
	irq_disable(RTC_IRQN);
}

static void mcux_rtc_jdp_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();
	uint32_t now = RTC_GetCountValue(RTC_JDP_BASE);
	uint32_t delta_ticks = (now - last_count) / cycles_per_tick;

	RTC_ClearInterruptFlags(RTC_JDP_BASE, kRTC_RTCInterruptFlag);

	last_count += delta_ticks * cycles_per_tick;
	last_elapsed = 0U;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* RTCVAL is one-shot; arm the next periodic tick. */
		rtc_jdp_set_compare(last_count + cycles_per_tick);
	}

	sys_clock_announce_locked((int32_t)delta_ticks, key);
}

static int sys_clock_driver_init(void)
{
	rtc_config_t config;
	uint32_t cycle_rate = RTC_CLK_FREQ / RTC_PRESCALER;

	/*
	 * The RTC rate must divide the tick rate exactly; this also rejects a
	 * rate below the tick rate, which would make cycles_per_tick zero and
	 * fault the divisions below.
	 */
	if ((cycle_rate == 0U) || ((cycle_rate % CONFIG_SYS_CLOCK_TICKS_PER_SEC) != 0U)) {
		return -EINVAL;
	}

	cycles_per_tick = cycle_rate / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	if (cycles_per_tick <= RTC_JDP_COMPARE_MARGIN) {
		/* Tick period too short for the RTC compare-sync margin. */
		return -EINVAL;
	}

	z_clock_hw_cycles_per_sec = cycle_rate;
	/*
	 * Schedule at most half the counter range ahead; a compare further out
	 * would wrap past the 32-bit counter and be indistinguishable from a
	 * value already in the past under wrap-around arithmetic, stalling the
	 * tick. Keep it tick-aligned. Mirrors mcux_stm_timer.c.
	 */
	cycles_max = (INT32_MAX / cycles_per_tick) * cycles_per_tick;

	RTC_GetDefaultConfig(&config);
	config.clockSource = (rtc_clock_source_t)RTC_CLK_SRC;
	config.clockDivide = RTC_JDP_DIV_ENUM;
	RTC_Init(RTC_JDP_BASE, &config);
	RTC_ClearInterruptFlags(RTC_JDP_BASE, kRTC_AllInterruptFlags);

	IRQ_CONNECT(RTC_IRQN, RTC_IRQ_PRIO, mcux_rtc_jdp_timer_isr, NULL, 0);
	irq_enable(RTC_IRQN);

	/*
	 * Program the first compare and the match interrupt, then enable the
	 * counter last so it starts from a fully configured state. Enabling
	 * resets the RTC logic and starts the count from zero, committing the
	 * RTCVAL written here (RM ch. 66).
	 */
	RTC_SetRTCValue(RTC_JDP_BASE, cycles_per_tick);
	RTC_EnableInterrupts(RTC_JDP_BASE, kRTC_RTCInterruptEnable);
	RTC_EnableRTC(RTC_JDP_BASE);

	last_count = 0U;
	last_elapsed = 0U;

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
