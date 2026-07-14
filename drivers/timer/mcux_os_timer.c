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
#include <zephyr/sys_clock.h>
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
#define MAX_TICKS  ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY  CONFIG_MCUX_OS_TIMER_MIN_DELAY

static struct k_spinlock lock;
static uint64_t last_count;
static OSTIMER_Type *base = (OSTIMER_Type *)DT_INST_REG_ADDR(0);
/* Total cycles of the timer compensated to include the time lost in "sleep/deep sleep" modes.
 * This maintains the timer count to account for the case if the OS Timer is reset in
 * certain deep sleep modes and the time elapsed when it is powered off.
 */
static uint64_t cyc_sys_compensated;
/*
 * Some SoCs collapse the OS Timer power domain in deep low-power states (e.g.
 * RT700 Deep Sleep Retention), so the OS Timer cannot wake the system on its
 * own. Two low-power companion mechanisms are supported:
 *
 *   - The generic system-timer low-power companion framework, selected through
 *     CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER/HOOKS and the
 *     /chosen/zephyr,system-timer-companion property. This is board agnostic
 *     and is the preferred mechanism for new boards.
 *   - The deprecated NXP "deep-sleep-counter" phandle path, kept for existing
 *     platforms (e.g. RW61x) that wire a Counter API device directly on the
 *     nxp,os-timer node and define a "standby" power state. New boards must
 *     use the generic companion above.
 */
#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_COUNTER) || \
	defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
#define MCUX_OS_TIMER_LPM_GENERIC 1
/* Indicates the low-power companion has been armed for the current sleep. */
static bool lpm_companion_armed;
#elif DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(standby)) && CONFIG_PM
/* Deprecated: use the generic system-timer companion (see comment above). */
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

void mcux_os_timer_set_next_tick_match(void)
{
	uint64_t adjustment = CYC_PER_TICK < MIN_DELAY ? 2 * CYC_PER_TICK : CYC_PER_TICK;
	uint64_t next_tick_cycles_match = last_count + adjustment;

	OSTIMER_SetMatchValue(base, next_tick_cycles_match, NULL);
}

static uint32_t mcux_os_timer_calc_elapsed_ticks(uint64_t current_cycles)
{
	uint64_t elapsed_cycles = current_cycles - last_count;
	uint32_t elapsed_ticks = (uint32_t)elapsed_cycles / CYC_PER_TICK;
	return elapsed_ticks;
}

void mcux_lpc_ostick_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* Clear interrupt flag by writing 1. */
	base->OSEVENT_CTRL &= ~OSTIMER_OSEVENT_CTRL_OSTIMER_INTENA_MASK;

	uint64_t now = mcux_lpc_ostick_get_compensated_timer_value();
	uint32_t elapsed_ticks = mcux_os_timer_calc_elapsed_ticks(now);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/*
		 * Advance in whole ticks to avoid accumulating sub-tick latency in
		 * last_count (which would otherwise show up as long-term drift).
		 */
		last_count += (uint64_t)elapsed_ticks * CYC_PER_TICK;
	} else {
		last_count = now;
		mcux_os_timer_set_next_tick_match();
	}

	k_spin_unlock(&lock, key);

	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? elapsed_ticks : 1);
}

#if defined(MCUX_OS_TIMER_LPM_GENERIC)

/* The OS Timer is disabled in certain low power modes and cannot wakeup the
 * system on timeout. Delegate low-power timekeeping to the generic system-timer
 * companion, which arms a Counter API device that stays alive across the mode.
 * A handoff-power-state means the OS Timer is powered off there, so its value is
 * captured now and restored on wakeup unconditionally.
 */
static uint32_t mcux_lpc_ostick_set_counter_timeout(uint64_t timeout_us)
{
	/* Arm the system-timer low-power companion to wake the system. */
	z_sys_clock_lpm_enter(timeout_us);
	lpm_companion_armed = true;

	/* Capture the OS Timer value; it loses its state in a handoff-power-state. */
	cyc_sys_compensated += OSTIMER_GetCurrentTimerValue(base);

	/* The companion now owns wakeup. Silence the OS Timer match interrupt so a
	 * stale match cannot wake the system early. It is re-armed on wakeup, once
	 * the timer is reinitialized and the kernel programs the next timeout.
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

#elif defined(MCUX_OS_TIMER_LPM_LEGACY) /* deprecated deep-sleep-counter path */

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
	cyc_sys_compensated += CYC_PER_US * slept_time_us;

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
/* Power states in which the OS Timer stops keeping time, so timekeeping must be
 * handed off to the low-power companion. Taken from the "handoff-power-states"
 * devicetree property; empty (never hand off) when the property is absent.
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
		/* Convert to microseconds and round up to the next value */
		timeout = (((timeout / CYC_PER_TICK) * CYC_PER_TICK) * CYC_PER_US);
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

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Only for tickless kernel system */
		return;
	}

#if defined(MCUX_OS_TIMER_LPM)
	/* We intercept calls from idle with a 0 tick count when PM=y */
	if (idle && (ticks == 0)) {
		mcux_os_timer_set_lp_counter_timeout();
		/* A low power counter has been started. No need to
		 * go further, simply return
		 */
		return;
	}
	/* When using a counter for certain low power modes, set this flag when the requested
	 * delay is forever. This is to keep track of wakeup sources in case of counter overflows.
	 */
	wait_forever = (ticks == SYS_CLOCK_MAX_WAIT);
#else
	ARG_UNUSED(idle);
#endif /* MCUX_OS_TIMER_LPM */
	ticks = CLAMP(ticks, 1, MAX_TICKS) - 1;

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = mcux_lpc_ostick_get_compensated_timer_value();
	uint32_t adj, cyc = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary. */
	adj = (uint32_t)(now - last_count) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;

	if ((int32_t)(cyc + last_count - now) < MIN_DELAY) {
		cyc += CYC_PER_TICK;
	}

	OSTIMER_SetMatchValue(base, cyc + last_count - cyc_sys_compensated, NULL);

	counter_remaining_ticks = 0;

	k_spin_unlock(&lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Always return 0 for tickful kernel system */
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint64_t now = mcux_lpc_ostick_get_compensated_timer_value();
	uint32_t elapsed_ticks = mcux_os_timer_calc_elapsed_ticks(now);

	k_spin_unlock(&lock, key);

	return elapsed_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)mcux_lpc_ostick_get_compensated_timer_value();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return mcux_lpc_ostick_get_compensated_timer_value();
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

	last_count = mcux_lpc_ostick_get_compensated_timer_value();
	OSTIMER_SetMatchValue(base, last_count + CYC_PER_TICK, NULL);

	/* Configure and enable event timer interrupt */
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), mcux_lpc_ostick_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

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
