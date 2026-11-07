/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_cmsdk_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/clock.h>
#include "timer_cmsdk_apb.h"

#define TIMER_NODE DT_CHOSEN(zephyr_system_timer)

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an arm,cmsdk-timer node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(TIMER_NODE, arm_cmsdk_timer),
	     "zephyr,system-timer must point to an arm,cmsdk-timer compatible node");

#define TIMER_IRQ      DT_IRQN(TIMER_NODE)
#define TIMER_IRQ_PRIO DT_IRQ(TIMER_NODE, priority)
#define TIMER_BASE     DT_REG_ADDR(TIMER_NODE)

/* Reload floor, in cycles: the closest-in interval the driver will program.
 * Expanded by the core, so it may use the cycles-per-tick the core derives.
 */
#ifdef CONFIG_CMSDK_APB_TIMER_MIN_DELAY_OVERRIDE
#define MIN_DELAY_CYCLES CONFIG_CMSDK_APB_TIMER_MIN_DELAY_CYCLES
#else
#define MIN_DELAY_CYCLES MAX(1024U, (TIMER_CORE_CYC_PER_TICK / 16U))
#endif

struct tmr_cmsdk_apb_cfg {
	volatile struct timer_cmsdk_apb *timer;
};

struct tmr_cmsdk_apb_dev_data {
	/* Interval currently programmed into RELOAD. */
	uint32_t load;
	/* Whole intervals accumulated so far, the synthesized cycle count. */
	uint32_t cycle_count;
};

static const struct tmr_cmsdk_apb_cfg cfg_inst0 = {
	.timer = ((volatile struct timer_cmsdk_apb *)TIMER_BASE),
};

static struct tmr_cmsdk_apb_dev_data data_inst0;

/* Cycles counted down since the interval was programmed.
 *
 * @param val_out Optional: the raw VALUE snapshot this used, for a caller that
 *                needs to chain a measurement onto the window accounted here
 *                with no gap in between (see timer_driver_set_reload()).
 */
/* Cycles of finished periods not yet folded into cycle_count */
static uint32_t overflow_cyc;

/*
 * Cycles elapsed since cycle_count was last updated. The counter reloads by
 * itself when it reaches zero, before the interrupt is served, so a reader
 * that holds the clock lock can see the reloaded counter while the finished
 * period is still unaccounted. The pending interrupt flag, or a counter that
 * grew between two reads, tells that a period finished; it is counted here
 * once and the flag cleared, the way the SysTick driver uses COUNTFLAG.
 */
static uint32_t elapsed(uint32_t *val_out)
{
	const struct tmr_cmsdk_apb_cfg *const cfg = &cfg_inst0;
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	uint32_t val1 = cfg->timer->value;
	uint32_t pending = cfg->timer->intstatus;
	uint32_t val2 = cfg->timer->value;

	/* Zero is the instant between reaching zero and reloading */
	if (val1 == 0) {
		val1 = data->load;
	}
	if (val2 == 0) {
		val2 = data->load;
	}

	if ((pending & TIMER_CTRL_INT_CLEAR) != 0 || val1 < val2) {
		overflow_cyc += data->load;
		cfg->timer->intclear = TIMER_CTRL_INT_CLEAR;
	}

	if (val_out != NULL) {
		*val_out = val2;
	}

	return (data->load - val2) + overflow_cyc;
}

static inline uint32_t timer_driver_cycle_get(void)
{
	return data_inst0.cycle_count + elapsed(NULL);
}

static void timer_driver_set_reload(uint32_t rel)
{
	const struct tmr_cmsdk_apb_cfg *const cfg = &cfg_inst0;
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	uint32_t last_load = data->load;
	uint32_t val1;

	/* elapsed() hands back its own VALUE snapshot, so the window measured by
	 * (val1 - val2) below abuts the window it just accounted for. Reading
	 * VALUE separately afterwards would leave the cycles in between counted
	 * by neither, i.e. systematically lost drift. The core has already
	 * clamped rel to [MIN_DELAY_CYCLES, TIMER_CORE_ALARM_MAX_CYCLES].
	 */
	data->cycle_count += elapsed(&val1);
	overflow_cyc = 0;
	data->load = rel;

	uint32_t val2 = cfg->timer->value;

	cfg->timer->reload = rel;
	cfg->timer->value = rel;
	/* A wrap accounted above must not be counted again by the ISR */
	cfg->timer->intclear = TIMER_CTRL_INT_CLEAR;
	NVIC_ClearPendingIRQ(TIMER_IRQ);

	/* verify if underflow occurred after reading val1 and before reading val2 */
	if (val1 < val2) {
		data->cycle_count += (val1 + (last_load - val2));
	} else {
		data->cycle_count += (val1 - val2);
	}
}

#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_ALARM_MIN_CYCLES MIN_DELAY_CYCLES
#define TIMER_CORE_COUNTER_NONATOMIC

#include "system_timer_generic.h"

static void cmsdk_apb_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);
	const struct tmr_cmsdk_apb_cfg *const cfg = &cfg_inst0;
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	k_spinlock_key_t key = sys_clock_lock();

	/* The finished interval sits in overflow_cyc, counted by elapsed() here
	 * or by an earlier reader. Commit it under the clock lock, atomically
	 * with the announce.
	 */
	(void)elapsed(NULL);
	data->cycle_count += overflow_cyc;
	overflow_cyc = 0;

	cfg->timer->intclear = TIMER_CTRL_INT_CLEAR;
	NVIC_ClearPendingIRQ(TIMER_IRQ);

	timer_core_announce_from(key);
}

static int sys_clock_driver_init(void)
{
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	const struct tmr_cmsdk_apb_cfg *cfg = &cfg_inst0;
	const struct wuc_dt_spec wuc = WUC_DT_SPEC_GET_OR(TIMER_NODE, {0});

	if ((wuc.dev != NULL) && (wuc_enable_wakeup_source_dt(&wuc) != 0)) {
		return -EIO;
	}

	data->load = TIMER_CORE_CYC_PER_TICK;
	cfg->timer->reload = TIMER_CORE_CYC_PER_TICK;
	cfg->timer->value = TIMER_CORE_CYC_PER_TICK;
	cfg->timer->ctrl = TIMER_CTRL_EN | TIMER_CTRL_IRQ_EN;

	IRQ_CONNECT(TIMER_IRQ, TIMER_IRQ_PRIO, cmsdk_apb_timer_isr, NULL, 0);
	timer_core_init();
	irq_enable(TIMER_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
