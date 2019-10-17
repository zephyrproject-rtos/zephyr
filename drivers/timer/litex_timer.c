/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <device.h>
#include <irq.h>
#include <drivers/timer/system_timer.h>

#define TIMER_BASE	        DT_INST_0_LITEX_TIMER0_BASE_ADDRESS
#define TIMER_LOAD_ADDR		((TIMER_BASE) + 0x00)
#define TIMER_RELOAD_ADDR	((TIMER_BASE) + 0x10)
#define TIMER_EN_ADDR		((TIMER_BASE) + 0x20)
#define TIMER_EV_PENDING_ADDR	((TIMER_BASE) + 0x3c)
#define TIMER_EV_ENABLE_ADDR	((TIMER_BASE) + 0x40)

#define TIMER_EV	0x1
#define TIMER_IRQ	DT_INST_0_LITEX_TIMER0_IRQ_0
#define TIMER_DISABLE	0x0
#define TIMER_ENABLE	0x1

static u32_t accumulated_cycle_count;

static void litex_timer_irq_handler(void *device)
{
	ARG_UNUSED(device);
	int key = irq_lock();

	sys_write8(TIMER_EV, TIMER_EV_PENDING_ADDR);
	accumulated_cycle_count += sys_clock_hw_cycles_per_tick();
	z_clock_announce(1);

	irq_unlock(key);
}

u32_t z_timer_cycle_get_32(void)
{
	return accumulated_cycle_count;
}

/* tickless kernel is not supported */
u32_t z_clock_elapsed(void)
{
	return 0;
}

int z_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);
	IRQ_CONNECT(TIMER_IRQ, DT_INST_0_LITEX_TIMER0_IRQ_0_PRIORITY,
			litex_timer_irq_handler, NULL, 0);
	irq_enable(TIMER_IRQ);

	sys_write8(TIMER_DISABLE, TIMER_EN_ADDR);

	for (int i = 0; i < 4; i++) {
		sys_write8(sys_clock_hw_cycles_per_tick() >> (24 - i * 8),
				TIMER_RELOAD_ADDR + i * 0x4);
		sys_write8(sys_clock_hw_cycles_per_tick() >> (24 - i * 8),
				TIMER_LOAD_ADDR + i * 0x4);
	}

	sys_write8(TIMER_ENABLE, TIMER_EN_ADDR);
	sys_write8(sys_read8(TIMER_EV_PENDING_ADDR), TIMER_EV_PENDING_ADDR);
	sys_write8(TIMER_EV, TIMER_EV_ENABLE_ADDR);

	return 0;
}
