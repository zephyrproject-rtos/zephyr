/*
 * Copyright (c) 2026 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys/clock.h>
#include <zephyr/logging/log.h>
#include <r_ostm.h>

LOG_MODULE_REGISTER(renesas_rh850_ostm_timer);

#define DT_DRV_COMPAT renesas_rh850_ostm_timer
#define TIMER_NODE    DT_INST_PARENT(0)
#define TIMER_REG     ((R_OSTMn_Type *)DT_REG_ADDR(TIMER_NODE))
#define COUNTER_MAX   (~(uint32_t)0)

/*
 * Tick <-> cycle:
 *
 *  - CONFIG_SYS_CLOCK_TICKS_PER_SEC: System tick frequency (in ticks/second)
 *  - CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC: System clock's h/w timer frequency
 *
 *  => CYC_PER_TICK = cycle / tick
 */
/* Compute MAX_TICK, CYC_PER_TICK at driver init to avoid recomputing in function calls  */
static uint64_t max_tick;
static uint32_t cyc_per_tick;
static uint32_t counter_half_span;
#define MAX_TICKS         max_tick
#define CYC_PER_TICK      cyc_per_tick
#define COUNTER_HALF_SPAN counter_half_span

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(TIMER_NODE);
#endif

static void ostm_irq_handler(timer_callback_args_t *arg);
static void ostm_period_set(uint32_t period_counts);
void ostm_int_isr(void);

static const struct device *g_os_timer_dev = DEVICE_DT_INST_GET(0);

struct rh850_os_timer_config {
	const timer_api_t *fsp_api;
};

struct rh850_os_timer_data {
	timer_cfg_t *fsp_cfg;
	timer_ctrl_t *fsp_ctrl;
	struct k_spinlock lock;
	uint32_t last_cycle;
};

void rh850_os_timer_ostm_isr(const struct device *arg)
{
	ARG_UNUSED(arg);
	ostm_int_isr();
}

static void ostm_irq_handler(timer_callback_args_t *arg)
{
	ARG_UNUSED(arg);

	struct rh850_os_timer_data *data = (struct rh850_os_timer_data *)g_os_timer_dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t now_cycle = sys_clock_cycle_get_32();
	uint32_t delta_ticks = (now_cycle - data->last_cycle) / CYC_PER_TICK;

	data->last_cycle += delta_ticks * CYC_PER_TICK;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t next_cycle = data->last_cycle + CYC_PER_TICK;

		ostm_period_set(next_cycle);
	} else {
		irq_disable(DT_IRQN(TIMER_NODE));
	}
	k_spin_unlock(&data->lock, key);

	/* Announce to the kernel */
	sys_clock_announce(delta_ticks);
}

static void ostm_period_set(uint32_t period_counts)
{
	TIMER_REG->OSTMnCMP = period_counts;
	uint32_t now = sys_clock_cycle_get_32();

	/* Ensure compare value is not set in the past count */
	if ((int32_t)(period_counts - now) <= 1) {
		uint32_t bump = 1;

		do {
			bump <<= 1;
			period_counts = now + bump;
			TIMER_REG->OSTMnCMP = period_counts;
			now = sys_clock_cycle_get_32();

		} while ((((int32_t)(period_counts - now)) <= 1));
	}

	/* Update period member of FSP timer ctrl struct */
	struct rh850_os_timer_data *data = (struct rh850_os_timer_data *)g_os_timer_dev->data;
	ostm_instance_ctrl_t *g_timer0_ctrl = (ostm_instance_ctrl_t *)data->fsp_ctrl;

	g_timer0_ctrl->period = period_counts;
}

void sys_clock_set_timeout(uint32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	struct rh850_os_timer_data *data = (struct rh850_os_timer_data *)g_os_timer_dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Clamp the ticks value to a valid range */
	ticks = CLAMP(ticks, 1U, (uint32_t)MAX_TICKS) - 1U;

	/* Calculate the timer delay in cycles */
	uint32_t now = sys_clock_cycle_get_32();
	uint32_t elapsed = now - data->last_cycle;
	uint32_t delay_cycles = ticks * CYC_PER_TICK;

	/* Force tick interrupt as soon as possible if
	 * there has been no call to sys_clock_announce
	 * for more than half the 32-bit timer duration.
	 * This is to prevent losing of timer rollover
	 * event.
	 */
	if (elapsed >= COUNTER_HALF_SPAN) {
		delay_cycles = 0;
	}

	/* Adjust delay to align with tick boundaries */
	delay_cycles += elapsed;
	delay_cycles = DIV_ROUND_UP(delay_cycles, CYC_PER_TICK) * CYC_PER_TICK;
	delay_cycles -= elapsed;

	uint32_t next_cycle = now + delay_cycles;

	ostm_period_set(next_cycle);

	k_spin_unlock(&data->lock, key);
	irq_enable(DT_IRQN(TIMER_NODE));
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	struct rh850_os_timer_data *data = (struct rh850_os_timer_data *)g_os_timer_dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t delta_cycles = sys_clock_cycle_get_32() - data->last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	k_spin_unlock(&data->lock, key);

	return delta_ticks;
}

void sys_clock_disable(void)
{
	struct rh850_os_timer_config *config =
		(struct rh850_os_timer_config *)g_os_timer_dev->config;
	struct rh850_os_timer_data *data = (struct rh850_os_timer_data *)g_os_timer_dev->data;

	config->fsp_api->close(data->fsp_ctrl);
}

uint32_t sys_clock_cycle_get_32(void)
{
	return TIMER_REG->OSTMnCNT;
}

static int sys_clock_driver_init(void)
{
	fsp_err_t ret;
	struct rh850_os_timer_config *config =
		(struct rh850_os_timer_config *)g_os_timer_dev->config;
	struct rh850_os_timer_data *data = (struct rh850_os_timer_data *)g_os_timer_dev->data;

	IRQ_CONNECT(DT_IRQN(TIMER_NODE), DT_IRQ(TIMER_NODE, priority), rh850_os_timer_ostm_isr,
		    DEVICE_DT_INST_GET(0), 0);

	data->last_cycle = 0;

	cyc_per_tick = DIV_ROUND_UP(sys_clock_hw_cycles_per_sec(), CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	counter_half_span = COUNTER_MAX / 2;
	max_tick = (COUNTER_HALF_SPAN - CYC_PER_TICK) / CYC_PER_TICK;

	data->fsp_cfg->period_counts = CYC_PER_TICK;
	ret = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);

	if (ret != FSP_SUCCESS) {
		LOG_ERR("timer initialize failed");
		return -EIO;
	}

	irq_enable(DT_IRQN(TIMER_NODE));

	ret = config->fsp_api->start(data->fsp_ctrl);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("timer start failed");
		return -EIO;
	}

	return 0;
}

#define OS_TIMER_RH850_OSTM_INIT()                                                                 \
	const ostm_extended_cfg_t g_timer0_extend = {                                              \
		.generate_interrupt_when_starts = OSTM_GIWS_TYPE_DISABLED,                         \
		.ostm_mode = OSTM_TIMER_MODE_FREERUN,                                              \
		.counter_load_at_start = OSTM_COUNTER_START_RELOAD_ENABLE,                         \
		.output_setting = OSTM_OUTPUT_SOFTWARE_CONTROL_MODE,                               \
	};                                                                                         \
                                                                                                   \
	static timer_cfg_t g_timer0_cfg = {                                                        \
		.mode = TIMER_MODE_PERIODIC,                                                       \
		.period_counts = 0,                                                                \
		.channel = DT_PROP(TIMER_NODE, channel),                                           \
		.p_callback = ostm_irq_handler,                                                    \
		.p_context = (void *) DEVICE_DT_INST_GET(0),                                       \
		.p_extend = &g_timer0_extend,                                                      \
		.cycle_end_ipl = DT_IRQ(TIMER_NODE, priority),                                     \
		.cycle_end_irq = DT_IRQN(TIMER_NODE),                                              \
	};                                                                                         \
                                                                                                   \
	static ostm_instance_ctrl_t g_timer0_ctrl;                                                 \
                                                                                                   \
	static struct rh850_os_timer_data g_rh850_os_timer_data = {                                \
		.fsp_cfg = &g_timer0_cfg,                                                          \
		.fsp_ctrl = (timer_ctrl_t *)&g_timer0_ctrl,                                        \
	};                                                                                         \
                                                                                                   \
	struct rh850_os_timer_config g_rh850_os_timer_config = {                                   \
		.fsp_api = &g_timer_on_ostm,                                                       \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(0, NULL, NULL, &g_rh850_os_timer_data, &g_rh850_os_timer_config,     \
			      PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);              \
                                                                                                   \
	SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

OS_TIMER_RH850_OSTM_INIT();
