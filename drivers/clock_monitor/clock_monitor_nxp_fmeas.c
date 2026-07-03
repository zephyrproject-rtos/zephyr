/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for the NXP BASIC Frequency Measurement
 * (FREQME) block, built on the MCUX SDK fsl_fmeas HAL.
 *
 * This is the single-register FREQMECTRL. It has no interrupt line, so only
 * the clock_monitor MEASURE (one-shot) mode is supported. Completion is
 * detected by polling the MEASURE_IN_PROGRESS bit from a workqueue; when the
 * measurement finishes the configure-time callback is invoked with
 * CLOCK_MONITOR_EVT_MEASURE_DONE (or CLOCK_MONITOR_EVT_CLOCK_LOST when the
 * monitored clock produced no edges).
 *
 * The fsl_fmeas HAL fixes the reference window at 2^20 reference-clock
 * periods, so clock_monitor_measure_cfg::window_ns is accepted but not
 * honored; the effective window is HW/HAL-fixed.
 *
 * Reference and target clocks are routed through the INPUTMUX peripheral
 * (channel 0 = reference, channel 1 = target); the reference frequency is
 * queried from clock_control and the peripheral gate is enabled through
 * clock_control.
 */

#define DT_DRV_COMPAT nxp_fmeas

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_fmeas.h>
#include <fsl_inputmux.h>

LOG_MODULE_REGISTER(clock_monitor_nxp_fmeas, CONFIG_CLOCK_MONITOR_LOG_LEVEL);

/* Result counter is 31 bits; the HW formula subtracts a fixed +2 bias
 * (target edge count = RESULT - 2), so a captured value at or below 2 means
 * zero target edges were counted (clock lost).
 */
#define FMEAS_RESULT_BIAS 2U
/* The fsl_fmeas HAL measures over a fixed 2^20 reference-clock window. */
#define FMEAS_REF_WINDOW_LOG2 20U
/* Upper bound on poll iterations before a stuck MEASURE_IN_PROGRESS bit is
 * treated as a lost clock, relative to one estimated window duration.
 */
#define FMEAS_POLL_MAX_TRIES 20U

struct nxp_fmeas_inputmux_entry {
	INPUTMUX_Type *base;
	uint16_t channel;
	uint32_t connection;
};

struct nxp_fmeas_config {
	FMEAS_SYSCON_Type *base;
	/* Peripheral gate clock (clocks "ipg"), enabled via clock_control_on(). */
	const struct device    *gate_clk_dev;
	clock_control_subsys_t  gate_clk_subsys;
	/* Reference timebase clock (clocks "reference"), queried for its rate. */
	const struct device    *ref_clk_dev;
	clock_control_subsys_t  ref_clk_subsys;
	struct reset_dt_spec    reset;
	const struct nxp_fmeas_inputmux_entry *inputmux_entries;
	uint8_t inputmux_entries_count;
};

enum nxp_fmeas_state {
	NXP_FMEAS_STATE_IDLE = 0,
	/* configure()/set_source() are between their lock-free clock query /
	 * HAL work and their commit; concurrent callers get -EBUSY.
	 */
	NXP_FMEAS_STATE_CONFIGURING,
	NXP_FMEAS_STATE_CONFIGURED,
	NXP_FMEAS_STATE_RUNNING,
};

struct nxp_fmeas_data {
	/* Protects the state machine, cfg and result fields against the poll
	 * work handler and against concurrent API calls.
	 */
	struct k_spinlock lock;
	enum nxp_fmeas_state state;
	struct clock_monitor_config cfg;
	uint32_t ref_hz;      /* cached from clock_control at configure */
	uint32_t poll_us;     /* estimated window duration, poll cadence */
	uint16_t poll_tries;  /* remaining poll iterations for this cycle */
	/* Most recent completed MEASURE result; retained across reads. */
	uint32_t last_rate_hz;
	bool has_rate;
	/* CLOCK_LOST latched since configure(). */
	bool clock_lost;
	struct k_work_delayable poll_work;
	const struct device *dev;
};

static void nxp_fmeas_setup_inputmux(const struct nxp_fmeas_config *config)
{
	for (uint8_t i = 0U; i < config->inputmux_entries_count; i++) {
		const struct nxp_fmeas_inputmux_entry *e = &config->inputmux_entries[i];

		INPUTMUX_Init(e->base);
		INPUTMUX_AttachSignal(e->base, e->channel,
				      (inputmux_connection_t)e->connection);
		INPUTMUX_Deinit(e->base);
	}
}

/* Read the raw 31-bit capture value without the HAL's assert(capval >= 2). */
static uint32_t nxp_fmeas_raw_result(const struct nxp_fmeas_config *config)
{
	return ((FREQME_Type *)config->base)->FREQMECTRL_R &
	       FREQME_FREQMECTRL_R_RESULT_MASK;
}

static int nxp_fmeas_configure(const struct device *dev,
			       const struct clock_monitor_config *cfg)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;
	enum nxp_fmeas_state prev_state;
	uint32_t ref_hz = 0U;
	int ret;

	/* This back-end's hardware supports one-shot measurement only.
	 * cfg->measure.window_ns is ignored: the fsl_fmeas HAL always measures
	 * over a fixed 2^20 reference-clock window.
	 */
	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FMEAS_STATE_RUNNING ||
	    data->state == NXP_FMEAS_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	prev_state = data->state;
	data->state = NXP_FMEAS_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	ret = clock_control_get_rate(config->ref_clk_dev, config->ref_clk_subsys, &ref_hz);
	if (ret != 0 || ref_hz == 0U) {
		ret = -EIO;
		goto restore;
	}

	key = k_spin_lock(&data->lock);
	data->cfg = *cfg;
	data->ref_hz = ref_hz;
	/* Estimated cycle duration in microseconds: 2^20 / ref_hz. */
	data->poll_us = (uint32_t)(((uint64_t)BIT(FMEAS_REF_WINDOW_LOG2) *
				    1000000ULL) / ref_hz);
	if (data->poll_us == 0U) {
		data->poll_us = 1U;
	}
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	data->state = NXP_FMEAS_STATE_CONFIGURED;
	k_spin_unlock(&data->lock, key);
	return 0;

restore:
	/* Hardware untouched: the previous configuration is still valid. */
	key = k_spin_lock(&data->lock);
	data->state = prev_state;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int nxp_fmeas_start(const struct device *dev)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FMEAS_STATE_RUNNING ||
	    data->state == NXP_FMEAS_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	if (data->state != NXP_FMEAS_STATE_CONFIGURED) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	data->state = NXP_FMEAS_STATE_RUNNING;
	data->poll_tries = FMEAS_POLL_MAX_TRIES;
	FMEAS_StartMeasure(config->base);
	k_spin_unlock(&data->lock, key);

	/* First check after roughly one full window; then poll faster. */
	k_work_reschedule(&data->poll_work, K_USEC(data->poll_us));
	return 0;
}

static int nxp_fmeas_stop(const struct device *dev)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FMEAS_STATE_RUNNING) {
		/* Abort the cycle: clearing FREQMECTRL_W terminates any
		 * measurement in progress and resets the result.
		 */
		((FREQME_Type *)config->base)->FREQMECTRL_W = 0U;
		data->state = NXP_FMEAS_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);

	k_work_cancel_delayable(&data->poll_work);
	return 0;
}

static void nxp_fmeas_poll_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct nxp_fmeas_data *data = CONTAINER_OF(dwork, struct nxp_fmeas_data, poll_work);
	const struct device *dev = data->dev;
	const struct nxp_fmeas_config *config = dev->config;
	uint32_t evts = 0U;
	uint32_t rate = 0U;
	clock_monitor_callback_t cb = NULL;
	void *user_data = NULL;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	/* stop()/reconfigure() may have left the RUNNING state already. */
	if (data->state != NXP_FMEAS_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (!FMEAS_IsMeasureComplete(config->base)) {
		if (data->poll_tries != 0U) {
			data->poll_tries--;
			k_spin_unlock(&data->lock, key);
			/* Poll faster than the initial window estimate. */
			k_work_reschedule(&data->poll_work,
					  K_USEC(MAX(data->poll_us / 4U, 1U)));
			return;
		}
		/* Measurement never completed: the reference clock is not
		 * advancing the window counter.
		 */
		data->clock_lost = true;
		evts = CLOCK_MONITOR_EVT_CLOCK_LOST;
		LOG_WRN("%s: measurement timed out (FREQMECTRL_R=0x%08x, ref_hz=%u) - "
			"reference clock not running?", dev->name,
			((FREQME_Type *)config->base)->FREQMECTRL_R, data->ref_hz);
	} else {
		uint32_t result = nxp_fmeas_raw_result(config);

		/* target edge count = result - FMEAS_RESULT_BIAS, so result <= bias
		 * means zero target edges: the target clock is stuck or unrouted.
		 */
		if (result <= FMEAS_RESULT_BIAS) {
			data->clock_lost = true;
			evts = CLOCK_MONITOR_EVT_CLOCK_LOST;
			LOG_WRN("%s: no target edges counted (result=%u, ref_hz=%u) - "
				"target clock not running/routed?", dev->name,
				result, data->ref_hz);
		} else {
			rate = FMEAS_GetFrequency(config->base, data->ref_hz);
			data->last_rate_hz = rate;
			data->has_rate = true;
			data->clock_lost = false;
			evts = CLOCK_MONITOR_EVT_MEASURE_DONE;
			LOG_DBG("%s: measured %u Hz (result=%u, ref_hz=%u)",
				dev->name, rate, result, data->ref_hz);
		}
	}

	/* Auto-disarm before the callback so it may restart the cycle. */
	data->state = NXP_FMEAS_STATE_CONFIGURED;
	cb = data->cfg.callback;
	user_data = data->cfg.user_data;
	k_spin_unlock(&data->lock, key);

	if (cb != NULL) {
		struct clock_monitor_event_data evt = {
			.events = evts,
			.measured_hz = rate,
		};
		cb(dev, &evt, user_data);
	}
}

static int nxp_fmeas_get_rate(const struct device *dev, uint32_t *rate_hz)
{
	struct nxp_fmeas_data *data = dev->data;
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

static int nxp_fmeas_set_source(const struct device *dev, uint32_t reference, uint32_t target)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	const struct nxp_fmeas_inputmux_entry *ref_entry = &config->inputmux_entries[0];
	const struct nxp_fmeas_inputmux_entry *tar_entry = &config->inputmux_entries[1];
	k_spinlock_key_t key;
	enum nxp_fmeas_state prev_state;

	/* Target is mandatory; the reference is fixed to the devicetree clock. */
	if (target == 0U) {
		return -EINVAL;
	}
	if (reference != 0U && reference != ref_entry->connection) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FMEAS_STATE_RUNNING ||
	    data->state == NXP_FMEAS_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	prev_state = data->state;
	data->state = NXP_FMEAS_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	k_work_cancel_delayable(&data->poll_work);

	/* Re-route the target channel */
	INPUTMUX_Init(tar_entry->base);
	INPUTMUX_AttachSignal(tar_entry->base, tar_entry->channel,
			      (inputmux_connection_t)target);
	INPUTMUX_Deinit(tar_entry->base);

	key = k_spin_lock(&data->lock);
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	data->state = prev_state;
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_fmeas_init(const struct device *dev)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	int ret;

	data->state = NXP_FMEAS_STATE_IDLE;
	data->dev = dev;
	k_work_init_delayable(&data->poll_work, nxp_fmeas_poll_work);

	if (!device_is_ready(config->gate_clk_dev)) {
		LOG_ERR("%s: gate clock device not ready", dev->name);
		return -ENODEV;
	}

	ret = clock_control_on(config->gate_clk_dev, config->gate_clk_subsys);
	if (ret != 0) {
		LOG_ERR("%s: failed to enable gate clock (%d)", dev->name, ret);
		return ret;
	}

	ret = clock_control_on(config->ref_clk_dev, config->ref_clk_subsys);
	if (ret != 0) {
		LOG_ERR("%s: failed to route reference clock (%d)", dev->name, ret);
		return ret;
	}

	/* Release the FREQME peripheral reset before touching its registers.
	 * clock_control_on() only ungates the peripheral clock; without this
	 * the FREQMECTRL register stays inert (writes are dropped, reads return
	 * 0) and every measurement reports RESULT = 0.
	 */
	if (!device_is_ready(config->reset.dev)) {
		LOG_ERR("%s: reset controller not ready", dev->name);
		return -ENODEV;
	}

	ret = reset_line_toggle_dt(&config->reset);
	if (ret != 0) {
		LOG_ERR("%s: failed to reset FREQME (%d)", dev->name, ret);
		return ret;
	}

	nxp_fmeas_setup_inputmux(config);
	return 0;
}

static DEVICE_API(clock_monitor, nxp_fmeas_api) = {
	.configure  = nxp_fmeas_configure,
	.start      = nxp_fmeas_start,
	.stop       = nxp_fmeas_stop,
	.get_rate   = nxp_fmeas_get_rate,
	.set_source = nxp_fmeas_set_source,
};

#define NXP_FMEAS_INPUTMUX_ENTRY(node_id, prop, idx)                           \
	{                                                                      \
		.base = (INPUTMUX_Type *)DT_REG_ADDR(                          \
			DT_PHANDLE_BY_IDX(node_id, prop, idx)),                \
		.channel = (uint16_t)DT_PHA_BY_IDX(node_id, prop, idx, channel), \
		.connection =                                                  \
			(uint32_t)DT_PHA_BY_IDX(node_id, prop, idx, connection), \
	}

#define NXP_FMEAS_DEVICE_INIT(inst)                                            \
	static const struct nxp_fmeas_inputmux_entry                           \
		nxp_fmeas_inputmux_##inst[] = {                                \
		DT_INST_FOREACH_PROP_ELEM_SEP(inst, inputmux_connections,      \
					      NXP_FMEAS_INPUTMUX_ENTRY, (,))   \
	};                                                                     \
	static struct nxp_fmeas_data nxp_fmeas_data_##inst;                    \
	static const struct nxp_fmeas_config nxp_fmeas_cfg_##inst = {           \
		.base = (FMEAS_SYSCON_Type *)DT_INST_REG_ADDR(inst),           \
		.gate_clk_dev =                                                \
			DEVICE_DT_GET(DT_INST_CLOCKS_CTLR_BY_NAME(inst, ipg)), \
		.gate_clk_subsys = (clock_control_subsys_t)(uintptr_t)         \
			DT_INST_CLOCKS_CELL_BY_NAME(inst, ipg, name),          \
		.ref_clk_dev = DEVICE_DT_GET(                                  \
			DT_INST_CLOCKS_CTLR_BY_NAME(inst, reference)),         \
		.ref_clk_subsys = (clock_control_subsys_t)(uintptr_t)          \
			DT_INST_CLOCKS_CELL_BY_NAME(inst, reference, name),    \
		.reset = RESET_DT_SPEC_INST_GET(inst),                        \
		.inputmux_entries = nxp_fmeas_inputmux_##inst,                 \
		.inputmux_entries_count =                                      \
			DT_INST_PROP_LEN(inst, inputmux_connections),          \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_fmeas_init, NULL,                      \
			      &nxp_fmeas_data_##inst, &nxp_fmeas_cfg_##inst,   \
			      POST_KERNEL, CONFIG_CLOCK_MONITOR_INIT_PRIORITY, \
			      &nxp_fmeas_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_FMEAS_DEVICE_INIT)
