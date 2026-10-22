/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for NXP CMU_FM (Frequency Meter).
 *
 */

#define DT_DRV_COMPAT nxp_cmu_fm

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_cmu_fm.h>

#include "clock_monitor_common.h"

LOG_MODULE_REGISTER(clock_monitor_nxp_cmu_fm,
		    CONFIG_CLOCK_MONITOR_LOG_LEVEL);

struct nxp_cmu_fm_config {
	CMU_FM_Type *base;
	const struct device    *ref_clk_dev;
	clock_control_subsys_t  ref_clk_subsys;
	const struct device    *mon_clk_dev;
	clock_control_subsys_t  mon_clk_subsys;
	void (*irq_config_func)(const struct device *dev);
};

enum nxp_cmu_fm_state {
	NXP_CMU_FM_STATE_IDLE = 0,
	/* configure() is between its lock-free clock query / HAL init and
	 * its commit; concurrent configure()/start() get -EBUSY.
	 */
	NXP_CMU_FM_STATE_CONFIGURING,
	NXP_CMU_FM_STATE_CONFIGURED,
	NXP_CMU_FM_STATE_RUNNING,
};

struct nxp_cmu_fm_data {
	/* Protects the state machine, cfg and result fields against the
	 * ISR and against concurrent API calls.
	 */
	struct k_spinlock lock;
	enum nxp_cmu_fm_state state;
	struct clock_monitor_config cfg;
	uint32_t ref_hz;            /* cached from clock_control at configure */
	uint32_t ref_cnt;
	/* Most recent completed measurement since configure(); retained
	 * across reads for get_rate().
	 */
	uint32_t last_rate_hz;
	bool has_rate;
	/* CLOCK_LOST latched since configure(). */
	bool clock_lost;
};

static int nxp_cmu_fm_configure(const struct device *dev,
				const struct clock_monitor_config *cfg)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;
	k_spinlock_key_t key;
	enum nxp_cmu_fm_state prev_state;
	int ret;

	/* Input-only validation, no state needed. */
	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE) {
		return -ENOTSUP;
	}

	if (cfg->measure.window_ns == 0U) {
		return -EINVAL;
	}

	/* Claim the configure transaction: CONFIGURING makes concurrent
	 * configure()/start() callers fail with -EBUSY while the
	 * clock query and HAL init below run outside the spinlock.
	 */
	key = k_spin_lock(&data->lock);
	if (data->state == NXP_CMU_FM_STATE_RUNNING ||
	    data->state == NXP_CMU_FM_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	prev_state = data->state;
	data->state = NXP_CMU_FM_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	/* Query reference clock rate from clock_control for this configure. */
	uint32_t ref_hz = 0U;
	int cc_ret = clock_control_get_rate(config->ref_clk_dev,
					    config->ref_clk_subsys, &ref_hz);

	if (cc_ret != 0 || ref_hz == 0U) {
		ret = -EIO;
		goto restore;
	}

	uint32_t ref_cnt;

	ret = clock_monitor_compute_ref_cnt(cfg->measure.window_ns, ref_hz,
					    CMU_FM_RCCR_REF_CNT_MASK, &ref_cnt);
	if (ret != 0) {
		LOG_ERR("ref_cnt out of range: window_ns=%u, ref_hz=%u "
			"(HW max %u)", cfg->measure.window_ns, ref_hz,
			(uint32_t)CMU_FM_RCCR_REF_CNT_MASK);
		ret = -EINVAL;
		goto restore;
	}

	cmu_fm_config_t hal_cfg;

	CMU_FM_GetDefaultConfig(&hal_cfg);
	hal_cfg.refClockCount = ref_cnt;
	hal_cfg.enableInterrupt = true;

	status_t st = CMU_FM_Init(config->base, &hal_cfg);

	if (st != kStatus_Success) {
		/* Hardware state is unknown after a failed init. */
		key = k_spin_lock(&data->lock);
		data->state = NXP_CMU_FM_STATE_IDLE;
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	key = k_spin_lock(&data->lock);
	data->cfg = *cfg;
	data->ref_hz = ref_hz;
	data->ref_cnt = ref_cnt;
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	data->state = NXP_CMU_FM_STATE_CONFIGURED;
	k_spin_unlock(&data->lock, key);
	return 0;

restore:
	/* Hardware untouched: the previous configuration is still valid. */
	key = k_spin_lock(&data->lock);
	data->state = prev_state;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int nxp_cmu_fm_start(const struct device *dev)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_CMU_FM_STATE_RUNNING ||
	    data->state == NXP_CMU_FM_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	if (data->state != NXP_CMU_FM_STATE_CONFIGURED) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/* State transition and HW kick share one critical section so the
	 * completion ISR can never observe RUNNING hardware with a
	 * not-yet-RUNNING state machine.
	 */
	data->state = NXP_CMU_FM_STATE_RUNNING;
	CMU_FM_StartFreqMetering(config->base);
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_cmu_fm_stop(const struct device *dev)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_CMU_FM_STATE_RUNNING) {
		CMU_FM_StopFreqMetering(config->base);
		data->state = NXP_CMU_FM_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);
	return 0;
}

static uint32_t nxp_cmu_fm_latch_result(const struct device *dev,
					uint32_t flags, uint32_t *rate_out)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;
	uint32_t evts = 0U;
	uint32_t rate = 0U;

	/* A completion interrupt can be left pending while stop() (or a
	 * subsequent configure()) holds the lock; by the time it runs the
	 * device is no longer RUNNING. Discard it: stop() aborts the
	 * in-flight measurement, and the state machine must not be
	 * modified from here in that case.
	 */
	if (data->state != NXP_CMU_FM_STATE_RUNNING) {
		return 0U;
	}

	if ((flags & (uint32_t)kCMU_FM_MeterComplete) != 0U) {
		uint32_t met_cnt = CMU_FM_GetMeteredClkCnt(config->base);

		rate = CMU_FM_CalcMeteredClkFreq(met_cnt, data->ref_cnt,
						 data->ref_hz);
		data->last_rate_hz = rate;
		data->has_rate = true;
		evts |= CLOCK_MONITOR_EVT_MEASURE_DONE;
	}

	if ((flags & (uint32_t)kCMU_FM_MeterTimeout) != 0U) {
		data->clock_lost = true;
		evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
	}

	/* Auto-disarm precedes the callback so the user may restart from
	 * the callback itself or reconfigure/restart from a thread.
	 */
	if (evts != 0U) {
		data->state = NXP_CMU_FM_STATE_CONFIGURED;
	}

	if (rate_out != NULL) {
		*rate_out = rate;
	}
	return evts;
}

/* Pure cached read of the ISR-latched result: the binding requires an
 * NVIC interrupt, so completion detection is always ISR-driven.
 */
static int nxp_cmu_fm_get_rate(const struct device *dev, uint32_t *rate_hz)
{
	struct nxp_cmu_fm_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&data->lock);
	if (data->clock_lost) {
		ret = -EIO;
	} else if (data->has_rate) {
		*rate_hz = data->last_rate_hz;
		ret = 0;
	} else {
		ret = -EAGAIN;
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static void nxp_cmu_fm_isr(const struct device *dev)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;
	uint32_t flags = CMU_FM_GetStatusFlags(config->base);
	uint32_t rate = 0U;
	uint32_t evts;
	clock_monitor_callback_t cb = NULL;
	void *user_data = NULL;
	k_spinlock_key_t key;

	CMU_FM_ClearStatusFlags(config->base, flags);

	key = k_spin_lock(&data->lock);
	evts = nxp_cmu_fm_latch_result(dev, flags, &rate);
	if (evts != 0U) {
		cb = data->cfg.callback;
		user_data = data->cfg.user_data;
	}
	k_spin_unlock(&data->lock, key);

	/* The callback runs outside the spinlock so it may legally call
	 * clock_monitor_start() to begin the next measurement (the
	 * auto-disarm above already returned the device to the
	 * configured state) or clock_monitor_stop().
	 */
	if (cb != NULL && evts != 0U) {
		struct clock_monitor_event_data evt = {
			.events      = evts,
			.measured_hz = rate,
		};
		cb(dev, &evt, user_data);
	}
}

static int nxp_cmu_fm_init(const struct device *dev)
{
	const struct nxp_cmu_fm_config *config = dev->config;
	struct nxp_cmu_fm_data *data = dev->data;

	data->state = NXP_CMU_FM_STATE_IDLE;

	config->irq_config_func(dev);
	return 0;
}

static DEVICE_API(clock_monitor, nxp_cmu_fm_api) = {
	.configure = nxp_cmu_fm_configure,
	.start     = nxp_cmu_fm_start,
	.stop      = nxp_cmu_fm_stop,
	.get_rate  = nxp_cmu_fm_get_rate,
};

/* The nxp,cmu-fm binding requires `interrupts`; wire it unconditionally. */
#define NXP_CMU_FM_IRQ_WIRE(inst)                                              \
	static void nxp_cmu_fm_irq_cfg_##inst(const struct device *dev)        \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),   \
			    nxp_cmu_fm_isr, DEVICE_DT_INST_GET(inst), 0);      \
		irq_enable(DT_INST_IRQN(inst));                                \
	}

#define NXP_CMU_FM_DEVICE_INIT(inst)                                           \
	NXP_CMU_FM_IRQ_WIRE(inst)                                              \
	static struct nxp_cmu_fm_data nxp_cmu_fm_data_##inst;                  \
	static const struct nxp_cmu_fm_config nxp_cmu_fm_cfg_##inst = {        \
		.base = (CMU_FM_Type *)DT_INST_REG_ADDR(inst),                 \
		.ref_clk_dev = DEVICE_DT_GET(                                  \
			DT_INST_CLOCKS_CTLR_BY_NAME(inst, reference)),         \
		.ref_clk_subsys = (clock_control_subsys_t)(uintptr_t)          \
			DT_INST_CLOCKS_CELL_BY_NAME(inst, reference, name),    \
		.mon_clk_dev = DEVICE_DT_GET(                                  \
			DT_INST_CLOCKS_CTLR_BY_NAME(inst, monitored)),         \
		.mon_clk_subsys = (clock_control_subsys_t)(uintptr_t)          \
			DT_INST_CLOCKS_CELL_BY_NAME(inst, monitored, name),    \
		.irq_config_func = nxp_cmu_fm_irq_cfg_##inst,                  \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_cmu_fm_init, NULL,                     \
			      &nxp_cmu_fm_data_##inst,                         \
			      &nxp_cmu_fm_cfg_##inst,                          \
			      POST_KERNEL,                                     \
			      CONFIG_CLOCK_MONITOR_INIT_PRIORITY,              \
			      &nxp_cmu_fm_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_CMU_FM_DEVICE_INIT)
