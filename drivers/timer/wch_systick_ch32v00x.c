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

#define STK_CNTIF BIT(0)

#define CYCLES_PER_SEC  sys_clock_hw_cycles_per_sec()
#define CYCLES_PER_TICK (CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define SYSTICK ((SysTick_Type *)(DT_INST_REG_ADDR(0)))

static uint64_t last_cycles_announced;

static inline bool cycles_close_to_next_cmp(uint32_t cycles)
{
	return (cycles % CYCLES_PER_TICK) > (9 * CYCLES_PER_TICK / 10);
}

static void ch32v00x_systick_irq(const void *unused)
{
	uint64_t elapsed_cycles;
	uint32_t ticks = 0;
	uint64_t cnt = SYSTICK->CNT;

	ARG_UNUSED(unused);

	if (cnt < last_cycles_announced) {
		elapsed_cycles = (UINT64_MAX - last_cycles_announced) + cnt;
		ticks = elapsed_cycles / CYCLES_PER_TICK;

		/* If we're too close to the next tick, announce that tick early now rather than
		 * miss it
		 */
		if (cycles_close_to_next_cmp(elapsed_cycles % CYCLES_PER_TICK)) {
			ticks++;
			last_cycles_announced = (cnt % CYCLES_PER_TICK) + CYCLES_PER_TICK;
		} else {
			last_cycles_announced = cnt % CYCLES_PER_TICK;
		}
	} else {
		ticks = (cnt - last_cycles_announced) / CYCLES_PER_TICK;

		/* If we're too close to the next tick, announce that tick early now rather than
		 * miss it
		 */
		if (cycles_close_to_next_cmp(cnt - last_cycles_announced)) {
			ticks++;
		}

		last_cycles_announced += ticks * CYCLES_PER_TICK;
	}


	/* Ensure we trigger when CNT resets to zero */
	if (UINT64_MAX - SYSTICK->CMP < CYCLES_PER_TICK) {
		SYSTICK->CMP = SYSTICK->CMP % CYCLES_PER_TICK;
	} else {
		SYSTICK->CMP = (last_cycles_announced + CYCLES_PER_TICK);
	}

	SYSTICK->SR = 0;

	sys_clock_announce(ticks);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)SYSTICK->CNT;
}

uint64_t sys_clock_cycle_get_64(void)
{
	return SYSTICK->CNT;
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

static int ch32v00x_systick_init(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), 0, ch32v00x_systick_irq, NULL, 0);

	SYSTICK->SR = 0;
	SYSTICK->CMP = CYCLES_PER_TICK;
	SYSTICK->CNT = 0;

	irq_enable(DT_INST_IRQN(0));

	SYSTICK->CTLR = STK_STE | STK_STCLK | STK_STIE;

	return 0;
}

SYS_INIT(ch32v00x_systick_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
