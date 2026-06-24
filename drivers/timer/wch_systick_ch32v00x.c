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

#define STK_SWIE  BIT(31)
#define STK_STRE  BIT(3)
#define STK_STCLK BIT(2)
#define STK_STIE  BIT(1)
#define STK_STE   BIT(0)

#define STK_CNTIF BIT(0)

#define CYCLES_PER_SEC  sys_clock_hw_cycles_per_sec()
#define CYCLES_PER_TICK (CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define SYSTICK ((SysTick_Type *)(DT_INST_REG_ADDR(0)))

static volatile uint32_t ch32v00x_systick_count;

static void ch32v00x_systick_irq(const void *unused)
{
	ARG_UNUSED(unused);

#if defined(CONFIG_SOC_SERIES_QINGKE_V3A)
	SYSTICK->CTLR = 0; /* Stop timer. */

	/* Clear CNT */
	SYSTICK->CNT0 = 0;
	SYSTICK->CNT1 = 0;
	SYSTICK->CNT2 = 0;
	SYSTICK->CNT3 = 0;
	SYSTICK->CNT4 = 0;
	SYSTICK->CNT5 = 0;
	SYSTICK->CNT6 = 0;
	SYSTICK->CNT7 = 0;

	SYSTICK->CTLR = STK_STE; /* Restart timer. */
#else
	SYSTICK->SR = 0;
#endif
	ch32v00x_systick_count += CYCLES_PER_TICK; /* Track cycles. */
	sys_clock_announce(1);                     /* Poke the scheduler. */
}

uint32_t sys_clock_cycle_get_32(void)
{
	return ch32v00x_systick_count + SYSTICK->CNT;
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

static int ch32v00x_systick_init(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), 0, ch32v00x_systick_irq, NULL, 0);

#if defined(CONFIG_SOC_SERIES_QINGKE_V3A)
	/* SysTick runs at HCLK/8, so adjust cycles per tick accordingly. */
	static const uint32_t ticks = CYCLES_PER_TICK / 8;

	SYSTICK->CTLR = 0;

	/* Clear all 8 CNTs */
	SYSTICK->CNT0 = 0;
	SYSTICK->CNT1 = 0;
	SYSTICK->CNT2 = 0;
	SYSTICK->CNT3 = 0;
	SYSTICK->CNT4 = 0;
	SYSTICK->CNT5 = 0;
	SYSTICK->CNT6 = 0;
	SYSTICK->CNT7 = 0;

	SYSTICK->CMP0 = (uint8_t)ticks;
	SYSTICK->CMP1 = (uint8_t)(ticks >> 8);
	SYSTICK->CMP2 = (uint8_t)(ticks >> 16);
	SYSTICK->CMP3 = (uint8_t)(ticks >> 24);
	SYSTICK->CMP4 = 0;
	SYSTICK->CMP5 = 0;
	SYSTICK->CMP6 = 0;
	SYSTICK->CMP7 = 0;

	SYSTICK->CTLR = STK_STE;
#else
	SYSTICK->SR = 0;
	SYSTICK->CMP = CYCLES_PER_TICK;
	SYSTICK->CNT = 0;
	SYSTICK->CTLR = STK_STRE | STK_STCLK | STK_STIE | STK_STE;
#endif

	irq_enable(DT_INST_IRQN(0));

	return 0;
}

SYS_INIT(ch32v00x_systick_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
