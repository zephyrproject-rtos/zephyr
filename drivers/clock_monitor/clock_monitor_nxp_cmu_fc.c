/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/*
 * Zephyr clock_monitor back-end for NXP CMU_FC (Frequency Check).
 */

#define DT_DRV_COMPAT nxp_cmu_fc

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_cmu_fc.h>

#include "clock_monitor_common.h"

LOG_MODULE_REGISTER(clock_monitor_nxp_cmu_fc,
		    CONFIG_CLOCK_MONITOR_LOG_LEVEL);

/* Module positive/negative variation in monitored cycles. */
#define CMU_FC_VAR_POS 3
#define CMU_FC_VAR_NEG 3

/* LFREF absolute minimum and HFREF absolute maximum per register layout. */
#define CMU_FC_LFREF_MIN 0x3U
#define CMU_FC_HFREF_MAX 0x00FFFFFCU

struct nxp_cmu_fc_config {
	CMU_FC_Type *base;
	const struct device    *ref_clk_dev;
	clock_control_subsys_t  ref_clk_subsys;
	const struct device    *mon_clk_dev;
	clock_control_subsys_t  mon_clk_subsys;
	/* true = route SR[FHH/FLL] through the async IER bits;
	 * false = route through the sync IER bits
	 * (NVIC / driver ISR / user callback). From DT property
	 * fault-reaction.
	 */
	bool async_reaction;
	void (*irq_config_func)(const struct device *dev);
};

enum nxp_cmu_fc_state {
	NXP_CMU_FC_STATE_IDLE = 0,
	/* configure() is between its lock-free clock query / HAL init and
	 * its commit; concurrent configure()/start() get -EBUSY.
	 */
	NXP_CMU_FC_STATE_CONFIGURING,
	NXP_CMU_FC_STATE_CONFIGURED,
	NXP_CMU_FC_STATE_RUNNING,
};

struct nxp_cmu_fc_data {
	/* Protects the state machine and cfg against the ISR and against
	 * concurrent API calls.
	 */
	struct k_spinlock lock;
	enum nxp_cmu_fc_state state;
	struct clock_monitor_config cfg;
	uint32_t ref_hz;            /* cached from clock_control at configure */
	uint32_t ref_cnt;
};

/*
 * Compute HFREF / LFREF from the user's expected_hz + tolerance_ppm.
 */
static int compute_thresholds(uint32_t expected_hz, uint32_t tol_ppm,
			      uint32_t ref_cnt, uint32_t ref_hz,
			      uint32_t *hfref, uint32_t *lfref)
{
	uint64_t num_max = (uint64_t)expected_hz * (1000000ULL + tol_ppm);
	uint64_t num_min = (tol_ppm >= 1000000U)
		? 0ULL
		: (uint64_t)expected_hz * (1000000ULL - tol_ppm);
	uint64_t den = 1000000ULL * (uint64_t)ref_hz;

	/* HFREF is ceil of (f_max * ref_cnt / f_ref) + CMU_FC_VAR_POS */
	uint64_t h = (num_max * (uint64_t)ref_cnt + den - 1ULL) / den
		     + CMU_FC_VAR_POS;

	if (h > CMU_FC_HFREF_MAX) {
		LOG_ERR("HFREF %llu exceeds HW max %u "
			"(expected_hz=%u, tol_ppm=%u, ref_cnt=%u, ref_hz=%u)",
			(uint64_t)h, (uint32_t)CMU_FC_HFREF_MAX,
			expected_hz, tol_ppm, ref_cnt, ref_hz);
		return -ERANGE;
	}
	*hfref = (uint32_t)h;

	/* LFREF is floor of (f_min * ref_cnt / f_ref) - CMU_FC_VAR_NEG */
	uint64_t l_floor = (num_min * (uint64_t)ref_cnt) / den;
	uint32_t l = (l_floor > CMU_FC_VAR_NEG)
		? (uint32_t)(l_floor - CMU_FC_VAR_NEG)
		: 0U;

	if (l < CMU_FC_LFREF_MIN) {
		LOG_ERR("LFREF %u below HW min %u "
			"(expected_hz=%u, tol_ppm=%u, ref_cnt=%u, ref_hz=%u)",
			l, (uint32_t)CMU_FC_LFREF_MIN,
			expected_hz, tol_ppm, ref_cnt, ref_hz);
		return -ERANGE;
	}
	*lfref = l;
	return 0;
}

static int nxp_cmu_fc_configure(const struct device *dev,
				const struct clock_monitor_config *cfg)
{
	const struct nxp_cmu_fc_config *config = dev->config;
	struct nxp_cmu_fc_data *data = dev->data;
	k_spinlock_key_t key;
	enum nxp_cmu_fc_state prev_state;
	int ret;

	/* Input-only validation, no state needed. */
	if (cfg->mode != CLOCK_MONITOR_MODE_WINDOW) {
		return -ENOTSUP;
	}

	/* tolerance_ppm enters compute_thresholds() as
	 * (1000000 + tol) and (1000000 - tol). Values >= 1e6 collapse the
	 * lower-bound numerator to zero (no useful LFREF) and inflate the
	 * upper-bound multiplier into nonsensical thresholds, so reject
	 * them up front rather than producing a misconfigured monitor.
	 */
	if (cfg->window.tolerance_ppm >= 1000000U) {
		return -EINVAL;
	}

	if (cfg->window.window_ns == 0U) {
		return -EINVAL;
	}

	/* Claim the configure transaction: CONFIGURING makes concurrent
	 * configure()/start() callers fail with -EBUSY while the
	 * clock queries and HAL init below run outside the spinlock.
	 */
	key = k_spin_lock(&data->lock);
	if (data->state == NXP_CMU_FC_STATE_RUNNING ||
	    data->state == NXP_CMU_FC_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	prev_state = data->state;
	data->state = NXP_CMU_FC_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	/* Query reference clock rate from clock_control for this configure. */
	uint32_t ref_hz = 0U;
	int cc_ret = clock_control_get_rate(config->ref_clk_dev,
					    config->ref_clk_subsys, &ref_hz);

	if (cc_ret != 0 || ref_hz == 0U) {
		ret = -EIO;
		goto restore;
	}

	uint32_t expected_hz = cfg->window.expected_hz;

	if (expected_hz == 0U) {
		/* Auto-derive from the monitored clock. */
		cc_ret = clock_control_get_rate(config->mon_clk_dev,
						config->mon_clk_subsys,
						&expected_hz);
		if (cc_ret != 0 || expected_hz == 0U) {
			ret = -EINVAL;
			goto restore;
		}
	}

	cmu_fc_config_t hal_cfg;

	CMU_FC_GetDefaultConfig(&hal_cfg);

	uint32_t ref_cnt;

	ret = clock_monitor_compute_ref_cnt(cfg->window.window_ns, ref_hz,
					    CMU_FC_RCCR_REF_CNT_MASK, &ref_cnt);
	if (ret != 0) {
		LOG_ERR("ref_cnt out of range: window_ns=%u, ref_hz=%u "
			"(HW max %u)", cfg->window.window_ns, ref_hz,
			(uint32_t)CMU_FC_RCCR_REF_CNT_MASK);
		ret = -EINVAL;
		goto restore;
	}

	ret = compute_thresholds(expected_hz, cfg->window.tolerance_ppm,
				 ref_cnt, ref_hz,
				 &hal_cfg.highThresholdCnt,
				 &hal_cfg.lowThresholdCnt);
	if (ret != 0) {
		/* Thresholds out of HW range for the given params. */
		ret = -EINVAL;
		goto restore;
	}
	hal_cfg.refClockCount = ref_cnt;

	/* The sync path needs the NVIC IRQ wired; the async path does not
	 * require an NVIC IRQ entry in DT.
	 */
	const bool path_available =
		config->async_reaction || (config->irq_config_func != NULL);
	const uint32_t high_ie = config->async_reaction
		? (uint32_t)kCMU_FC_HigherThanHighThrAsyncInterruptEnable
		: (uint32_t)kCMU_FC_HigherThanHighThrInterruptEnable;
	const uint32_t low_ie = config->async_reaction
		? (uint32_t)kCMU_FC_LowerThanLowThrAsyncInterruptEnable
		: (uint32_t)kCMU_FC_LowerThanLowThrInterruptEnable;

	hal_cfg.interruptEnable = 0U;
	if (path_available) {
		hal_cfg.interruptEnable |= high_ie | low_ie;
	}

	status_t st = CMU_FC_Init(config->base, &hal_cfg);

	if (st != kStatus_Success) {
		/* Hardware state is unknown after a failed init. */
		key = k_spin_lock(&data->lock);
		data->state = NXP_CMU_FC_STATE_IDLE;
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}

	key = k_spin_lock(&data->lock);
	data->cfg = *cfg;
	data->ref_hz = ref_hz;
	data->ref_cnt = ref_cnt;
	data->state = NXP_CMU_FC_STATE_CONFIGURED;
	k_spin_unlock(&data->lock, key);
	return 0;

restore:
	/* Hardware untouched: the previous configuration is still valid. */
	key = k_spin_lock(&data->lock);
	data->state = prev_state;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int nxp_cmu_fc_start(const struct device *dev)
{
	const struct nxp_cmu_fc_config *config = dev->config;
	struct nxp_cmu_fc_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_CMU_FC_STATE_RUNNING ||
	    data->state == NXP_CMU_FC_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	if (data->state != NXP_CMU_FC_STATE_CONFIGURED) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/* State transition and HW kick share one critical section so the
	 * ISR can never observe running hardware with a not-yet-RUNNING
	 * state machine.
	 */
	data->state = NXP_CMU_FC_STATE_RUNNING;
	CMU_FC_StartFreqChecking(config->base);
	k_spin_unlock(&data->lock, key);
	return 0;
}

/*
 * Spinlock-only: ISR-safe and callable from the event callback.
 * Everything touched (HW stop bit + state) is spinlock-protected
 * against the ISR.
 */
static int nxp_cmu_fc_stop(const struct device *dev)
{
	const struct nxp_cmu_fc_config *config = dev->config;
	struct nxp_cmu_fc_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_CMU_FC_STATE_RUNNING) {
		CMU_FC_StopFreqChecking(config->base);
		data->state = NXP_CMU_FC_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);
	return 0;
}

static void nxp_cmu_fc_isr(const struct device *dev)
{
	const struct nxp_cmu_fc_config *config = dev->config;
	struct nxp_cmu_fc_data *data = dev->data;
	uint32_t flags = CMU_FC_GetStatusFlags(config->base);
	uint32_t evts = 0U;
	struct clock_monitor_event_data cb_data = {0};
	clock_monitor_callback_t cb = NULL;
	void *user_data = NULL;

	CMU_FC_ClearStatusFlags(config->base, flags);

	if ((flags & (uint32_t)kCMU_FC_HigherThanHighThr) != 0U) {
		evts |= CLOCK_MONITOR_EVT_FREQ_HIGH;
	}
	if ((flags & (uint32_t)kCMU_FC_LowerThanLowThr) != 0U) {
		evts |= CLOCK_MONITOR_EVT_FREQ_LOW;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* A threshold event can be left pending while stop() holds the
	 * lock; by the time it runs the monitor is no longer RUNNING.
	 * Discard it so no callback is delivered after stop().
	 */
	if (data->state != NXP_CMU_FC_STATE_RUNNING) {
		evts = 0U;
	}

	cb_data.events = evts;
	cb = data->cfg.callback;
	user_data = data->cfg.user_data;

	k_spin_unlock(&data->lock, key);

	if (cb != NULL && cb_data.events != 0U) {
		cb(dev, &cb_data, user_data);
	}
}

static int nxp_cmu_fc_init(const struct device *dev)
{
	const struct nxp_cmu_fc_config *config = dev->config;
	struct nxp_cmu_fc_data *data = dev->data;

	data->state = NXP_CMU_FC_STATE_IDLE;

	if (config->async_reaction) {
		LOG_INF("%s: fault-reaction = reset (destructive reset)",
			dev->name);
	}

	if (config->irq_config_func != NULL) {
		config->irq_config_func(dev);
		if (!config->async_reaction) {
			LOG_INF("%s: fault-reaction = interrupt (NVIC)",
				dev->name);
		}
	} else if (!config->async_reaction) {
		LOG_WRN("%s: fault-reaction = interrupt but no NVIC IRQ in DT; "
			"events will not be delivered", dev->name);
	} else {
		/* No actions taken. */
	}

	return 0;
}

static DEVICE_API(clock_monitor, nxp_cmu_fc_api) = {
	.configure = nxp_cmu_fc_configure,
	.start     = nxp_cmu_fc_start,
	.stop      = nxp_cmu_fc_stop,
};

#define NXP_CMU_FC_IRQ_WIRE(inst)                                              \
	static void nxp_cmu_fc_irq_cfg_##inst(const struct device *dev)        \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),   \
			    nxp_cmu_fc_isr, DEVICE_DT_INST_GET(inst), 0);      \
		irq_enable(DT_INST_IRQN(inst));                                \
	}

#define NXP_CMU_FC_IRQ_PTR(inst)                                               \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(inst, 0),                              \
		    (nxp_cmu_fc_irq_cfg_##inst), (NULL))

#define NXP_CMU_FC_DEVICE_INIT(inst)                                           \
	COND_CODE_1(DT_INST_IRQ_HAS_IDX(inst, 0),                              \
		    (NXP_CMU_FC_IRQ_WIRE(inst)), ())                           \
	static struct nxp_cmu_fc_data nxp_cmu_fc_data_##inst;                  \
	static const struct nxp_cmu_fc_config nxp_cmu_fc_cfg_##inst = {        \
		.base = (CMU_FC_Type *)DT_INST_REG_ADDR(inst),                 \
		.ref_clk_dev = DEVICE_DT_GET(                                  \
			DT_INST_CLOCKS_CTLR_BY_NAME(inst, reference)),         \
		.ref_clk_subsys = (clock_control_subsys_t)(uintptr_t)          \
			DT_INST_CLOCKS_CELL_BY_NAME(inst, reference, name),    \
		.mon_clk_dev = DEVICE_DT_GET(                                  \
			DT_INST_CLOCKS_CTLR_BY_NAME(inst, monitored)),         \
		.mon_clk_subsys = (clock_control_subsys_t)(uintptr_t)          \
			DT_INST_CLOCKS_CELL_BY_NAME(inst, monitored, name),    \
		.async_reaction =                                              \
			DT_INST_ENUM_IDX_OR(inst, fault_reaction, 0) == 1, \
		.irq_config_func = NXP_CMU_FC_IRQ_PTR(inst),                   \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_cmu_fc_init, NULL,                     \
			      &nxp_cmu_fc_data_##inst,                         \
			      &nxp_cmu_fc_cfg_##inst,                          \
			      POST_KERNEL,                                     \
			      CONFIG_CLOCK_MONITOR_INIT_PRIORITY,              \
			      &nxp_cmu_fc_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_CMU_FC_DEVICE_INIT)
