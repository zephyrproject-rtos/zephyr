/*
 * Copyright (c) 2020 Thorvald Natvig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <power/power.h>
#include <drivers/timer/system_timer.h>
#include <sl_sleeptimer.h>
#include <em_cmu.h>

#define TIMER_TO_TICKS(x) ((uint64_t)(x) * CONFIG_SYS_CLOCK_TICKS_PER_SEC / frequency)
#define TICKS_TO_TIMER(x) ((uint64_t)(x) * frequency / CONFIG_SYS_CLOCK_TICKS_PER_SEC)
#define CYC_PER_TICK (frequency / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#if defined(CONFIG_GECKO_SLEEPTIMER_ULFRCO)
#define CLOCK_SOURCE cmuSelect_ULFRCO
#elif defined(CONFIG_GECKO_SLEEPTIMER_LFRCO)
#define CLOCK_SOURCE cmuSelect_LFRCO
#elif defined(CONFIG_GECKO_SLEEPTIMER_LFXO)
#define CLOCK_SOURCE cmuSelect_LFXO
#else
#error No low-frequency clock source selected. Ensure CONFIG_SYS_CLOCK_TICKS_PER_SEC is less than 32768 (1000 for ULFRCO) and CONFIG_SYS_PM_STATE_LOCK is enabled if using sleep states.
#endif

/* ULFRCO is always on, but LFRCO and LFXO is only on in EM0-EM2 */
#if defined(CONFIG_SYS_PM_STATE_LOCK) && !defined(CONFIG_GECKO_SLEEPTIMER_ULFRCO)
#define NEED_EM3_BLOCK
#endif

static sl_sleeptimer_timer_handle_t timer_handle;

static uint64_t last_count;
static uint64_t tick_count;
static uint32_t frequency;

#ifdef NEED_EM3_BLOCK
static bool em3_disabled;

#ifdef CONFIG_TICKLESS_KERNEL
static void unblock_em3(void)
{
	if (em3_disabled) {
		sys_pm_ctrl_enable_state(SYS_POWER_STATE_SLEEP_3);
		em3_disabled = false;
	}
}
#endif

static void block_em3(void)
{
	if (!em3_disabled) {
		sys_pm_ctrl_disable_state(SYS_POWER_STATE_SLEEP_3);
		em3_disabled = true;
	}
}
#else /* NEED_EM3_BLOCK */
#define unblock_em3() do { } while (0)
#define block_em3() do { } while (0)
#endif /* NEED_EM3_BLOCK */

#ifdef CONFIG_GECKO_SLEEPTIMER_RTCC
void RTCC_IRQHandler(void);
#else
void RTC_IRQHandler(void);
#endif

static void gecko_sleeptimer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
	ARG_UNUSED(handle);
	ARG_UNUSED(data);

	uint64_t count = sl_sleeptimer_get_tick_count64();
	uint32_t dticks = TIMER_TO_TICKS(count - last_count);

	tick_count += dticks;
	last_count = TICKS_TO_TIMER(tick_count - 1);
	z_clock_announce(dticks);
}

void gecko_sleeptimer_isr(const void *arg)
{
	ARG_UNUSED(arg);

#ifdef CONFIG_GECKO_SLEEPTIMER_RTCC
	RTCC_IRQHandler();
#else
	RTC_IRQHandler();
#endif
}

int z_clock_driver_init(const struct device *device)
{
	ARG_UNUSED(device);

	sl_status_t status;

#ifndef CONFIG_GECKO_SLEEPTIMER_ULFRCO
	CMU_OscillatorEnable(CLOCK_SOURCE, true, true);
#endif

#if defined(cmuClock_CORELE)
	CMU_ClockEnable(cmuClock_CORELE, true);
#endif

#ifdef CONFIG_GECKO_SLEEPTIMER_RTCC
#if defined(CMU_LFECLKEN0_RTCC)
	CMU_ClockSelectSet(cmuClock_LFE, CLOCK_SOURCE);
#elif defined(_SILICON_LABS_32B_SERIES_2)
	CMU_ClockSelectSet(cmuClock_RTCC, CLOCK_SOURCE);
#else
	CMU_ClockSelectSet(cmuClock_LFA, CLOCK_SOURCE);
#endif
	CMU_ClockEnable(cmuClock_RTCC, true);
	IRQ_CONNECT(RTCC_IRQn, 1, gecko_sleeptimer_isr, 0, 0);
	irq_enable(RTCC_IRQn);
#else
	CMU_ClockSelectSet(cmuClock_LFA, CLOCK_SOURCE);
	CMU_ClockEnable(cmuClock_RTC, true);
	IRQ_CONNECT(RTC_IRQn, 1, gecko_sleeptimer_isr, 0, 0);
	irq_enable(RTC_IRQn);
#endif

	status = sl_sleeptimer_init();
	frequency = sl_sleeptimer_get_timer_frequency();
	last_count = sl_sleeptimer_get_tick_count64();

#ifndef CONFIG_TICKLESS_KERNEL
	status = sl_sleeptimer_start_periodic_timer(&timer_handle,
						    CYC_PER_TICK,
						    gecko_sleeptimer_callback,
						    NULL,
						    0,
						    0);
	__ASSERT(status == SL_STATUS_OK, "gecko_sleeptimer failed to start periodic timer: %d", status);
	block_em3();
#endif

	return 0;
}

void z_clock_set_timeout(int32_t ticks, bool idle)
{
#ifdef CONFIG_TICKLESS_KERNEL
	ARG_UNUSED(idle);

	if (ticks == K_TICKS_FOREVER || ticks == INT_MAX) {
		sl_sleeptimer_stop_timer(&timer_handle);
		unblock_em3();
	} else {
		sl_status_t status;
		uint64_t counts = MAX(1, TICKS_TO_TIMER(ticks));
		block_em3();

		status = sl_sleeptimer_restart_timer(&timer_handle,
						     counts,
						     gecko_sleeptimer_callback,
						     (void *)NULL,
						     0,
						     0);
		__ASSERT(status == SL_STATUS_OK, "gecko_sleeptimer failed to start timer: %d", status);
	}
#else
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);
#endif
}

uint32_t z_clock_elapsed(void)
{
#ifdef CONFIG_TICKLESS_KERNEL
	return TIMER_TO_TICKS(sl_sleeptimer_get_tick_count64() - last_count);
#else
	return 0;
#endif
}

uint32_t z_timer_cycle_get_32(void)
{
	return sl_sleeptimer_get_tick_count64() * sys_clock_hw_cycles_per_sec() / frequency;
}
