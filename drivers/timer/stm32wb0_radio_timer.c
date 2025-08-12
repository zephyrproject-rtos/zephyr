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
#define MIN_ALLOWED_DELAY	32
#define TIMER_ROUNDING		8

BUILD_ASSERT(DT_NODE_HAS_STATUS(DT_NODELABEL(clk_lsi), disabled),
	     "LSI is not supported yet");

#if (defined(CONFIG_SOC_STM32WB06XX) || defined(CONFIG_SOC_STM32WB06XX)) && defined(CONFIG_PM)
#error "PM is not supported yet for WB06/WB07"
#endif /* (CONFIG_SOC_STM32WB06XX || CONFIG_SOC_STM32WB06XX) && CONFIG_PM */

/* This value is only valid for the LSE with a frequency of 32768 Hz.
 * The implementation for the LSI will be done in the future.
 */
static const uint32_t calibration_data_freq1 = 0x0028F5C2;

/* Translate STU to MTU and vice versa. It is implemented by using integer operations. */
uint32_t blue_unit_conversion(uint32_t time, uint32_t period_freq, uint32_t thr);

static uint64_t announced_cycles;

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

	HAL_RADIO_TIMER_TimeoutCallback();
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		diff_cycles = HAL_RADIO_TIMER_GetCurrentSysTime() - announced_cycles;
		dticks = (int32_t)k_cyc_to_ticks_near64(diff_cycles);
		announced_cycles += k_ticks_to_cyc_near32(dticks);
		sys_clock_announce(dticks);
	} else {
		sys_clock_announce(1);
	}
}

#if defined(CONFIG_SOC_STM32WB06XX) || defined(CONFIG_SOC_STM32WB07XX)
static void radio_timer_txrx_wkup_isr(void *args)
{
	ARG_UNUSED(args);

	HAL_RADIO_TIMER_WakeUpCallback();
}
#endif /* CONFIG_SOC_STM32WB06XX || CONFIG_SOC_STM32WB07XX */

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (ticks == K_TICKS_FOREVER) {
		return;
	}

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t current_time, delay;

		ticks = MAX(1, ticks);
		delay = blue_unit_conversion(k_ticks_to_cyc_near32(ticks), calibration_data_freq1,
					     MULT64_THR_FREQ);
		if (delay > MAX_ALLOWED_DELAY) {
			delay = MAX_ALLOWED_DELAY;
		} else {
			delay = MAX(MIN_ALLOWED_DELAY, delay);
		}
		current_time = LL_RADIO_TIMER_GetAbsoluteTime(WAKEUP);
		LL_RADIO_TIMER_SetCPUWakeupTime(WAKEUP, current_time + delay + TIMER_ROUNDING);
		LL_RADIO_TIMER_EnableCPUWakeupTimer(WAKEUP);
	}
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	return k_cyc_to_ticks_near32(HAL_RADIO_TIMER_GetCurrentSysTime() - announced_cycles);
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
