/*
 * Copyright (c) 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for NXP CMU_FC (Frequency Check).
 *
 * Wraps the MCUX SDK fsl_cmu_fc HAL. Supports WINDOW mode only.
 * Uses its own ISR wired via IRQ_CONNECT (not CMU_FC_RegisterCallBack,
 * whose callback does not carry a context pointer).
 *
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

LOG_MODULE_REGISTER(clock_monitor_nxp_cmu_fc,
		    CONFIG_CLOCK_MONITOR_LOG_LEVEL);

/* RM section 53.4.2: module positive/negative variation in monitored cycles. */
#define CMU_FC_VAR_POS 3
#define CMU_FC_VAR_NEG 3

/* LFREF absolute minimum and HFREF absolute maximum per register layout. */
#define CMU_FC_LFREF_MIN 0x3U
#define CMU_FC_HFREF_MAX 0x00FFFFFCU

struct nxp_cmu_fc_config {
	struct clock_monitor_common_config common; /* MUST be first */
	CMU_FC_Type *base;
	const struct device    *ref_clk_dev;
	clock_control_subsys_t  ref_clk_subsys;
	const struct device    *mon_clk_dev;
	clock_control_subsys_t  mon_clk_subsys;
	/* true = route SR[FHH/FLL] through the async IER bits;
	 * false = route through the sync IER bits
	 * (NVIC / driver ISR / user callback). From DT property
	 * nxp,fault-reaction.
	 */
	bool async_reaction;
	void (*irq_config_func)(const struct device *dev);
};

enum nxp_cmu_fc_state {
	NXP_CMU_FC_STATE_IDLE = 0,
	NXP_CMU_FC_STATE_CONFIGURED,
	NXP_CMU_FC_STATE_RUNNING,
};

struct nxp_cmu_fc_data {
	struct k_spinlock lock;
	enum nxp_cmu_fc_state state;
	struct clock_monitor_config cfg;
	uint32_t ref_hz;            /* cached from clock_control at configure */
	uint32_t ref_cnt;
	uint32_t latched_events;
};

static uint32_t compute_ref_cnt(uint32_t window_ns, uint32_t ref_hz)
{
	uint64_t cnt = (uint64_t)window_ns * (uint64_t)ref_hz / 1000000000ULL;

	if (cnt < 1U) {
		cnt = 1U;
	}
	if (cnt > CMU_FC_RCCR_REF_CNT_MASK) {
		cnt = CMU_FC_RCCR_REF_CNT_MASK;
	}
	return (uint32_t)cnt;
}

/*
 * Compute HFREF / LFREF from the user's expected_hz + tolerance_ppm.
 * Reference clock deviation is taken as zero in this stage; integrating
 * it would require querying clock_control for the ref clock's specified
 * variation, which is Stage 3 work.
 */
static void compute_thresholds(uint32_t expected_hz, uint32_t tol_ppm,
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
		h = CMU_FC_HFREF_MAX;
	}
	*hfref = (uint32_t)h;

	/* LFREF is floor of (f_min * ref_cnt / f_ref) - CMU_FC_VAR_NEG */
	uint64_t l_floor = (num_min * (uint64_t)ref_cnt) / den;
	uint32_t l = (l_floor > CMU_FC_VAR_NEG)
		? (uint32_t)(l_floor - CMU_FC_VAR_NEG)
		: 0U;

	if (l < CMU_FC_LFREF_MIN) {
		l = CMU_FC_LFREF_MIN;
	}
	*lfref = l;
}

static int nxp_cmu_fc_configure(const struct device *dev,
				const struct clock_monitor_config *cfg)
{
	const struct nxp_cmu_fc_config *config = dev->config;
	struct nxp_cmu_fc_data *data = dev->data;
	k_spinlock_key_t key;

	if (cfg->mode != CLOCK_MONITOR_MODE_WINDOW &&
	    cfg->mode != CLOCK_MONITOR_MODE_DISABLED) {
		return -EINVAL;
	}

	/* tolerance_ppm enters compute_thresholds() as
	 * (1000000 + tol) and (1000000 - tol). Values >= 1e6 collapse the
	 * lower-bound numerator to zero (no useful LFREF) and inflate the
	 * upper-bound multiplier into nonsensical thresholds, so reject
	 * them up front rather than producing a misconfigured monitor.
	 */
	if (cfg->mode == CLOCK_MONITOR_MODE_WINDOW &&
	    cfg->window.tolerance_ppm >= 1000000U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_CMU_FC_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	if (cfg->mode == CLOCK_MONITOR_MODE_DISABLED) {
		CMU_FC_Deinit(config->base);
		data->cfg = *cfg;
		data->latched_events = 0U;
		data->state = NXP_CMU_FC_STATE_IDLE;
		k_spin_unlock(&data->lock, key);
		return 0;
	}
	k_spin_unlock(&data->lock, key);

	/* Query reference clock rate from clock_control for this configure. */
	uint32_t ref_hz = 0U;
	int cc_ret = clock_control_get_rate(config->ref_clk_dev,
					    config->ref_clk_subsys, &ref_hz);

	if (cc_ret != 0 || ref_hz == 0U) {
		return -EIO;
	}

	/* WINDOW mode. */
	uint32_t expected_hz = cfg->window.expected_hz;

	if (expected_hz == 0U) {
		/* Auto-derive from the monitored clock. */
		cc_ret = clock_control_get_rate(config->mon_clk_dev,
						config->mon_clk_subsys,
						&expected_hz);
		if (cc_ret != 0 || expected_hz == 0U) {
			return -EINVAL;
		}
	}

	cmu_fc_config_t hal_cfg;

	CMU_FC_GetDefaultConfig(&hal_cfg);

	uint32_t ref_cnt = compute_ref_cnt(cfg->window.window_ns, ref_hz);

	compute_thresholds(expected_hz, cfg->window.tolerance_ppm,
			   ref_cnt, ref_hz,
			   &hal_cfg.highThresholdCnt,
			   &hal_cfg.lowThresholdCnt);
	hal_cfg.refClockCount = ref_cnt;

	/* Sync path needs NVIC wired; async path and
	 * does not require an NVIC IRQ entry in DT.
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
	if (path_available &&
	    (cfg->window.event_mask & CLOCK_MONITOR_EVT_FREQ_HIGH)) {
		hal_cfg.interruptEnable |= high_ie;
	}
	if (path_available &&
	    (cfg->window.event_mask & CLOCK_MONITOR_EVT_FREQ_LOW)) {
		hal_cfg.interruptEnable |= low_ie;
	}

	status_t st = CMU_FC_Init(config->base, &hal_cfg);

	key = k_spin_lock(&data->lock);
	if (st != kStatus_Success) {
		data->state = NXP_CMU_FC_STATE_IDLE;
		k_spin_unlock(&data->lock, key);
		return -EIO;
	}
	data->cfg = *cfg;
	data->latched_events = 0U;
	data->ref_hz = ref_hz;
	data->ref_cnt = ref_cnt;
	data->state = NXP_CMU_FC_STATE_CONFIGURED;
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_cmu_fc_check(const struct device *dev)
{
	const struct nxp_cmu_fc_config *config = dev->config;
	struct nxp_cmu_fc_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->state != NXP_CMU_FC_STATE_CONFIGURED) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}
	if (data->cfg.mode != CLOCK_MONITOR_MODE_WINDOW) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	CMU_FC_StartFreqChecking(config->base);
	data->state = NXP_CMU_FC_STATE_RUNNING;
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_cmu_fc_stop(const struct device *dev)
{
	const struct nxp_cmu_fc_config *config = dev->config;
	struct nxp_cmu_fc_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (data->state == NXP_CMU_FC_STATE_RUNNING) {
		CMU_FC_StopFreqChecking(config->base);
		data->state = NXP_CMU_FC_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_cmu_fc_measure(const struct device *dev, uint32_t *rate_hz,
			      k_timeout_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rate_hz);
	ARG_UNUSED(timeout);

	return -ENOSYS;
}

static int nxp_cmu_fc_get_events(const struct device *dev, uint32_t *events)
{
	struct nxp_cmu_fc_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	*events = data->latched_events;
	data->latched_events = 0U;
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

	data->latched_events |= evts;
	cb_data.events = evts & data->cfg.window.event_mask;
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
	.configure  = nxp_cmu_fc_configure,
	.check      = nxp_cmu_fc_check,
	.stop       = nxp_cmu_fc_stop,
	.measure    = nxp_cmu_fc_measure,
	.get_events = nxp_cmu_fc_get_events,
};

/* ---------- Instantiation ---------- */

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
	static const struct clock_monitor_capabilities                         \
		nxp_cmu_fc_caps_##inst = {                                     \
		.supported_modes  = BIT(CLOCK_MONITOR_MODE_WINDOW),             \
		.supported_events = CLOCK_MONITOR_EVT_FREQ_HIGH |              \
				    CLOCK_MONITOR_EVT_FREQ_LOW,                \
	};                                                                     \
	static struct nxp_cmu_fc_data nxp_cmu_fc_data_##inst;                  \
	static const struct nxp_cmu_fc_config nxp_cmu_fc_cfg_##inst = {        \
		.common = {.caps = &nxp_cmu_fc_caps_##inst},                   \
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
			DT_INST_ENUM_IDX_OR(inst, nxp_fault_reaction, 0) == 1, \
		.irq_config_func = NXP_CMU_FC_IRQ_PTR(inst),                   \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_cmu_fc_init, NULL,                     \
			      &nxp_cmu_fc_data_##inst,                         \
			      &nxp_cmu_fc_cfg_##inst,                          \
			      POST_KERNEL,                                     \
			      CONFIG_CLOCK_MONITOR_INIT_PRIORITY,              \
			      &nxp_cmu_fc_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_CMU_FC_DEVICE_INIT)
