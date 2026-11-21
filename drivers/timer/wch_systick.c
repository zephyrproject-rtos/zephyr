/*
 * Copyright (c) 2024 Michael Hope
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_systick

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <hal_ch32fun.h>

#define STK_STCLK BIT(2)
#define STK_STIE  BIT(1)
#define STK_STE   BIT(0)

#define SYSTICK ((SysTick_Type *)(DT_INST_REG_ADDR(0)))

/*
 * The SysTick counts up at HCLK. With STRE clear it runs on past the compare
 * value instead of restarting from 0, and CNTIF is set when the count reaches
 * CMP: a free-running counter with an absolute compare. The manual does not
 * say whether a CMP already behind the count still fires, so this is a
 * COMPARE_EXACT backend and the core checks each write.
 *
 * The counter and compare are 64-bit where CONFIG_WCH_SYSTICK_64BIT is
 * set, each accessed as two 32-bit halves, and 32-bit otherwise.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT

#ifndef CONFIG_WCH_SYSTICK_64BIT

#define TIMER_CORE_COUNTER_WIDTH 32

static inline uint32_t timer_driver_cycle_get(void)
{
	return SYSTICK->CNT;
}

static inline void timer_driver_set_compare(uint32_t cycles)
{
	SYSTICK->CMP = cycles;
}

static inline void wch_systick_reset(void)
{
	SYSTICK->CMP = UINT32_MAX;
	SYSTICK->CNT = 0;
}

#else

#define TIMER_CORE_COUNTER_WIDTH 64
#define TIMER_CORE_HAVE_CYCLE_GET_32

#define SYSTICK_CNT ((volatile uint32_t *)&SYSTICK->CNT)
#define SYSTICK_CMP ((volatile uint32_t *)&SYSTICK->CMP)

static inline uint64_t timer_driver_cycle_get(void)
{
	uint32_t lo;
	uint32_t hi;

	/* The halves are not latched together: read the high one again to
	 * catch a carry from the low one in between.
	 */
	do {
		hi = SYSTICK_CNT[1];
		lo = SYSTICK_CNT[0];
	} while (SYSTICK_CNT[1] != hi);

	return ((uint64_t)hi << 32) | lo;
}

static inline void timer_driver_set_compare(uint64_t cycles)
{
	/* With the high half at its maximum, no value the low half takes in
	 * between can match the count.
	 */
	SYSTICK_CMP[1] = UINT32_MAX;
	SYSTICK_CMP[0] = (uint32_t)cycles;
	SYSTICK_CMP[1] = (uint32_t)(cycles >> 32);
}

static inline void wch_systick_reset(void)
{
	SYSTICK_CMP[1] = UINT32_MAX;
	SYSTICK_CMP[0] = UINT32_MAX;
	SYSTICK_CNT[1] = 0;
	SYSTICK_CNT[0] = 0;
}

#endif

#include "system_timer_generic.h"

#ifdef TIMER_CORE_HAVE_CYCLE_GET_32
/* The low half alone is the 32-bit count, so skip the carry check. */
uint32_t sys_clock_cycle_get_32(void)
{
	return SYSTICK_CNT[0];
}
#endif

static void wch_systick_isr(const void *unused)
{
	ARG_UNUSED(unused);

	SYSTICK->SR = 0;
	timer_core_announce();
}

static int wch_systick_init(void)
{
	/* Stop the counter and park the compare out of reach before
	 * enabling its interrupt. STRE stays clear so the counter runs free.
	 */
	SYSTICK->CTLR = 0;
	wch_systick_reset();
	SYSTICK->SR = 0;
	SYSTICK->CTLR = STK_STCLK | STK_STIE | STK_STE;

	IRQ_CONNECT(DT_INST_IRQN(0), 0, wch_systick_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	timer_core_init();

	return 0;
}

SYS_INIT(wch_systick_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
