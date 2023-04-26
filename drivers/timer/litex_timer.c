/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT litex_timer0

#include <zephyr/kernel.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>

#define TIMER_LOAD_ADDR			DT_INST_REG_ADDR_BY_NAME(0, load)
#define TIMER_RELOAD_ADDR		DT_INST_REG_ADDR_BY_NAME(0, reload)
#define TIMER_EN_ADDR			DT_INST_REG_ADDR_BY_NAME(0, en)
#define TIMER_UPDATE_VALUE_ADDR		DT_INST_REG_ADDR_BY_NAME(0, update_value)
#define TIMER_VALUE_ADDR		DT_INST_REG_ADDR_BY_NAME(0, value)
#define TIMER_EV_STATUS_ADDR		DT_INST_REG_ADDR_BY_NAME(0, ev_status)
#define TIMER_EV_PENDING_ADDR		DT_INST_REG_ADDR_BY_NAME(0, ev_pending)
#define TIMER_EV_ENABLE_ADDR		DT_INST_REG_ADDR_BY_NAME(0, ev_enable)
#define TIMER_UPTIME_LATCH_ADDR		DT_INST_REG_ADDR_BY_NAME(0, uptime_latch)
#define TIMER_UPTIME_CYCLES_ADDR	DT_INST_REG_ADDR_BY_NAME(0, uptime_cycles)

#define TIMER_EV		0x1
#define TIMER_IRQ		DT_INST_IRQN(0)
#define TIMER_DISABLE		0x0
#define TIMER_ENABLE		0x1
#define TIMER_UPTIME_LATCH	0x1
#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TIMER_IRQ;
#endif

static void litex_timer_irq_handler(const void *device)
{
	unsigned int key = irq_lock();

	litex_write8(TIMER_EV, TIMER_EV_PENDING_ADDR);
	sys_clock_announce(1);

	irq_unlock(key);
}

uint32_t sys_clock_cycle_get_32(void)
{
	static struct k_spinlock lock;
	uint32_t uptime_cycles;
	k_spinlock_key_t key = k_spin_lock(&lock);

	litex_write8(TIMER_UPTIME_LATCH, TIMER_UPTIME_LATCH_ADDR);
	uptime_cycles = (uint32_t)litex_read64(TIMER_UPTIME_CYCLES_ADDR);

	k_spin_unlock(&lock, key);

	return uptime_cycles;
}

uint64_t sys_clock_cycle_get_64(void)
{
	static struct k_spinlock lock;
	uint64_t uptime_cycles;
	k_spinlock_key_t key = k_spin_lock(&lock);

	litex_write8(TIMER_UPTIME_LATCH, TIMER_UPTIME_LATCH_ADDR);
	uptime_cycles = litex_read64(TIMER_UPTIME_CYCLES_ADDR);

	k_spin_unlock(&lock, key);

	return uptime_cycles;
}

/* tickless kernel is not supported */
uint32_t sys_clock_elapsed(void)
{
	return 0;
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(TIMER_IRQ, DT_INST_IRQ(0, priority),
			litex_timer_irq_handler, NULL, 0);
	irq_enable(TIMER_IRQ);

	litex_write8(TIMER_DISABLE, TIMER_EN_ADDR);

	litex_write32(k_ticks_to_cyc_floor32(1), TIMER_RELOAD_ADDR);
	litex_write32(k_ticks_to_cyc_floor32(1), TIMER_LOAD_ADDR);

	litex_write8(TIMER_ENABLE, TIMER_EN_ADDR);
	litex_write8(litex_read8(TIMER_EV_PENDING_ADDR), TIMER_EV_PENDING_ADDR);
	litex_write8(TIMER_EV, TIMER_EV_ENABLE_ADDR);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_1,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
