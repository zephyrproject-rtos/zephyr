/*
 * Copyright (c) 2024-2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/logging/log.h>
#include <instances/rzg/r_gtm.h>

LOG_MODULE_REGISTER(renesas_rz_gtm_timer);

#define DT_DRV_COMPAT renesas_rz_gtm_os_timer
#define TIMER_NODE    DT_INST_PARENT(0)

#define cycle_diff_t   uint32_t
#define CYCLE_DIFF_MAX (~(cycle_diff_t)0)

/*
 * We have two constraints on the maximum number of cycles we can wait for.
 *
 * 1) sys_clock_announce() accepts at most INT32_MAX ticks.
 *
 * 2) The number of cycles between two reports must fit in a cycle_diff_t
 *    variable before converting it to ticks.
 *
 * Then:
 *
 * 3) Pick the smallest between (1) and (2).
 *
 * 4) Take into account some room for the unavoidable IRQ servicing latency.
 *    Let's use 3/4 of the max range.
 *
 * Finally let's add the LSB value to the result so to clear out a bunch of
 * consecutive set bits coming from the original max values to produce a
 * nicer literal for assembly generation.
 */
#define CYCLES_MAX_1 ((uint64_t)INT32_MAX * (uint64_t)CYC_PER_TICK)
#define CYCLES_MAX_2 ((uint64_t)CYCLE_DIFF_MAX)
#define CYCLES_MAX_3 MIN(CYCLES_MAX_1, CYCLES_MAX_2)
#define CYCLES_MAX_4 (CYCLES_MAX_3 / 2 + CYCLES_MAX_3 / 4)
#define CYCLES_MAX_5 (CYCLES_MAX_4 + LSB_GET(CYCLES_MAX_4))

/* precompute CYCLES_MAX and CYC_PER_TICK at driver init to avoid runtime double divisions */
static uint64_t cycles_max;
static uint32_t cyc_per_tick;
#define CYCLES_MAX   cycles_max
#define CYC_PER_TICK cyc_per_tick

static void ostm_irq_handler(timer_callback_args_t *arg);
void gtm_int_isr(IRQn_Type const irq);

const struct device *g_os_timer_dev = DEVICE_DT_INST_GET(0);
extern unsigned int z_clock_hw_cycles_per_sec;

struct rz_os_timer_config {
	const timer_api_t *fsp_api;
};

struct rz_os_timer_data {
	timer_cfg_t *fsp_cfg;
	timer_ctrl_t *fsp_ctrl;
	struct k_spinlock lock;
	uint32_t last_cycle;
	uint32_t last_tick;
	uint32_t last_elapsed;
};

void rz_os_timer_gtm_isr(const struct device *dev)
{
	struct rz_os_timer_data *data = dev->data;

	gtm_int_isr(data->fsp_cfg->cycle_end_irq);
}

static void ostm_irq_handler(timer_callback_args_t *arg)
{
	ARG_UNUSED(arg);

	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	uint32_t delta_cycles = sys_clock_cycle_get_32() - data->last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	data->last_cycle += delta_ticks * CYC_PER_TICK;
	data->last_tick += delta_ticks;
	data->last_elapsed = 0;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		struct rz_os_timer_config *config =
			(struct rz_os_timer_config *)g_os_timer_dev->config;
		uint32_t next_cycle = data->last_cycle + CYC_PER_TICK;

		config->fsp_api->periodSet(data->fsp_ctrl, next_cycle);
	} else {
		irq_disable(DT_IRQN(TIMER_NODE));
	}
	k_spin_unlock(&data->lock, key);

	/* Announce to the kernel */
	sys_clock_announce(delta_ticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (idle && ticks == K_TICKS_FOREVER) {
		return;
	}

	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;
	uint32_t next_cycle;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (ticks == K_TICKS_FOREVER) {
		next_cycle = data->last_cycle + CYCLES_MAX;
	} else {
		next_cycle = (data->last_tick + data->last_elapsed + ticks) * CYC_PER_TICK;
		if ((next_cycle - data->last_cycle) > CYCLES_MAX) {
			next_cycle = data->last_cycle + CYCLES_MAX;
		}
	}

	config->fsp_api->periodSet(data->fsp_ctrl, next_cycle);
	irq_enable(DT_IRQN(TIMER_NODE));

	k_spin_unlock(&data->lock, key);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	uint32_t delta_cycles = sys_clock_cycle_get_32() - data->last_cycle;
	uint32_t delta_ticks = delta_cycles / CYC_PER_TICK;

	data->last_elapsed = delta_ticks;
	k_spin_unlock(&data->lock, key);

	return delta_ticks;
}

void sys_clock_disable(void)
{
	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;

	config->fsp_api->close(data->fsp_ctrl);
}

uint32_t sys_clock_cycle_get_32(void)
{
	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;
	timer_status_t timer_status;

	config->fsp_api->statusGet(data->fsp_ctrl, &timer_status);

	return timer_status.counter;
}

#ifdef CONFIG_CPU_CORTEX_M
#define RZ_GTM_GET_IRQ_FLAGS(irq_name) 0
#else /* Cortex-A/R */
#define RZ_GTM_GET_IRQ_FLAGS(irq_name) DT_IRQ_BY_NAME(TIMER_NODE, irq_name, flags)
#endif

static int sys_clock_driver_init(void)
{
	fsp_err_t ret;
	struct rz_os_timer_config *config = (struct rz_os_timer_config *)g_os_timer_dev->config;
	struct rz_os_timer_data *data = (struct rz_os_timer_data *)g_os_timer_dev->data;

	IRQ_CONNECT(DT_IRQN(TIMER_NODE), DT_IRQ(TIMER_NODE, priority), rz_os_timer_gtm_isr,
		    DEVICE_DT_INST_GET(0), RZ_GTM_GET_IRQ_FLAGS(overflow));

	data->last_tick = 0;
	data->last_cycle = 0;
	z_clock_hw_cycles_per_sec = R_FSP_SystemClockHzGet(FSP_PRIV_CLOCK_P0CLK);
	cyc_per_tick = sys_clock_hw_cycles_per_sec() / CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	cycles_max = CYCLES_MAX_5;
	data->fsp_cfg->period_counts = CYC_PER_TICK;
	ret = config->fsp_api->open(data->fsp_ctrl, data->fsp_cfg);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("timer initialize failed");
		return -EIO;
	}

	ret = config->fsp_api->start(data->fsp_ctrl);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("timer start failed");
		return -EIO;
	}

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
		.fsp_cfg = &g_timer0_cfg,                                                          \
		.fsp_ctrl = (timer_ctrl_t *)&g_timer0_ctrl,                                        \
	};                                                                                         \
                                                                                                   \
	struct rz_os_timer_config g_rz_os_timer_config = {                                         \
		.fsp_api = &g_timer_on_gtm,                                                        \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(0, NULL, NULL, &g_rz_os_timer_data, &g_rz_os_timer_config,           \
			      PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY, NULL);              \
                                                                                                   \
	SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);

OS_TIMER_RZG_GTM_INIT();
