/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for the NXP BASIC Frequency Measurement
 * (FREQME) block, built on the MCUX SDK fsl_fmeas HAL.
 *
 * The block is a single FREQMECTRL register with no interrupt line, so only
 * MEASURE (one-shot) mode is supported: completion is polled from a workqueue
 * and reported as MEASURE_DONE, or as CLOCK_LOST when the target counted no
 * edges or the reference window stopped advancing (indistinguishable here). The
 * HAL fixes the window at 2^20 reference periods, so window_ns is ignored and a
 * measurement takes 2^20 / ref_hz - 32 ms off 32 MHz, but 32 s off 32.768 kHz.
 *
 * Reference and target clocks come from the devicetree source lists
 * ("reference-sources" / "target-sources"); clock_monitor_set_source() switches
 * between them at runtime and drops the device to IDLE so the next configure()
 * re-derives the measurement parameters. The driver does NOT enable the selected
 * clocks; the caller must have both running before configure()/start().
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

/* The HAL computes target edges as RESULT - 2 (reference sync alignment). */
#define FMEAS_RESULT_BIAS 2U
/* Fixed 2^20 reference window; a power of two, so the rate is a shift. */
#define FMEAS_REF_WINDOW_LOG2 20U
/* Retries after the first check: total timeout = 1 + 20/4 windows, i.e. six. */
#define FMEAS_POLL_MAX_TRIES 20U

/* A selectable source: a mux route plus a frequency provider (fixed_hz wins over
 * clock_control). mux_state->state is the INPUTMUX cookie, and also the
 * set_source() selector key.
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
	/* Selectable sources; the is_default one is the power-on selection. */
	const struct nxp_fmeas_source *ref_srcs;
	uint8_t ref_srcs_count;
	const struct nxp_fmeas_source *tar_srcs;
	uint8_t tar_srcs_count;
};

enum nxp_fmeas_state {
	NXP_FMEAS_STATE_IDLE = 0,
	/* set_source() holds this across mux_state_apply(), which the lock cannot. */
	NXP_FMEAS_STATE_CONFIGURING,
	NXP_FMEAS_STATE_CONFIGURED,
	/* Armed, with the poll work scheduled. */
	NXP_FMEAS_STATE_RUNNING,
};

/* rate sentinels; any other value is a measured frequency in Hz. */
#define NXP_FMEAS_RATE_NONE (-1) /* no cycle completed since configure() */
#define NXP_FMEAS_RATE_LOST (-2) /* monitored clock found stuck */

struct nxp_fmeas_data {
	/* Guards every field below; never held across another subsystem's call. */
	struct k_spinlock lock;
	enum nxp_fmeas_state state;
	clock_monitor_callback_t cb;
	void *user_data;
	/* Current selection, indexes into config->ref_srcs / tar_srcs. */
	uint8_t cur_ref_idx;
	uint8_t cur_tar_idx;
	uint8_t poll_tries;  /* remaining poll iterations for this cycle */
	uint32_t ref_hz;     /* cached from the reference source at configure */
	/* Derived from ref_hz at configure(), so the lock holds no conversion. */
	k_timeout_t poll_first;
	k_timeout_t poll_retry;
	int32_t rate;        /* >= 0 is Hz, negative is a sentinel above */
	struct k_work_delayable poll_work;
	const struct device *dev;
};

/* configure()/set_source() answer -EBUSY in these states. */
static inline bool nxp_fmeas_busy(enum nxp_fmeas_state state)
{
	return (state == NXP_FMEAS_STATE_RUNNING) ||
	       (state == NXP_FMEAS_STATE_CONFIGURING);
}

/* Route a source's INPUTMUX connection; mux devices are validated at init. */
static int nxp_fmeas_route(const struct nxp_fmeas_source *src)
{
	return mux_state_apply(src->mux_dev, src->mux_state);
}

/* Resolve a source's frequency: fixed provider verbatim, else clock_control. */
static int nxp_fmeas_src_rate(const struct nxp_fmeas_source *src, uint32_t *hz)
{
	uint32_t rate = 0U;

	if (src->has_fixed_hz) {
		rate = src->fixed_hz;
	} else {
		if (src->clk_dev == NULL || !device_is_ready(src->clk_dev)) {
			return -EIO;
		}
		if (clock_control_get_rate(src->clk_dev, src->clk_subsys, &rate) != 0) {
			return -EIO;
		}
	}

	if (rate == 0U) {
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

/* Every source's mux device must be usable before the first route. */
static bool nxp_fmeas_srcs_ready(const struct nxp_fmeas_source *srcs, uint8_t count)
{
	for (uint8_t i = 0U; i < count; i++) {
		if (!device_is_ready(srcs[i].mux_dev)) {
			return false;
		}
	}
	return true;
}

static int nxp_fmeas_configure(const struct device *dev,
			       const struct clock_monitor_config *cfg)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;
	uint8_t ref_idx;
	uint32_t ref_hz = 0U;
	uint32_t poll_us;
	int ret;

	/* MEASURE only; window_ns is ignored (the HAL window is fixed). */
	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);
	ref_idx = data->cur_ref_idx;
	k_spin_unlock(&data->lock, key);

	/* Resolved with no lock held: this may call into clock_control. */
	ret = nxp_fmeas_src_rate(&config->ref_srcs[ref_idx], &ref_hz);
	if (ret != 0) {
		return ret;
	}

	/* One window, then a quarter per retry; clamped against a zero delay. */
	poll_us = (uint32_t)(((uint64_t)BIT(FMEAS_REF_WINDOW_LOG2) *
			      (uint64_t)USEC_PER_SEC) / ref_hz);

	key = k_spin_lock(&data->lock);

	/* Busy, or the selection moved while the rate was resolved - just retry. */
	if (nxp_fmeas_busy(data->state) || data->cur_ref_idx != ref_idx) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->cb = cfg->callback;
	data->user_data = cfg->user_data;
	data->ref_hz = ref_hz;
	data->poll_first = K_USEC(MAX(poll_us, 1U));
	data->poll_retry = K_USEC(MAX(poll_us / 4U, 1U));
	data->rate = NXP_FMEAS_RATE_NONE;
	data->state = NXP_FMEAS_STATE_CONFIGURED;

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_fmeas_start(const struct device *dev)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->state != NXP_FMEAS_STATE_CONFIGURED) {
		enum nxp_fmeas_state st = data->state;

		k_spin_unlock(&data->lock, key);
		return (st == NXP_FMEAS_STATE_IDLE) ? -EINVAL : -EBUSY;
	}

	data->state = NXP_FMEAS_STATE_RUNNING;
	data->poll_tries = FMEAS_POLL_MAX_TRIES;

	/* Arm and queue inside the lock, so a concurrent stop() lands entirely
	 * before or entirely after the cycle. k_work_reschedule() does not sleep.
	 */
	FMEAS_StartMeasure(config->base);
	k_work_reschedule(&data->poll_work, data->poll_first);

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_fmeas_stop(const struct device *dev)
{
	const struct nxp_fmeas_config *config = dev->config;
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->state == NXP_FMEAS_STATE_RUNNING) {
		/* Force Terminate (RM): MEASURE_IN_PROGRESS=0 also resets RESULT.
		 * Cancel here so it cannot kill a newer cycle's poll; _sync would
		 * deadlock on the handler waiting for this lock.
		 */
		config->base->FREQMECTRL_W = 0U;
		k_work_cancel_delayable(&data->poll_work);
		data->state = NXP_FMEAS_STATE_CONFIGURED;
	}

	/* Other states have nothing armed, CONFIGURING included. */
	k_spin_unlock(&data->lock, key);
	return 0;
}

static void nxp_fmeas_poll_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct nxp_fmeas_data *data = CONTAINER_OF(dwork, struct nxp_fmeas_data, poll_work);
	const struct device *dev = data->dev;
	const struct nxp_fmeas_config *config = dev->config;
	clock_monitor_callback_t cb;
	void *user_data;
	k_spinlock_key_t key;
	bool timed_out = false;
	uint32_t ref_hz;
	uint32_t raw;
	int32_t rate;

	key = k_spin_lock(&data->lock);

	/* stop() may have torn the cycle down while this run waited for the lock. */
	if (data->state != NXP_FMEAS_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	ref_hz = data->ref_hz;

	if (!FMEAS_IsMeasureComplete(config->base)) {
		if (data->poll_tries != 0U) {
			data->poll_tries--;
			k_work_reschedule(&data->poll_work, data->poll_retry);
			k_spin_unlock(&data->lock, key);
			return;
		}
		/* The reference window is not advancing. A stuck reference and a
		 * stuck target are indistinguishable, so both report CLOCK_LOST.
		 */
		timed_out = true;
		rate = NXP_FMEAS_RATE_LOST;
		raw = config->base->FREQMECTRL_R; /* whole register, for the log */
		/* Still armed - that is why it timed out. Terminate it so no stable
		 * state ever has the block running.
		 */
		config->base->FREQMECTRL_W = 0U;
	} else {
		raw = nxp_fmeas_raw_result(config);

		/* edges = raw - bias, so raw <= bias means stuck or unrouted. */
		if (raw <= FMEAS_RESULT_BIAS) {
			rate = NXP_FMEAS_RATE_LOST;
		} else {
			/* FMEAS_GetFrequency() without its second read and division. */
			rate = (int32_t)(((uint64_t)(raw - FMEAS_RESULT_BIAS) *
					  ref_hz) >> FMEAS_REF_WINDOW_LOG2);
		}
	}

	data->rate = rate;
	/* Auto-disarm before the callback so it may restart the cycle. */
	data->state = NXP_FMEAS_STATE_CONFIGURED;
	cb = data->cb;
	user_data = data->user_data;

	k_spin_unlock(&data->lock, key);

	/* Outside the lock: immediate-mode logging must not run with irqs off. */
	if (timed_out) {
		LOG_WRN("%s: measurement timed out (FREQMECTRL_R=0x%08x, ref_hz=%u) - "
			"reference clock not running?", dev->name, raw, ref_hz);
	} else if (rate < 0) {
		LOG_WRN("%s: no target edges counted (result=%u, ref_hz=%u) - "
			"target clock not running/routed?", dev->name, raw, ref_hz);
	} else {
		LOG_DBG("%s: measured %u Hz (result=%u, ref_hz=%u)",
			dev->name, (uint32_t)rate, raw, ref_hz);
	}

	if (cb != NULL) {
		bool measured = (rate >= 0);
		struct clock_monitor_event_data evt = {
			.events = measured ? CLOCK_MONITOR_EVT_MEASURE_DONE
					   : CLOCK_MONITOR_EVT_CLOCK_LOST,
			.measured_hz = measured ? (uint32_t)rate : 0U,
		};

		cb(dev, &evt, user_data);
	}
}

static int nxp_fmeas_get_rate(const struct device *dev, uint32_t *rate_hz)
{
	struct nxp_fmeas_data *data = dev->data;
	k_spinlock_key_t key;
	int32_t rate;

	key = k_spin_lock(&data->lock);
	rate = data->rate;
	k_spin_unlock(&data->lock, key);

	if (rate >= 0) {
		*rate_hz = (uint32_t)rate;
		return 0;
	}

	/* LOST: stuck since configure(). NONE: in flight, or none completed. */
	return (rate == NXP_FMEAS_RATE_LOST) ? -EIO : -EAGAIN;
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

	/* Input-only validation; reference 0 means "keep current". */
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

	if (nxp_fmeas_busy(data->state)) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	cur_ref_idx = data->cur_ref_idx;
	cur_tar_idx = data->cur_tar_idx;
	if (reference == 0U) {
		new_ref_idx = cur_ref_idx;
	}

	/* No-op: nothing to re-route, so the state is left as it was. */
	if (new_ref_idx == cur_ref_idx && new_tar_idx == cur_tar_idx) {
		k_spin_unlock(&data->lock, key);
		return 0;
	}

	/* CONFIGURING holds the device across the re-route: mux_state_apply() may
	 * sleep, so the lock cannot cover it.
	 */
	data->state = NXP_FMEAS_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	if (new_ref_idx != cur_ref_idx) {
		ret = nxp_fmeas_route(&config->ref_srcs[new_ref_idx]);
		if (ret != 0) {
			LOG_ERR("%s: failed to re-route reference source (%d)",
				dev->name, ret);
			goto route_err;
		}
		key = k_spin_lock(&data->lock);
		data->cur_ref_idx = new_ref_idx;
		k_spin_unlock(&data->lock, key);
	}
	if (new_tar_idx != cur_tar_idx) {
		ret = nxp_fmeas_route(&config->tar_srcs[new_tar_idx]);
		if (ret != 0) {
			LOG_ERR("%s: failed to re-route target source (%d)",
				dev->name, ret);
			goto route_err;
		}
		key = k_spin_lock(&data->lock);
		data->cur_tar_idx = new_tar_idx;
		k_spin_unlock(&data->lock, key);
	}

	key = k_spin_lock(&data->lock);
	data->rate = NXP_FMEAS_RATE_NONE;
	/* IDLE, so the next configure() re-derives the rate and the poll delays. */
	data->state = NXP_FMEAS_STATE_IDLE;
	k_spin_unlock(&data->lock, key);
	return 0;

route_err:
	/* IDLE drops the now-stale rate and forces a re-configure. */
	key = k_spin_lock(&data->lock);
	data->rate = NXP_FMEAS_RATE_NONE;
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
	data->rate = NXP_FMEAS_RATE_NONE;
	data->cur_ref_idx = nxp_fmeas_default_idx(config->ref_srcs,
						   config->ref_srcs_count);
	data->cur_tar_idx = nxp_fmeas_default_idx(config->tar_srcs,
						   config->tar_srcs_count);
	data->dev = dev;
	k_work_init_delayable(&data->poll_work, nxp_fmeas_poll_work);

	/* Validated once here, so set_source() cannot fail on an unready mux. */
	if (!nxp_fmeas_srcs_ready(config->ref_srcs, config->ref_srcs_count) ||
	    !nxp_fmeas_srcs_ready(config->tar_srcs, config->tar_srcs_count)) {
		LOG_ERR("%s: inputmux device not ready", dev->name);
		return -ENODEV;
	}

	if (!device_is_ready(config->gate_clk_dev)) {
		LOG_ERR("%s: gate clock device not ready", dev->name);
		return -ENODEV;
	}

	ret = clock_control_on(config->gate_clk_dev, config->gate_clk_subsys);
	if (ret != 0) {
		LOG_ERR("%s: failed to enable gate clock (%d)", dev->name, ret);
		return ret;
	}

	/* Ungating the clock is not enough: until the reset is released FREQMECTRL
	 * is inert (writes dropped, reads 0) and every measurement reports 0.
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
