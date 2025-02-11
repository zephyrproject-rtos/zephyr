/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <r_ulpt.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>

#define DT_DRV_COMPAT renesas_ra8_ulpt_timer

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error Only one ULPT instance should be enabled
#endif

void ulpt_int_isr(void);
void ra_ulpt_int_isr(void);
void systick_irq(timer_callback_args_t *p_args);

/* Tick/cycle count of the last announce call. */
static volatile uint32_t ulpt_last;
/* Current tick count. */
static volatile uint32_t ulpt_counter;
/* Tick value of the next timeout. */
static volatile uint32_t ulpt_timeout;

ulpt_instance_ctrl_t g_ulpt_ctrl;
const ulpt_extended_cfg_t g_ulpt_extend = {
	.count_source = ULPT_CLOCK_LOCO,
	.ulpto = ULPT_PULSE_PIN_CFG_DISABLED,
	.ulptoab_settings_b.ulptoa = ULPT_MATCH_PIN_CFG_DISABLED,
	.ulptoab_settings_b.ulptob = ULPT_MATCH_PIN_CFG_DISABLED,
	.ulptevi_filter = ULPT_ULPTEVI_FILTER_NONE,
	.enable_function = ULPT_ENABLE_FUNCTION_IGNORED,
	.trigger_edge = ULPT_TRIGGER_EDGE_RISING,
	.event_pin = ULPT_EVENT_PIN_RISING,
};
const timer_cfg_t g_ulpt_cfg = {
	.mode = TIMER_MODE_PERIODIC,
	.source_div = (timer_source_div_t)0,
	.period_counts = 10,
	.duty_cycle_counts = 5,
	.channel = DT_INST_PROP(0, channel),
	.p_callback = systick_irq,
	.p_context = NULL,
	.p_extend = &g_ulpt_extend,
	.cycle_end_ipl = DT_INST_IRQ(0, priority),
	.cycle_end_irq = DT_INST_IRQN(0),
};

static uint32_t accumulated_ulpt_cnt;

static struct k_spinlock lock;

#define _ELC_EVENT_ULPT_INT(channel) ELC_EVENT_ULPT##channel##_INT
#define ELC_EVENT_ULPT_INT(channel)  _ELC_EVENT_ULPT_INT(channel)

void systick_irq(timer_callback_args_t *p_args)
{
	timer_info_t ulpt_info;

	if (TIMER_EVENT_CYCLE_END == p_args->event) {

		R_ULPT_InfoGet(&g_ulpt_ctrl, &ulpt_info);
		k_spinlock_key_t key = k_spin_lock(&lock);

		accumulated_ulpt_cnt += ulpt_info.period_counts;
		k_spin_unlock(&lock, key);

		if (0 == ulpt_timeout) {
			ulpt_counter++;
			sys_clock_announce(1);
			ulpt_last = ulpt_counter;
		} else if (++ulpt_counter == ulpt_timeout) {
			sys_clock_announce(ulpt_counter - ulpt_last);
			ulpt_last = ulpt_counter;
		}
	}
}
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (ticks == K_TICKS_FOREVER) {
		/* Disable comparator for K_TICKS_FOREVER and other negative
		 * values.
		 */
		ulpt_timeout = ulpt_counter;
		return;
	}

	if (ticks < 1) {
		ticks = 1;
	}

	/* Avoid race condition between reading counter and ISR incrementing
	 * it.
	 */
	k_spinlock_key_t key = k_spin_lock(&lock);

	ulpt_timeout = ulpt_counter + ticks;
	k_spin_unlock(&lock, key);
}
uint32_t sys_clock_elapsed(void)
{
	uint32_t elap_time;

	k_spinlock_key_t key = k_spin_lock(&lock);

	elap_time = ulpt_counter - ulpt_last;
	k_spin_unlock(&lock, key);

	return elap_time;
}

static void ra_ulpt_setup_irq(void)
{
	R_ICU->IELSR[DT_INST_IRQN(0)] = ELC_EVENT_ULPT_INT(DT_INST_PROP(0, channel));

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), ra_ulpt_int_isr, 0, 0);

	irq_enable(DT_INST_IRQN(0));
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* just gives the accumulated count in a number of hw cycles */
	timer_info_t ulpt_info;

	k_spinlock_key_t key = k_spin_lock(&lock);

	R_ULPT_InfoGet(&g_ulpt_ctrl, &ulpt_info);

	uint32_t ulpt_time = accumulated_ulpt_cnt;

	/* convert ulpt count in a nb of hw cycles with precision */
	uint64_t ret =
		((uint64_t)ulpt_time * sys_clock_hw_cycles_per_sec()) / ulpt_info.clock_frequency;

	k_spin_unlock(&lock, key);

	return (uint32_t)(ret);
}

static int sys_clock_driver_init(void)
{
	timer_info_t ulpt_info;
	uint32_t period_counts = 0;
	fsp_err_t err;

	ulpt_last = 0U;
	ulpt_counter = 0U;
	ulpt_timeout = 0U;
	ra_ulpt_setup_irq();
	/* Initializes the module. */
	err = R_ULPT_Open(&g_ulpt_ctrl, &g_ulpt_cfg);
	__ASSERT(err == 0, "ULPT timer: initialization: open failed");

	err = R_ULPT_Start(&g_ulpt_ctrl);
	__ASSERT(err == 0, "ULPT timer: start failed");

	R_ULPT_InfoGet(&g_ulpt_ctrl, &ulpt_info);
	period_counts = (uint32_t)((ulpt_info.clock_frequency) / (CONFIG_SYS_CLOCK_TICKS_PER_SEC));

	err = R_ULPT_PeriodSet(&g_ulpt_ctrl, period_counts);
	__ASSERT(err == 0, "ULPT timer: start failed");
	accumulated_ulpt_cnt = 0;
	return 0;
}

void ra_ulpt_int_isr(void)
{
	ulpt_int_isr();
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
