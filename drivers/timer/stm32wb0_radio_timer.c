/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <cmsis_core.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include "stm32wb0x_hal_radio_timer.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(radio_timer_driver);

/* Max HS startup time expressed in system time (1953 us / 2.4414 us) */
#define MAX_HS_STARTUP_TIME	DT_PROP(DT_NODELABEL(radio_timer), max_hs_startup_time)
#define BLE_WKUP_PRIO		0
#define CPU_WKUP_PRIO		1
#define RADIO_TIMER_ERROR_PRIO	3

#define MULT64_THR_FREQ		806
#define TIMER_WRAPPING_MARGIN	4096
#define MAX_ALLOWED_DELAY	(UINT32_MAX - TIMER_WRAPPING_MARGIN)

/* The CPU wakeup comparator only considers the 28 most significant bits of
 * the compare value: matches happen on whole slow-clock periods (16 MTU).
 * Deadlines are biased up by one slow-clock period so this truncation can
 * never make the interrupt land before the requested tick boundary.
 */
#define WAKEUP_COMPARE_UNIT	16

/* Never program a compare value closer than this to the live counter. The
 * wakeup block runs in the slow-clock domain, so a compare value the counter
 * passes before the write has taken effect is not matched until the counter
 * fully wraps (2^32 MTU, about 2 hours 16 minutes at 32768 Hz).
 */
#define MIN_ALLOWED_DELAY	(4 * WAKEUP_COMPARE_UNIT)

/* System time units (STU) per kernel tick. STU is the domain of
 * HAL_RADIO_TIMER_GetCurrentSysTime() and announced_cycles.
 */
#define CYC_PER_TICK	(sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* Largest tick count we will program. Bounded by INT32_MAX (ticks and
 * sys_clock_announce() are int32_t) so the value stays a positive int32_t, and
 * the resulting STU span (MAX_TICKS * CYC_PER_TICK) still fits the 32-bit input
 * of blue_unit_conversion(). The MTU result is additionally clamped to
 * MAX_ALLOWED_DELAY below.
 */
#define MAX_TICKS	(INT32_MAX / CYC_PER_TICK)

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(clk_lsi), disabled),
	     "LSI is not supported yet");

/* This value is only valid for the LSE with a frequency of 32768 Hz.
 * The implementation for the LSI will be done in the future.
 */
static const uint32_t calibration_data_freq1 = 0x0028F5C2;

/* Translate STU to MTU and vice versa. It is implemented by using integer operations. */
uint32_t blue_unit_conversion(uint32_t time, uint32_t period_freq, uint32_t thr);

static uint64_t announced_cycles;
static uint32_t last_cpu_wakeup_time;
/* Ticks observed by the last sys_clock_elapsed(); reset to 0 at announce. */
static uint32_t last_elapsed;

static void radio_timer_error_isr(void *args)
{
	volatile uint32_t debug_cmd;

	ARG_UNUSED(args);

	BLUE->DEBUGCMDREG |= 1;
	/* If the device is configured with CLK_SYS = 64MHz
	 * and BLE clock = 16MHz, a register read is necessary
	 * to ensure interrupt register is properly cleared
	 * due to AHB down converter latency
	 */
	debug_cmd = BLUE->DEBUGCMDREG;
	LOG_ERR("Timer error");
}

static void radio_timer_cpu_wkup_isr(void *args)
{
	int32_t dticks;
	uint64_t diff_cycles;

	ARG_UNUSED(args);

	last_cpu_wakeup_time = LL_RADIO_TIMER_GetAbsoluteTime(WAKEUP);

	/* Clear the interrupt */
	LL_RADIO_TIMER_ClearFlag_CPUWakeup(WAKEUP);

	/* Read back the register to ensure that the previous
	 * write operation is completed.
	 */
	LL_RADIO_TIMER_IsActiveFlag_CPUWakeup(WAKEUP);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Read the current counter (not the scheduled wakeup time) and
		 * floor to whole ticks, so any latency between the wakeup event
		 * and this handler is folded into the announce. Advance
		 * announced_cycles by the exact tick-aligned amount, preserving
		 * the sub-tick remainder for the next announce.
		 */
		diff_cycles = HAL_RADIO_TIMER_GetCurrentSysTime() - announced_cycles;
		dticks = (int32_t)(diff_cycles / CYC_PER_TICK);
		announced_cycles += (uint64_t)dticks * CYC_PER_TICK;
		last_elapsed = 0;
		sys_clock_announce(dticks);
	} else {
		sys_clock_announce(1);
	}
}

#if defined(CONFIG_SOC_STM32WB06XX) || defined(CONFIG_SOC_STM32WB07XX)
static void radio_timer_txrx_wkup_isr(void *args)
{
	ARG_UNUSED(args);

	/* The callback body will be properly implemented in the future
	 * while providing PM support for WB06/WB07.
	 */

	/* Clear the interrupt */
	LL_RADIO_TIMER_ClearFlag_BLEWakeup(WAKEUP);

	/* Read back the register to ensure that the previous
	 * write operation is completed.
	 */
	LL_RADIO_TIMER_IsActiveFlag_BLEWakeup(WAKEUP);
}
#endif /* CONFIG_SOC_STM32WB06XX || CONFIG_SOC_STM32WB07XX */

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t current_time, delay, delta_stu;
		uint64_t target, now_stu;
		uint32_t key;

		/* K_TICKS_FOREVER parks the wakeup as far out as the hardware
		 * allows (the deadline is capped by MAX_ALLOWED_DELAY below),
		 * which also keeps the counter wrap-protected.
		 */
		ticks = (ticks == K_TICKS_FOREVER) ? (int32_t)MAX_TICKS
						   : CLAMP(ticks, 1, (int32_t)MAX_TICKS);

		key = irq_lock();

		/* Absolute, tick-aligned deadline measured from the last
		 * announce: announced_cycles is tick-aligned and last_elapsed is
		 * the tick count already reported via sys_clock_elapsed(), so the
		 * fire lands on the requested tick boundary regardless of the
		 * sub-tick offset of the current counter. A target in the past
		 * collapses to the minimum delay and is recovered by the handler
		 * reading the live counter.
		 */
		target = announced_cycles +
			 (uint64_t)(last_elapsed + (uint32_t)ticks) * CYC_PER_TICK;
		now_stu = HAL_RADIO_TIMER_GetCurrentSysTime();
		delta_stu = (target > now_stu) ? (uint32_t)(target - now_stu) : 0;

		/* Bias the deadline one compare unit late so the truncated
		 * compare never fires before the tick boundary: an early fire
		 * would announce 0 ticks and force an immediate re-arm.
		 */
		delay = blue_unit_conversion(delta_stu, calibration_data_freq1, MULT64_THR_FREQ);
		delay += WAKEUP_COMPARE_UNIT;
		delay = CLAMP(delay, MIN_ALLOWED_DELAY, MAX_ALLOWED_DELAY);

		/* Due to a hardware limitation, the radio timer wake-up time
		 * must not be updated until at least 16 Machine Time Units
		 * have elapsed since the interrupt was triggered; otherwise,
		 * the next wake-up will only occur after the timer wraps around.
		 *
		 * This counter read must stay immediately before the compare
		 * write: delay was computed against the (earlier) system time
		 * read, so anchoring it to a later counter value can only move
		 * the deadline later, never closer than MIN_ALLOWED_DELAY.
		 */
		do {
			current_time = LL_RADIO_TIMER_GetAbsoluteTime(WAKEUP);
		} while ((current_time - last_cpu_wakeup_time) < 16U);

		LL_RADIO_TIMER_SetCPUWakeupTime(WAKEUP, current_time + delay);
		LL_RADIO_TIMER_EnableCPUWakeupTimer(WAKEUP);
		irq_unlock(key);
	}
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	/* Number of whole ticks elapsed since the last announce (floor): a
	 * partially-elapsed tick must not be reported, otherwise uptime runs
	 * ahead and measured intervals come up short.
	 */
	last_elapsed = (uint32_t)((HAL_RADIO_TIMER_GetCurrentSysTime() - announced_cycles)
				  / CYC_PER_TICK);
	return last_elapsed;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return sys_clock_cycle_get_64() & UINT32_MAX;
}

uint64_t sys_clock_cycle_get_64(void)
{
	return HAL_RADIO_TIMER_GetCurrentSysTime();
}

void sys_clock_disable(void)
{
#if defined(CONFIG_SOC_STM32WB06XX) || defined(CONFIG_SOC_STM32WB07XX)
	irq_disable(RADIO_TIMER_TXRX_WKUP_IRQn);
#endif /* CONFIG_SOC_STM32WB06XX || CONFIG_SOC_STM32WB07XX */
	irq_disable(RADIO_TIMER_CPU_WKUP_IRQn);
	irq_disable(RADIO_TIMER_ERROR_IRQn);
	__HAL_RCC_RADIO_CLK_DISABLE();
}

void sys_clock_idle_exit(void)
{
#if defined(CONFIG_SOC_STM32WB06XX) || defined(CONFIG_SOC_STM32WB07XX)
	irq_enable(RADIO_TIMER_TXRX_WKUP_IRQn);
#endif /* CONFIG_SOC_STM32WB06XX || CONFIG_SOC_STM32WB07XX */

	irq_enable(RADIO_TIMER_CPU_WKUP_IRQn);
	irq_enable(RADIO_TIMER_ERROR_IRQn);
}

static int sys_clock_driver_init(void)
{
	RADIO_TIMER_InitTypeDef vtimer_init_struct = {MAX_HS_STARTUP_TIME, 0, 0};

#if defined(CONFIG_SOC_STM32WB06XX) || defined(CONFIG_SOC_STM32WB07XX)
	IRQ_CONNECT(RADIO_TIMER_TXRX_WKUP_IRQn,
				BLE_WKUP_PRIO,
				radio_timer_txrx_wkup_isr,
				NULL,
				0);
#endif /* CONFIG_SOC_STM32WB06XX || CONFIG_SOC_STM32WB07XX */

	IRQ_CONNECT(RADIO_TIMER_CPU_WKUP_IRQn,
				CPU_WKUP_PRIO,
				radio_timer_cpu_wkup_isr,
				NULL,
				0);
	IRQ_CONNECT(RADIO_TIMER_ERROR_IRQn,
				RADIO_TIMER_ERROR_PRIO,
				radio_timer_error_isr,
				NULL,
				0);

	/* Peripheral clock enable */
	if (__HAL_RCC_RADIO_IS_CLK_DISABLED()) {
		/* Radio reset */
		__HAL_RCC_RADIO_FORCE_RESET();
		__HAL_RCC_RADIO_RELEASE_RESET();

		/* Enable Radio peripheral clock */
		__HAL_RCC_RADIO_CLK_ENABLE();
	}

	/* Wait to be sure that the Radio Timer is active */
	while (LL_RADIO_TIMER_GetAbsoluteTime(WAKEUP) < 0x10) {
	}

	/* Device IRQs are enabled by this function. */
	HAL_RADIO_TIMER_Init(&vtimer_init_struct);
	LL_RADIO_TIMER_EnableWakeupTimerLowPowerMode(WAKEUP);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
