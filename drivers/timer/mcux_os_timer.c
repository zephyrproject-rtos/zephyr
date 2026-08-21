/*
 * Copyright (c) 2021, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_os_timer

#include <limits.h>

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/timer/system_timer_lpm.h>
#include <zephyr/drivers/timer/nxp_os_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys/clock.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/state.h>
#include <zephyr/drivers/reset.h>
#include "fsl_ostimer.h"
#if !defined(CONFIG_SOC_FAMILY_MCXN) && !defined(CONFIG_SOC_FAMILY_MCXA)
#include "fsl_power.h"
#endif
#include <soc.h>

#define CYC_PER_TICK                                                                               \
	((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() /                                      \
		    (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define CYC_PER_US ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() / (uint64_t)USEC_PER_SEC))
#define MAX_CYC    INT_MAX

static OSTIMER_Type *base = (OSTIMER_Type *)DT_INST_REG_ADDR(0);
/* Total cycles of the timer compensated to include the time lost in "sleep/deep sleep" modes.
 * This maintains the timer count to account for the case if the OS Timer is reset in
 * certain deep sleep modes and the time elapsed when it is powered off.
 */
static uint64_t cyc_sys_compensated;
/*
 * Some SoCs power off the OS Timer domain in deep low-power states (e.g. RT700
 * Deep Sleep Retention), so it cannot wake the system. Two companion mechanisms
 * keep timekeeping and wakeup working across such a state:
 *   - Generic system-timer LPM companion (CONFIG_SYSTEM_TIMER_LPM_COMPANION_*
 *     + /chosen/zephyr,system-timer-companion); board-agnostic, preferred.
 *   - Legacy "deep-sleep-counter" phandle on the nxp,os-timer node, for boards
 *     whose companion counter does not fit the generic Counter alarm API.
 */
#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER) || \
	defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
#define MCUX_OS_TIMER_LPM_GENERIC 1
/* Indicates the low-power companion has been armed for the current sleep. */
static bool lpm_companion_armed;
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
/* Legacy deep-sleep-counter path (see comment above). */
#define MCUX_OS_TIMER_LPM_LEGACY 1
/* This is the counter device used when OS timer is not available in standby mode. */
static const struct device *counter_dev =
	DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(0, deep_sleep_counter));
/* Indicates if the counter is running. */
static bool counter_running;
static uint32_t counter_max_val;
#endif

/* Either companion mechanism enables the shared low-power timeout integration. */
#if defined(MCUX_OS_TIMER_LPM_GENERIC) || defined(MCUX_OS_TIMER_LPM_LEGACY)
#define MCUX_OS_TIMER_LPM 1
#endif
/* Indicates we received a call with ticks set to wait forever */
static bool wait_forever;
/* In case of counter overflow, track the remaining ticks left */
static uint32_t counter_remaining_ticks;

static uint64_t mcux_lpc_ostick_get_compensated_timer_value(void)
{
	return (OSTIMER_GetCurrentTimerValue(base) + cyc_sys_compensated);
}

/*
 * A free-running counter and a match register whose match is on equality: a
 * COMPARE_EXACT backend.
 *
 * The cycle domain is the counter plus the compensation the low-power paths
 * add for the time it was stopped, so the match register, which knows nothing
 * of that, is written with the compensation taken back off. That compensation
 * is shared with those paths, hence TIMER_CORE_COUNTER_NONATOMIC.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH 64
#define TIMER_CORE_COUNTER_NONATOMIC
#define TIMER_CORE_ALARM_MAX_CYCLES MAX_CYC

static inline uint64_t timer_driver_cycle_get(void)
{
	return mcux_lpc_ostick_get_compensated_timer_value();
}

/* The match write crosses into the OSTimer clock domain, and a match set nearer
 * than that window is passed before it takes effect. fsl_ostimer.c puts the
 * window at 11 us on a 1 MHz OSTimer, so 16 us covers it with margin. On the
 * parts clocked from the 32 kHz oscillator that is below one cycle, where the
 * floor stands in for the domain crossing itself.
 */
#define TIMER_CORE_ALARM_LEAD_CYCLES MAX(2U, 16U * CYC_PER_US)

/* Bound for the write-sync spin below, in iterations. Orders of magnitude
 * beyond the window, so it is reached only if the OSTimer is not clocked.
 */
#define MATCH_WR_SPIN_MAX 100000U

static inline void timer_driver_set_compare(uint64_t cycles)
{
	uint32_t spins = 0;

	OSTIMER_SetMatchValue(base, cycles - cyc_sys_compensated, NULL);

	/* Wait for the write to cross into the OSTimer clock domain, so the
	 * counter the core reads next cannot predate the match going live. This
	 * runs with the clock lock held, so it is bounded rather than able to
	 * hang the system on an unclocked timer.
	 */
	while ((base->OSEVENT_CTRL & OSTIMER_OSEVENT_CTRL_MATCH_WR_RDY_MASK) != 0U) {
		if (++spins > MATCH_WR_SPIN_MAX) {
			__ASSERT(false, "OSTimer match write did not complete");
			break;
		}
	}
}

#include "system_timer_generic.h"

void mcux_lpc_ostick_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();

	/* Clear interrupt flag by writing 1. */
	base->OSEVENT_CTRL &= ~OSTIMER_OSEVENT_CTRL_OSTIMER_INTENA_MASK;

	timer_core_announce_from(key);
}

#if defined(MCUX_OS_TIMER_LPM_GENERIC)

/* In a handoff-power-state the OS Timer is powered off and cannot wake the
 * system, so delegate wakeup to the generic companion and capture the OS Timer
 * value (lost across the state) for restoration on wakeup.
 */
static uint32_t mcux_lpc_ostick_set_counter_timeout(uint64_t timeout_us)
{
	/* Arm the system-timer low-power companion to wake the system. */
	z_sys_clock_lpm_enter(timeout_us);
	lpm_companion_armed = true;

	/* Capture the OS Timer value; it loses its state in a handoff-power-state. */
	cyc_sys_compensated += OSTIMER_GetCurrentTimerValue(base);

	/* The OS Timer is not a wakeup source for this state (the companion is), but
	 * it still has its previous match interrupt enabled. If that match is pending
	 * when the low-power entry executes WFI, WFI returns immediately and the state
	 * is never entered, so mask the match interrupt here. The kernel re-enables it
	 * after wakeup when it programs the next timeout.
	 */
	base->OSEVENT_CTRL &= ~OSTIMER_OSEVENT_CTRL_OSTIMER_INTENA_MASK;

	return 0;
}

static uint32_t mcux_lpc_ostick_compensate_system_timer(void)
{
	uint64_t slept_time_us;

	if (!lpm_companion_armed) {
		return 0;
	}
	lpm_companion_armed = false;

	/* Recover the time spent in low power from the companion counter. */
	slept_time_us = z_sys_clock_lpm_exit();
	cyc_sys_compensated += (uint64_t)CYC_PER_US * slept_time_us;

	/* The OS Timer lost its state in the handoff-power-state; reset it to a
	 * known state and reinitialize it.
	 */
	const struct reset_dt_spec reset = RESET_DT_SPEC_INST_GET_OR(0, {0});

	if (reset.dev != NULL) {
		reset_line_toggle_dt(&reset);
	}
	OSTIMER_Init(base);

	/* Announce the time slept to the kernel. */
	mcux_lpc_ostick_isr(NULL);

	return 0;
}

#elif defined(MCUX_OS_TIMER_LPM_LEGACY) /* legacy deep-sleep-counter path */

static struct counter_top_cfg top_cfg = {0};
static struct counter_alarm_cfg alarm_cfg = {0};

/* The OS Timer is disabled in certain low power modes and cannot wakeup the system
 * on timeout. This function will be called by the low power code to allow the
 * OS Timer to save off the count if needed and also start a wakeup counter
 * that would wakeup the system from deep power down modes.
 */
static uint32_t mcux_lpc_ostick_set_counter_timeout(int32_t curr_timeout)
{
	uint32_t ticks;

	if (counter_dev == NULL) {
		return 1;
	}

	/* Check if we should use the remaining ticks from a prior overflow */
	if (counter_remaining_ticks) {
		ticks = counter_remaining_ticks;
	} else {
		ticks = counter_us_to_ticks(counter_dev, curr_timeout);
		counter_remaining_ticks = ticks;
	}

	/* Check if the counter overflows */
	if (ticks > counter_max_val) {
		counter_remaining_ticks -= counter_max_val;
	} else {
		counter_remaining_ticks = 0;
	}
	ticks = CLAMP(ticks, 1, counter_max_val);

	top_cfg.ticks = ticks;
	alarm_cfg.ticks = ticks;
	/* short circuit conditional logic, if top value doesn't work, we try alarm */
	if (counter_set_top_value(counter_dev, &top_cfg) != 0 &&
	    counter_set_channel_alarm(counter_dev, 0, &alarm_cfg) != 0) {
		return 1;
	}

	/* Counter is set to wakeup the system after the requested time */
	if (counter_start(counter_dev) != 0) {
		return 1;
	}
	counter_running = true;

	if (IS_ENABLED(CONFIG_MCUX_OS_TIMER_PM_POWERED_OFF)) {
		/* Capture the current timer value for cases where it loses its state
		 * in low power modes.
		 */
		cyc_sys_compensated += OSTIMER_GetCurrentTimerValue(base);
	}

	return 0;
}

/* After exit from certain low power modes where the OS Timer was disabled, the
 * current tick value should be updated to account for the period when the OS Timer
 * was disabled. Also in certain cases, the OS Timer might lose its state and needs
 * to be reinitialized.
 */
static uint32_t mcux_lpc_ostick_compensate_system_timer(void)
{
	uint32_t slept_time_ticks;
	uint32_t slept_time_us;

	if (!counter_dev) {
		return 1;
	}

	if (!counter_running) {
		return 0;
	}

	counter_stop(counter_dev);
	counter_running = false;
	counter_get_value(counter_dev, &slept_time_ticks);

	if (!(counter_is_counting_up(counter_dev))) {
		slept_time_ticks = counter_get_top_value(counter_dev) - slept_time_ticks;
	}
	slept_time_us = counter_ticks_to_us(counter_dev, slept_time_ticks);
	cyc_sys_compensated += k_us_to_cyc_floor64(slept_time_us);

	if (IS_ENABLED(CONFIG_MCUX_OS_TIMER_PM_POWERED_OFF)) {
		/* Reset the OS Timer to a known state */
		const struct reset_dt_spec reset = RESET_DT_SPEC_INST_GET_OR(0, {0});

		if (reset.dev != NULL) {
			reset_line_toggle_dt(&reset);
		}
		/* Reactivate os_timer for cases where it loses its state */
		OSTIMER_Init(base);
	}

	/* Announce the time slept to the kernel*/
	mcux_lpc_ostick_isr(NULL);

	return 0;
}

#endif /* MCUX_OS_TIMER_LPM_GENERIC / MCUX_OS_TIMER_LPM_LEGACY */

#if defined(MCUX_OS_TIMER_LPM)
/* Whether the OS Timer stops keeping time in a power state and must hand off to
 * the companion. Driven by "handoff-power-states"; the legacy deep-sleep-counter
 * path (no such property) falls back to PM_STATE_STANDBY.
 */
#if DT_INST_NODE_HAS_PROP(0, handoff_power_states)
#define OS_TIMER_HANDOFF_STATE(node_id, prop, idx) \
	PM_STATE_DT_INIT(DT_PHANDLE_BY_IDX(node_id, prop, idx)),
static const enum pm_state os_timer_handoff_states[] = {
	DT_INST_FOREACH_PROP_ELEM(0, handoff_power_states, OS_TIMER_HANDOFF_STATE)
};
static bool os_timer_state_needs_handoff(enum pm_state state)
{
	ARRAY_FOR_EACH(os_timer_handoff_states, i) {
		if (os_timer_handoff_states[i] == state) {
			return true;
		}
	}
	return false;
}
#elif defined(MCUX_OS_TIMER_LPM_LEGACY)
/* Legacy deep-sleep-counter boards do not set handoff-power-states; keep the
 * historical behavior of handing off in PM_STATE_STANDBY.
 */
static inline bool os_timer_state_needs_handoff(enum pm_state state)
{
	return state == PM_STATE_STANDBY;
}
#else
static inline bool os_timer_state_needs_handoff(enum pm_state state)
{
	ARG_UNUSED(state);
	return false;
}
#endif /* handoff-power-states */

/* Poke-path helper: the SoC signals a low-power entry via a zero-tick idle
 * timeout. Hand off to the companion only when the next power state is one in
 * which the OS Timer stops keeping time (handoff-power-states).
 */
static void mcux_os_timer_set_lp_counter_timeout(void)
{
	uint64_t timeout;

	/* OS Timer may not be able to wakeup in certain low power modes.
	 * For these cases, we start a counter that can wakeup
	 * from low power modes.
	 */
	if (!os_timer_state_needs_handoff(pm_state_next_get(0)->state)) {
		return;
	}

	if (wait_forever) {
		timeout = UINT32_MAX;
	} else if (counter_remaining_ticks) {
		timeout = counter_remaining_ticks;
	} else {
		/* Check the amount of time left and switch to a counter
		 * that is active in this power mode.
		 */
		timeout = base->MATCH_L;
		timeout |= (uint64_t)(base->MATCH_H) << 32;
		timeout = OSTIMER_GrayToDecimal(timeout);
		timeout -= OSTIMER_GetCurrentTimerValue(base);
		/* Round up to the next tick boundary */
		timeout += (CYC_PER_TICK - 1);
		timeout = (timeout / CYC_PER_TICK) * CYC_PER_TICK;
		/* Convert to microseconds and round up to the next value */
		timeout = k_cyc_to_us_ceil64(timeout);
	}

	mcux_lpc_ostick_set_counter_timeout(timeout);
}
#else
#define mcux_os_timer_set_lp_counter_timeout(...) do { } while (0)
#endif /* MCUX_OS_TIMER_LPM */

bool z_nxp_os_timer_ignore_timer_wakeup(void)
{
	return (wait_forever || counter_remaining_ticks);
}

void sys_clock_no_timeout(void)
{
	/* Called from reprogram_next() and from sys_clock_idle_enter(), both
	 * with the clock lock already held.
	 */
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	/* Program the longest wait the hardware can hold. wait_forever records
	 * the state for the counter-overflow wakeup path, which the core knows
	 * nothing about, and is why this hook exists at all.
	 */
	timer_driver_set_compare(timer_driver_cycle_get() + MAX_CYC);
	counter_remaining_ticks = 0;
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
	wait_forever = true;
#endif
}

void sys_clock_idle_enter(uint32_t ticks)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
	/* We intercept idle entry with a 0 tick count when PM=y */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL) && (ticks == 0)) {
		mcux_os_timer_set_lp_counter_timeout();
		/* A low power counter has been started. No need to
		 * go further, simply return
		 */
		return;
	}
#endif
	if (ticks == SYS_CLOCK_IDLE_FOREVER) {
		/* Nothing to wake up for: same handling as on the running path,
		 * which also leaves the wait_forever bookkeeping set.
		 */
		sys_clock_no_timeout();
		return;
	}

	sys_clock_set_timeout(ticks, false);
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
	wait_forever = false;
#endif
	counter_remaining_ticks = 0;
}

void sys_clock_idle_exit(void)
{
#if defined(MCUX_OS_TIMER_LPM)
	/* Recover the tick for a handoff-power-state where the OS Timer was
	 * disabled. compensate() no-ops if the companion was not armed.
	 */
	if (os_timer_state_needs_handoff(pm_state_next_get(0)->state)) {
		mcux_lpc_ostick_compensate_system_timer();
	}
#endif /* MCUX_OS_TIMER_LPM */
}

static int sys_clock_driver_init(void)
{
	/* Initialize the OS timer, setting clock configuration. */
	OSTIMER_Init(base);

	/* Configure and enable event timer interrupt */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mcux_lpc_ostick_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	/* Seed the announce baseline and arm the first tick. */
	timer_core_init();

/* On some SoC's, OS Timer cannot wakeup from low power mode in standby modes */
#if defined(MCUX_OS_TIMER_LPM_LEGACY)
	counter_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PHANDLE(0, deep_sleep_counter));
	if (NULL != counter_dev) {
		counter_max_val = counter_get_max_top_value(counter_dev);
	}
#endif

#if (DT_INST_PROP(0, wakeup_source))
	NXP_ENABLE_WAKEUP_SIGNAL(DT_INST_IRQN(0));
#endif
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
