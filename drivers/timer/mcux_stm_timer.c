/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys_clock.h>

#include <fsl_stm.h>

#define STM_NODE DT_CHOSEN(zephyr_system_timer)

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an nxp,stm node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(STM_NODE, nxp_stm),
	     "zephyr,system-timer must point to an nxp,stm compatible node");

#define STM_BASE ((STM_Type *)DT_REG_ADDR(STM_NODE))
#define STM_IRQN DT_IRQN(STM_NODE)
#define STM_IRQ_PRIORITY DT_IRQ(STM_NODE, priority)
#define STM_PRESCALER DT_PROP(STM_NODE, prescaler)
#define STM_CHANNEL kSTM_Channel_0

extern unsigned int z_clock_hw_cycles_per_sec;

static uint32_t cycles_per_tick;
static uint32_t cycles_max;
static uint32_t last_count;
static uint32_t last_elapsed;

static void stm_set_compare(uint32_t compare)
{
	uint32_t now;

	STM_DisableCompareChannel(STM_BASE, STM_CHANNEL);
	STM_ClearStatusFlags(STM_BASE, STM_CHANNEL);
	STM_SetCompare(STM_BASE, STM_CHANNEL, compare);

	now = STM_GetTimerCount(STM_BASE);
	if (unlikely((int32_t)(compare - now) <= 0)) {
		uint32_t bump = 1U;

		do {
			compare = now + bump;
			bump *= 2U;
			STM_SetCompare(STM_BASE, STM_CHANNEL, compare);
			now = STM_GetTimerCount(STM_BASE);
		} while ((int32_t)(compare - now) <= 0);
	}
}

static void stm_disable_compare(void)
{
	STM_DisableCompareChannel(STM_BASE, STM_CHANNEL);
	STM_ClearStatusFlags(STM_BASE, STM_CHANNEL);
}

static void mcux_stm_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();
	uint32_t now = STM_GetTimerCount(STM_BASE);
	uint32_t delta_cycles = now - last_count;
	uint32_t delta_ticks = (unsigned long)delta_cycles / cycles_per_tick;

	last_count += delta_ticks * cycles_per_tick;
	last_elapsed = 0U;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		stm_set_compare(last_count + cycles_per_tick);
	} else {
		stm_disable_compare();
	}

	sys_clock_announce_locked(delta_ticks, key);
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	uint64_t wait_ticks = (uint64_t)last_elapsed + (uint64_t)ticks;
	uint64_t wait_cycles = wait_ticks * (uint64_t)cycles_per_tick;
	uint32_t cycles = (wait_cycles > cycles_max) ? cycles_max : (uint32_t)wait_cycles;

	stm_set_compare(last_count + cycles);
}

uint32_t sys_clock_elapsed(void)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	uint32_t now = STM_GetTimerCount(STM_BASE);
	uint32_t delta_cycles = now - last_count;
	uint32_t delta_ticks = (unsigned long)delta_cycles / cycles_per_tick;

	last_elapsed = delta_ticks;
	return delta_ticks;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return STM_GetTimerCount(STM_BASE);
}

void sys_clock_disable(void)
{
	stm_disable_compare();
	STM_StopTimer(STM_BASE);
	irq_disable(STM_IRQN);
}

static int sys_clock_driver_init(void)
{
	const struct device *clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(STM_NODE));
	clock_control_subsys_t clock_subsys =
		(clock_control_subsys_t)DT_CLOCKS_CELL(STM_NODE, name);
	stm_config_t config;
	uint32_t clock_freq;
	uint32_t timer_freq;

	if (!device_is_ready(clock_dev)) {
		return -ENODEV;
	}

	if (clock_control_on(clock_dev, clock_subsys) != 0) {
		return -EINVAL;
	}

	if (clock_control_get_rate(clock_dev, clock_subsys, &clock_freq) != 0) {
		return -EINVAL;
	}

	timer_freq = clock_freq / (STM_PRESCALER + 1U);
	if ((timer_freq == 0U) ||
	    ((timer_freq % CONFIG_SYS_CLOCK_TICKS_PER_SEC) != 0U)) {
		return -EINVAL;
	}

	z_clock_hw_cycles_per_sec = timer_freq;
	cycles_per_tick = timer_freq / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	cycles_max = (INT32_MAX / cycles_per_tick) * cycles_per_tick;

	STM_GetDefaultConfig(&config);
	config.enableIRQ = true;
	config.prescale = (uint8_t)STM_PRESCALER;
	STM_Init(STM_BASE, &config);

	IRQ_CONNECT(STM_IRQN, STM_IRQ_PRIORITY, mcux_stm_timer_isr, NULL, 0);
	irq_enable(STM_IRQN);

	STM_StartTimer(STM_BASE);
	last_count = (STM_GetTimerCount(STM_BASE) / cycles_per_tick) * cycles_per_tick;
	last_elapsed = 0U;
	stm_set_compare(last_count + cycles_per_tick);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
