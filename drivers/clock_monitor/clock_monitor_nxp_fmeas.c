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
 * monitored clock produced no edges, or when the reference window counter
 * stops advancing — both conditions are indistinguishable in hardware and
 * are reported as CLOCK_LOST).
 *
 * The fsl_fmeas HAL fixes the reference window at 2^20 reference-clock
 * periods, so clock_monitor_measure_cfg::window_ns is accepted but not
 * honored; the effective window is HW/HAL-fixed.
 *
 * Reference and target clocks are selected from devicetree-declared source
 * lists ("reference-sources" / "target-sources"); each source pairs an
 * INPUTMUX route (applied via the mux subsystem) with a frequency provider
 * (a fixed constant or a clock_control subsystem). clock_monitor_set_source()
 * switches between the declared sources at runtime, dropping the device back
 * to IDLE so the next configure() re-derives the measurement parameters.
 *
 * This driver does NOT enable the selected source clocks; the caller must
 * ensure both are running before configure()/start().
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

#include <zephyr/devicetree/mux.h>
#include <zephyr/drivers/mux.h>

LOG_MODULE_REGISTER(clock_monitor_nxp_fmeas, CONFIG_CLOCK_MONITOR_LOG_LEVEL);

/* Result counter is 31 bits; HAL computes target edges as RESULT - 2 (bias
 * compensates for reference-clock sync alignment at start). RESULT <= 2
 * means zero target edges counted (clock lost).
 */
#define FMEAS_RESULT_BIAS 2U
/* The fsl_fmeas HAL measures over a fixed 2^20 reference-clock window. */
#define FMEAS_REF_WINDOW_LOG2 20U
/* Upper bound on poll iterations before a stuck MEASURE_IN_PROGRESS bit is
 * treated as a lost clock, relative to one estimated window duration.
 */
#define FMEAS_POLL_MAX_TRIES 20U

/*
 * A selectable fmeas clock source: an INPUTMUX route (applied through the
 * mux subsystem) paired with an optional frequency provider. The provider is
 * a fixed constant (has_fixed_hz) or a clock_control subsystem (clk_dev /
 * clk_subsys); a fixed provider takes precedence when both are declared.
 *
 * mux_state->state holds the inputmux_connection_t cookie, which also doubles
 * as the runtime clock_monitor_set_source() selector key.
 */
struct nxp_fmeas_source {
	const struct device    *mux_dev;
	const struct mux_state *mux_state;
	const struct device    *clk_dev;
	clock_control_subsys_t  clk_subsys;
	uint32_t                fixed_hz;
	bool                    has_fixed_hz;
	bool                    is_default;
};

struct nxp_fmeas_config {
	FMEAS_SYSCON_Type *base;
	/* Peripheral gate clock, enabled via clock_control_on(). */
	const struct device    *gate_clk_dev;
	clock_control_subsys_t  gate_clk_subsys;
	struct reset_dt_spec    reset;
	/* Selectable reference / target sources; the is_default source is the
	 * power-on selection.
	 */
	const struct nxp_fmeas_source *ref_srcs;
	uint8_t ref_srcs_count;
	const struct nxp_fmeas_source *tar_srcs;
	uint8_t tar_srcs_count;
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
	/* Index into config->ref_srcs / tar_srcs for the current selection. */
	uint8_t cur_ref_idx;
	uint8_t cur_tar_idx;
	uint32_t ref_hz;      /* cached from the reference source at configure */
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

/* Route a source's INPUTMUX connection through the mux subsystem. */
static int nxp_fmeas_route(const struct nxp_fmeas_source *src)
{
	if (!device_is_ready(src->mux_dev)) {
		LOG_ERR("inputmux device not ready");
		return -ENODEV;
	}
	return mux_state_apply(src->mux_dev, src->mux_state);
}

/*
 * Resolve a source's frequency in Hz. A fixed provider is returned verbatim;
 * otherwise the clock_control rate is queried. Returns -EIO when the provider
 * device is not ready or the rate is unavailable/zero.
 */
static int nxp_fmeas_src_rate(const struct nxp_fmeas_source *src, uint32_t *hz)
{
	uint32_t rate = 0U;
	int ret;

	if (src->has_fixed_hz) {
		*hz = src->fixed_hz;
		return 0;
	}
	if (src->clk_dev == NULL || !device_is_ready(src->clk_dev)) {
		return -EIO;
	}
	ret = clock_control_get_rate(src->clk_dev, src->clk_subsys, &rate);
	if (ret != 0 || rate == 0U) {
		return -EIO;
	}
	*hz = rate;
	return 0;
}

/* Find the source whose INPUTMUX connection cookie equals @p cookie. */
static int nxp_fmeas_find_src(const struct nxp_fmeas_source *srcs, uint8_t count,
			       uint32_t cookie, uint8_t *out_idx)
{
	for (uint8_t i = 0U; i < count; i++) {
		if (srcs[i].mux_state->state == cookie) {
			*out_idx = i;
			return 0;
		}
	}
	return -EINVAL;
}

/* The default (power-on) source index: the child marked default-source. */
static uint8_t nxp_fmeas_default_idx(const struct nxp_fmeas_source *srcs,
				      uint8_t count)
{
	for (uint8_t i = 0U; i < count; i++) {
		if (srcs[i].is_default) {
			return i;
		}
	}
	return 0U;
}

/* Read the raw 31-bit capture value without the HAL's assert(capval >= 2). */
static uint32_t nxp_fmeas_raw_result(const struct nxp_fmeas_config *config)
{
	return config->base->FREQMECTRL_R & FREQME_FREQMECTRL_R_RESULT_MASK;
}

static int nxp_fmeas_configure(const struct device *dev,
			       const struct clock_monitor_config *cfg)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;
	enum nxp_fmeas_state prev_state;
	uint8_t ref_idx;
	uint32_t ref_hz = 0U;
	int ret;

	/* This back-end's hardware supports one-shot measurement only.
	 * cfg->measure.window_ns is ignored: the fsl_fmeas HAL always measures
	 * over a fixed 2^20 reference-clock window.
	 */
	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE) {
		return -ENOTSUP;
	}

	/* Claim the configure transaction; concurrent callers get -EBUSY. */
	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FMEAS_STATE_RUNNING ||
	    data->state == NXP_FMEAS_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	prev_state = data->state;
	ref_idx    = data->cur_ref_idx;
	data->state = NXP_FMEAS_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	/* Query the reference rate outside the lock; needed by poll_work to
	 * convert the raw result counter into Hz via FMEAS_GetFrequency().
	 */
	ret = nxp_fmeas_src_rate(&config->ref_srcs[ref_idx], &ref_hz);
	if (ret != 0) {
		goto restore;
	}

	key = k_spin_lock(&data->lock);
	data->cfg = *cfg;          /* save callback / user_data for poll_work */
	data->ref_hz = ref_hz;     /* cache for FMEAS_GetFrequency() in poll_work */
	/* Workqueue delay: one measurement window = 2^20 ref-clock periods. */
	data->poll_us = (uint32_t)(((uint64_t)BIT(FMEAS_REF_WINDOW_LOG2) *
				    (uint64_t)USEC_PER_SEC) / ref_hz);
	data->last_rate_hz = 0U;   /* invalidate stale result from prior cycle */
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
	uint32_t poll_us;

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
	poll_us = data->poll_us;
	k_spin_unlock(&data->lock, key);

	/* Start the measurement outside the spinlock: fmeas uses workqueue
	 * polling (not an ISR), so poll_work cannot fire before the scheduled
	 * delay expires — no state-machine race to guard against.
	 */
	FMEAS_StartMeasure(config->base);
	/* First check after roughly one full window; then poll faster. Clamp to
	 * a minimum of 1 us so a very fast reference clock (large ref_hz -> tiny
	 * poll_us) can never schedule a zero-delay work that busy-spins.
	 */
	k_work_reschedule(&data->poll_work, K_USEC(MAX(poll_us, 1U)));
	return 0;
}

static int nxp_fmeas_stop(const struct device *dev)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FMEAS_STATE_RUNNING) {
		/* MEASURE_IN_PROGRESS=0 force-terminates any cycle in progress
		 * and resets RESULT (RM: Force Terminate).
		 */
		config->base->FREQMECTRL_W = 0U;
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
		 * advancing the window counter. This is reported as CLOCK_LOST
		 * since both a stuck reference and a stuck target are
		 * indistinguishable at the fmeas register level.
		 */
		data->clock_lost = true;
		evts = CLOCK_MONITOR_EVT_CLOCK_LOST;
		LOG_WRN("%s: measurement timed out (FREQMECTRL_R=0x%08x, ref_hz=%u) - "
			"reference clock not running?", dev->name,
			config->base->FREQMECTRL_R, data->ref_hz);
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
			/* A fresh good measurement clears any prior CLOCK_LOST
			 * so get_rate() reflects the most recent completed cycle.
			 */
			data->clock_lost = false;
			evts = CLOCK_MONITOR_EVT_MEASURE_DONE;
			LOG_DBG("%s: measured %u Hz (result=%u, ref_hz=%u)",
				dev->name, rate, result, data->ref_hz);
		}
	}

	/* Auto-disarm before the callback so it may restart the cycle. */
	data->state = NXP_FMEAS_STATE_CONFIGURED;
	if (evts != 0U) {
		cb = data->cfg.callback;
		user_data = data->cfg.user_data;
	}
	k_spin_unlock(&data->lock, key);

	if (cb != NULL && evts != 0U) {
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

static int nxp_fmeas_set_source(const struct device *dev, uint32_t reference,
				 uint32_t target)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;
	uint8_t cur_ref_idx;
	uint8_t cur_tar_idx;
	uint8_t new_ref_idx = 0U;
	uint8_t new_tar_idx;
	int ret;

	/* Resolve requested reference source (0 = keep current, resolved after
	 * the lock is taken).
	 */
	if (reference != 0U) {
		ret = nxp_fmeas_find_src(config->ref_srcs, config->ref_srcs_count,
					  reference, &new_ref_idx);
		if (ret != 0) {
			return -EINVAL;
		}
	}

	/* Target is mandatory. */
	if (target == 0U) {
		return -EINVAL;
	}
	ret = nxp_fmeas_find_src(config->tar_srcs, config->tar_srcs_count,
				  target, &new_tar_idx);
	if (ret != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FMEAS_STATE_RUNNING ||
	    data->state == NXP_FMEAS_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	/* Snapshot the current selection under the lock; the route decision and
	 * later commit compare against this snapshot rather than re-reading
	 * data->cur_*_idx (which is lock-protected) outside the lock.
	 */
	cur_ref_idx = data->cur_ref_idx;
	cur_tar_idx = data->cur_tar_idx;

	/* Fill in "keep current" after the busy check. */
	if (reference == 0U) {
		new_ref_idx = cur_ref_idx;
	}

	/* No-op: neither source has changed. */
	if (new_ref_idx == cur_ref_idx && new_tar_idx == cur_tar_idx) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	data->state = NXP_FMEAS_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	k_work_cancel_delayable(&data->poll_work);

	/* Re-route changed channels through the mux subsystem. */
	if (new_ref_idx != cur_ref_idx) {
		ret = nxp_fmeas_route(&config->ref_srcs[new_ref_idx]);
		if (ret != 0) {
			LOG_ERR("%s: failed to re-route reference source (%d)",
				dev->name, ret);
			goto route_err;
		}
	}
	if (new_tar_idx != cur_tar_idx) {
		ret = nxp_fmeas_route(&config->tar_srcs[new_tar_idx]);
		if (ret != 0) {
			LOG_ERR("%s: failed to re-route target source (%d)",
				dev->name, ret);
			goto route_err;
		}
	}

	key = k_spin_lock(&data->lock);
	data->cur_ref_idx = new_ref_idx;
	data->cur_tar_idx = new_tar_idx;
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	/* Source changed; drop to IDLE so the next configure() re-derives
	 * measurement parameters (ref_hz, poll_us) against the new selection.
	 */
	data->state = NXP_FMEAS_STATE_IDLE;
	k_spin_unlock(&data->lock, key);
	return 0;

route_err:
	/* Drop to IDLE so a subsequent set_source() is not rejected with
	 * -EBUSY. The previous selection is left untouched.
	 */
	key = k_spin_lock(&data->lock);
	data->state = NXP_FMEAS_STATE_IDLE;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int nxp_fmeas_init(const struct device *dev)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	int ret;

	data->state = NXP_FMEAS_STATE_IDLE;
	data->cur_ref_idx = nxp_fmeas_default_idx(config->ref_srcs,
						   config->ref_srcs_count);
	data->cur_tar_idx = nxp_fmeas_default_idx(config->tar_srcs,
						   config->tar_srcs_count);
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

	/* Route the default reference and target sources to the FREQME inputs.
	 * The driver does not enable these source clocks; the caller must ensure
	 * both are running before configure()/start().
	 */
	ret = nxp_fmeas_route(&config->ref_srcs[data->cur_ref_idx]);
	if (ret != 0) {
		LOG_ERR("%s: failed to route default reference source (%d)",
			dev->name, ret);
		return ret;
	}
	ret = nxp_fmeas_route(&config->tar_srcs[data->cur_tar_idx]);
	if (ret != 0) {
		LOG_ERR("%s: failed to route default target source (%d)",
			dev->name, ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(clock_monitor, nxp_fmeas_api) = {
	.configure  = nxp_fmeas_configure,
	.start      = nxp_fmeas_start,
	.stop       = nxp_fmeas_stop,
	.get_rate   = nxp_fmeas_get_rate,
	.set_source = nxp_fmeas_set_source,
};

/* Frequency provider macros — mirrors nxp_freqme equivalents. */
#define NXP_FMEAS_SRC_CLK_DEV(node)                                            \
	COND_CODE_1(DT_NODE_HAS_PROP(node, clock_frequency), (NULL),           \
		    (COND_CODE_1(DT_NODE_HAS_PROP(node, clocks),               \
				 (DEVICE_DT_GET(DT_CLOCKS_CTLR(node))),        \
				 (NULL))))

#define NXP_FMEAS_SRC_CLK_SUBSYS(node)                                         \
	COND_CODE_1(DT_NODE_HAS_PROP(node, clock_frequency),                   \
		    ((clock_control_subsys_t)0),                               \
		    (COND_CODE_1(DT_NODE_HAS_PROP(node, clocks),               \
				 ((clock_control_subsys_t)(uintptr_t)          \
					  DT_CLOCKS_CELL(node, name)),         \
				 ((clock_control_subsys_t)0))))

/* Emit static mux_state storage for one source child node. */
#define NXP_FMEAS_SRC_MUX_DEFINE(node) MUX_STATE_DT_SPEC_DEFINE(node);

#define NXP_FMEAS_SRC_ENTRY(node)                                              \
	{                                                                      \
		.mux_dev   = MUX_STATE_DT_DEV_GET(node),                       \
		.mux_state = MUX_STATE_DT_GET(node),                           \
		.clk_dev   = NXP_FMEAS_SRC_CLK_DEV(node),                     \
		.clk_subsys = NXP_FMEAS_SRC_CLK_SUBSYS(node),                 \
		.fixed_hz  = (uint32_t)DT_PROP_OR(node, clock_frequency, 0),  \
		.has_fixed_hz = DT_NODE_HAS_PROP(node, clock_frequency),       \
		.is_default = DT_PROP(node, default_source),                   \
	}

/* Per-reference-source compile-time check: must have a frequency provider. */
#define NXP_FMEAS_REF_SRC_ASSERT(node)                                         \
	BUILD_ASSERT(DT_NODE_HAS_PROP(node, clock_frequency) ||                \
			     DT_NODE_HAS_PROP(node, clocks),                   \
		     "nxp,fmeas reference source needs clocks or clock-frequency");

/* Count of default-source children in a container. */
#define NXP_FMEAS_DEFAULT_INC(node) + DT_PROP(node, default_source)

#define NXP_FMEAS_REF_NODE(inst) DT_INST_CHILD(inst, reference_sources)
#define NXP_FMEAS_TAR_NODE(inst) DT_INST_CHILD(inst, target_sources)

#define NXP_FMEAS_DEVICE_INIT(inst)                                            \
	BUILD_ASSERT(DT_NODE_EXISTS(NXP_FMEAS_REF_NODE(inst)),                 \
		     "nxp,fmeas: missing reference-sources node");             \
	BUILD_ASSERT(DT_NODE_EXISTS(NXP_FMEAS_TAR_NODE(inst)),                 \
		     "nxp,fmeas: missing target-sources node");                \
	BUILD_ASSERT(DT_CHILD_NUM(NXP_FMEAS_REF_NODE(inst)) >= 1,             \
		     "nxp,fmeas: reference-sources needs >= 1 source");        \
	BUILD_ASSERT(DT_CHILD_NUM(NXP_FMEAS_TAR_NODE(inst)) >= 1,             \
		     "nxp,fmeas: target-sources needs >= 1 source");           \
	BUILD_ASSERT((0 DT_FOREACH_CHILD(NXP_FMEAS_REF_NODE(inst),            \
					 NXP_FMEAS_DEFAULT_INC)) == 1,         \
		     "nxp,fmeas: reference-sources needs exactly one default-source"); \
	BUILD_ASSERT((0 DT_FOREACH_CHILD(NXP_FMEAS_TAR_NODE(inst),            \
					 NXP_FMEAS_DEFAULT_INC)) == 1,         \
		     "nxp,fmeas: target-sources needs exactly one default-source"); \
	DT_FOREACH_CHILD(NXP_FMEAS_REF_NODE(inst), NXP_FMEAS_REF_SRC_ASSERT) \
	DT_FOREACH_CHILD(NXP_FMEAS_REF_NODE(inst), NXP_FMEAS_SRC_MUX_DEFINE) \
	DT_FOREACH_CHILD(NXP_FMEAS_TAR_NODE(inst), NXP_FMEAS_SRC_MUX_DEFINE) \
	static const struct nxp_fmeas_source nxp_fmeas_ref_srcs_##inst[] = {  \
		DT_FOREACH_CHILD_SEP(NXP_FMEAS_REF_NODE(inst),                \
				     NXP_FMEAS_SRC_ENTRY, (,))                 \
	};                                                                     \
	static const struct nxp_fmeas_source nxp_fmeas_tar_srcs_##inst[] = {  \
		DT_FOREACH_CHILD_SEP(NXP_FMEAS_TAR_NODE(inst),                \
				     NXP_FMEAS_SRC_ENTRY, (,))                 \
	};                                                                     \
	static struct nxp_fmeas_data nxp_fmeas_data_##inst;                    \
	static const struct nxp_fmeas_config nxp_fmeas_cfg_##inst = {          \
		.base = (FMEAS_SYSCON_Type *)DT_INST_REG_ADDR(inst),           \
		.gate_clk_dev =                                                \
			DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),             \
		.gate_clk_subsys = (clock_control_subsys_t)(uintptr_t)         \
			DT_INST_CLOCKS_CELL(inst, name),                       \
		.reset = RESET_DT_SPEC_INST_GET(inst),                         \
		.ref_srcs = nxp_fmeas_ref_srcs_##inst,                         \
		.ref_srcs_count =                                              \
			(uint8_t)ARRAY_SIZE(nxp_fmeas_ref_srcs_##inst),        \
		.tar_srcs = nxp_fmeas_tar_srcs_##inst,                         \
		.tar_srcs_count =                                              \
			(uint8_t)ARRAY_SIZE(nxp_fmeas_tar_srcs_##inst),        \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_fmeas_init, NULL,                      \
			      &nxp_fmeas_data_##inst, &nxp_fmeas_cfg_##inst,   \
			      POST_KERNEL, CONFIG_CLOCK_MONITOR_INIT_PRIORITY, \
			      &nxp_fmeas_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_FMEAS_DEVICE_INIT)
