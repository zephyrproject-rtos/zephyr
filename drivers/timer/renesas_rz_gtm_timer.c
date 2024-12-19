/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <instances/rzg/r_gtm.h>

#define DT_DRV_COMPAT renesas_rz_gtm_os_timer
#define TIMER_NODE    DT_INST_PARENT(0)

static void ostm_irq_handler(timer_callback_args_t *arg);
void gtm_int_isr(void);
const struct device *g_os_timer_dev = DEVICE_DT_INST_GET(0);
extern int z_clock_hw_cycles_per_sec;

struct rz_os_timer_config {
	timer_cfg_t *fsp_cfg;
	const timer_api_t *fsp_api;
};

struct rz_os_timer_data {
	timer_ctrl_t *fsp_ctrl;
	struct k_spinlock lock;
	uint32_t cyc_per_tick;
	volatile uint32_t ostm_last_cnt;
};

static void ostm_irq_handler(timer_callback_args_t *arg)
{
	uint32_t ticks_diff;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;

	ARG_UNUSED(arg);

	ticks_diff = sys_clock_cycle_get_32() - data->ostm_last_cnt;
	ticks_diff /= data->cyc_per_tick;

	data->ostm_last_cnt += ticks_diff * data->cyc_per_tick;

#if !defined(CONFIG_TICKLESS_KERNEL)
	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;

	config->fsp_api->periodSet(data->fsp_ctrl, data->ostm_last_cnt + data->cyc_per_tick);
#else
	irq_disable(DT_IRQN(TIMER_NODE));
#endif
	/* Announce to the kernel */
	sys_clock_announce(ticks_diff);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
#if defined(CONFIG_TICKLESS_KERNEL)
	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;
	uint32_t unannounced;
	uint32_t next_cycle;
	timer_status_t timer_status;

	if (ticks == K_TICKS_FOREVER) {
		if (idle) {
			return;
		}
		ticks = INT32_MAX;
	}

	ticks = CLAMP(ticks, 0, (unsigned long)-1 / 2 / data->cyc_per_tick);
	ticks = CLAMP(ticks - 1, 0, INT32_MAX / 2);

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	config->fsp_api->statusGet(data->fsp_ctrl, &timer_status);
	unannounced = timer_status.counter - data->ostm_last_cnt + data->cyc_per_tick - 1;
	unannounced /= data->cyc_per_tick;

	next_cycle = (ticks + unannounced) * data->cyc_per_tick + data->ostm_last_cnt;

	config->fsp_api->periodSet(data->fsp_ctrl, next_cycle);

	k_spin_unlock(&data->lock, key);

	irq_enable(DT_IRQN(TIMER_NODE));
#else /* CONFIG_TICKLESS_KERNEL */
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	uint32_t ticks_diff;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	ticks_diff = sys_clock_cycle_get_32() - data->ostm_last_cnt;
	return ticks_diff / data->cyc_per_tick;
}

void sys_clock_disable(void)
{
	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;

	config->fsp_api->close(data->fsp_ctrl);
}

uint32_t sys_clock_cycle_get_32(void)
{
	k_spinlock_key_t key;
	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;
	timer_status_t timer_status;

	key = k_spin_lock(&data->lock);
	config->fsp_api->statusGet(data->fsp_ctrl, &timer_status);
	k_spin_unlock(&data->lock, key);

	return timer_status.counter;
}

static int sys_clock_driver_init(void)
{
	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;

	IRQ_CONNECT(DT_IRQN(TIMER_NODE), DT_IRQ(TIMER_NODE, priority), gtm_int_isr,
		    DEVICE_DT_INST_GET(0), 0);

	data->ostm_last_cnt = 0;
	z_clock_hw_cycles_per_sec = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_P0CLK);
	data->cyc_per_tick = sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	config->fsp_cfg->period_counts = data->cyc_per_tick;
	config->fsp_api->open(data->fsp_ctrl, config->fsp_cfg);
	config->fsp_api->start(data->fsp_ctrl);

	return 0;
}

#define OS_TIMER_RZG_GTM_INIT()                                                                    \
	const gtm_extended_cfg_t g_timer0_extend = {                                               \
		.generate_interrupt_when_starts = GTM_GIWS_TYPE_DISABLED,                          \
		.gtm_mode = GTM_TIMER_MODE_FREERUN,                                                \
	};                                                                                         \
                                                                                                   \
	static timer_cfg_t g_timer0_cfg = {                                                        \
		.mode = TIMER_MODE_PERIODIC,                                                       \
		.period_counts = 0,                                                                \
		.channel = DT_PROP(TIMER_NODE, channel),                                           \
		.p_callback = ostm_irq_handler,                                                    \
		.p_context = DEVICE_DT_INST_GET(0),                                                \
		.p_extend = &g_timer0_extend,                                                      \
		.cycle_end_ipl = DT_IRQ(TIMER_NODE, priority),                                     \
		.cycle_end_irq = DT_IRQN(TIMER_NODE),                                              \
	};                                                                                         \
                                                                                                   \
	static gtm_instance_ctrl_t g_timer0_ctrl;                                                  \
                                                                                                   \
	static struct rz_os_timer_data g_rz_os_timer_data = {                                      \
		.fsp_ctrl = (timer_ctrl_t *)&g_timer0_ctrl,                                        \
	};                                                                                         \
                                                                                                   \
	struct rz_os_timer_config g_rz_os_timer_config = {                                         \
		.fsp_cfg = &g_timer0_cfg,                                                          \
		.fsp_api = &g_timer_on_gtm,                                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(0, NULL, NULL, &g_rz_os_timer_data, &g_rz_os_timer_config,           \
			      PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);              \
                                                                                                   \
	SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

OS_TIMER_RZG_GTM_INIT();
