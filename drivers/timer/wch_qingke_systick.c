/*
 * Copyright (c) 2022 TOKITA Hiroshi <tokita.hiroshi@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <zephyr/spinlock.h>

#define DT_DRV_COMPAT wch_qingke_systick

#define SYSTICK_CTRL_NO_INIT_COUNTER	(0)
#define SYSTICK_CTRL_INIT_COUNTER	BIT(5)
#define SYSTICK_CTRL_COUNT_UP		(0)
#define SYSTICK_CTRL_COUNT_DOWN		BIT(4)
#define SYSTICK_CTRL_DISABLE_AUTORELOAD (0)
#define SYSTICK_CTRL_ENABLE_AUTORELOAD	BIT(3)
#define SYSTICK_CTRL_CLOCK_HCLK		(0)
#define SYSTICK_CTRL_CLOCK_HCLK_DIV_8	BIT(2)
#define SYSTICK_CTRL_DISABLE_INTERRUPT	(0)
#define SYSTICK_CTRL_ENABLE_INTERRUPT	BIT(1)
#define SYSTICK_CTRL_DISABLE_COUNTER	(0)
#define SYSTICK_CTRL_ENABLE_COUNTER	BIT(0)

#define SYSTICK_REG  DT_INST_REG_ADDR(0)
#define SYSTICK_IRQN DT_INST_IRQN(0)
#define SYSTICK	     ((struct QingKe_SysTick *)SYSTICK_REG)

#define CYC_PER_TICK                                                                               \
	((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() /                                      \
		    (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_CYC	  INT_MAX
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

#define TICKLESS IS_ENABLED(CONFIG_TICKLESS_KERNEL)

struct QingKe_SysTick {
	volatile uint32_t CTLR;
	volatile uint32_t SR;
	volatile uint64_t CNT;
	volatile uint64_t CMP;
};

static struct k_spinlock lock;
static uint64_t last_count;
#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = SYSTICK_IRQN;
#endif

static void set_systick_compare(uint64_t time)
{
	SYSTICK->CMP = time;
}

static uint64_t systick_count(void)
{
	return SYSTICK->CNT;
}

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);
	SYSTICK->SR = 0;

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = systick_count();
	uint32_t dticks = (uint32_t)((now - last_count) / CYC_PER_TICK);

	last_count = now;

	if (!TICKLESS) {
		uint64_t next = last_count + CYC_PER_TICK;

		if ((int64_t)(next - now) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_systick_compare(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = systick_count();
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

	set_systick_compare(cyc + last_count);
	k_spin_unlock(&lock, key);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = ((uint32_t)systick_count() - (uint32_t)last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)systick_count();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return systick_count();
}

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	SYSTICK->CMP = 0;
	SYSTICK->SR = 0;
	SYSTICK->CNT = 0;
	SYSTICK->CTLR = 0x7;

	IRQ_CONNECT(SYSTICK_IRQN, 0, timer_isr, NULL, 0);
	last_count = systick_count();
	set_systick_compare(last_count + CYC_PER_TICK);
	irq_enable(SYSTICK_IRQN);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
