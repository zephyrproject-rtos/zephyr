/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for the frequency-measurement feature of the
 * NXP ANACTRL (Analog Control) module.
 *
 * Reference and target clocks are selected from devicetree-declared source
 * lists ("reference-sources" / "target-sources"); each source pairs an
 * INPUTMUX route with a frequency provider (a fixed constant or a
 * clock_control subsystem). clock_monitor_set_source() switches between the
 * declared sources at runtime, dropping the device back to the unconfigured
 * state so the next configure() re-derives the measurement parameters.
 *
 * This driver does NOT enable the selected reference/target source clocks:
 * the caller must ensure both clocks are running before configure()/start().
 */

#define DT_DRV_COMPAT nxp_anactrl_freqme

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/dt-bindings/clock-monitor/nxp-anactrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_anactrl.h>

#include <zephyr/devicetree/mux.h>
#include <zephyr/drivers/mux.h>

LOG_MODULE_REGISTER(clock_monitor_nxp_anactrl, CONFIG_CLOCK_MONITOR_LOG_LEVEL);

/* SCALE is a 5-bit field programming the reference counter to (2^SCALE - 1)
 * reference-clock edges; the HAL requires 2..31.
 */
#define ANACTRL_SCALE_MIN 2U
#define ANACTRL_SCALE_MAX 31U

/* Number of extra deferred polls (each ~1/4 of the nominal window) allowed
 * before a still-set PROG bit is declared a lost/stuck target clock. Gives a
 * total wait of roughly 1x + retries*0.25x the measurement window.
 */
#define ANACTRL_POLL_RETRIES 16U

/*
 * Sentinel for set_source()'s resolved source indices: leave that side at its
 * current source. A valid index is always below the container's source count,
 * which is a uint8_t, so UINT8_MAX can never be one.
 */
#define ANACTRL_SRC_KEEP UINT8_MAX

/*
 * A selectable ANACTRL freq-measure clock source: an INPUTMUX route paired
 * with a frequency provider. The provider is a fixed constant (has_fixed_hz)
 * or a clock_control subsystem (clk_dev / clk_subsys); a fixed provider takes
 * precedence when both are declared. Exactly one source per container is
 * flagged is_default.
 */
struct nxp_anactrl_source {
	const struct device    *mux_dev;
	const struct mux_state *mux_state;
	const struct device    *clk_dev;
	clock_control_subsys_t  clk_subsys;
	uint32_t                fixed_hz;
	bool                    has_fixed_hz;
	bool                    is_default;
};

struct nxp_anactrl_config {
	ANACTRL_Type *base;
	/* Peripheral gate clock, enabled via clock_control_on(). */
	const struct device    *gate_clk_dev;
	clock_control_subsys_t  gate_clk_subsys;
	/* Selectable reference / target sources; the is_default source is the
	 * power-on selection.
	 */
	const struct nxp_anactrl_source *ref_srcs;
	uint8_t ref_srcs_count;
	const struct nxp_anactrl_source *tar_srcs;
	uint8_t tar_srcs_count;
};

enum nxp_anactrl_state {
	NXP_ANACTRL_STATE_IDLE = 0,
	/* set_source() holds this across mux_state_apply(), which the lock
	 * cannot; concurrent configure()/start()/set_source() get -EBUSY.
	 */
	NXP_ANACTRL_STATE_CONFIGURING,
	NXP_ANACTRL_STATE_CONFIGURED,
	NXP_ANACTRL_STATE_RUNNING,
};

struct nxp_anactrl_data {
	/* Protects the state machine, cfg, source selection and result fields
	 * against the poll work handler and against concurrent API calls.
	 */
	struct k_spinlock lock;
	enum nxp_anactrl_state state;
	struct clock_monitor_config cfg;
	/* Index into config->ref_srcs / tar_srcs for the currently selected
	 * reference / target source. Only ever written while CONFIGURING.
	 */
	uint8_t cur_ref_idx;
	uint8_t cur_tar_idx;
	uint32_t ref_hz;      /* cached from the reference source at configure */
	uint8_t  scale;       /* cached SCALE for the poll formula / re-arm */
	uint32_t meas_us;     /* nominal measurement window in microseconds */
	uint32_t retries_left;
	/* Most recent completed MEASURE result; retained across reads. */
	uint32_t last_rate_hz;
	bool has_rate;
	/* CLOCK_LOST latched since configure(). */
	bool clock_lost;
	struct k_work_delayable poll_work;
	const struct device *dev;
};

/* configure()/start()/set_source() answer -EBUSY in these states. */
static inline bool nxp_anactrl_busy(enum nxp_anactrl_state state)
{
	return (state == NXP_ANACTRL_STATE_RUNNING) ||
	       (state == NXP_ANACTRL_STATE_CONFIGURING);
}

/*
 * Convert a measurement window (ns) into the SCALE exponent. The ANACTRL
 * window is (2^SCALE - 1) reference-clock periods, i.e. approximately a
 * rounded base-2 logarithm of the requested cycle count, clamped to
 * [ANACTRL_SCALE_MIN, ANACTRL_SCALE_MAX].
 */
static int anactrl_window_to_scale(uint32_t window_ns, uint32_t ref_hz, uint8_t *out)
{
	uint64_t cycles = (uint64_t)window_ns * (uint64_t)ref_hz / (uint64_t)NSEC_PER_SEC;

	if (cycles == 0U) {
		return -ERANGE;
	}

	uint32_t s = 0U;
	uint64_t v = cycles;

	while ((v >> 1) != 0U) {
		v >>= 1;
		s++;
	}

	/* Round to the nearer power of two. */
	if (s < ANACTRL_SCALE_MAX &&
	    (cycles - (1ULL << s)) > ((1ULL << (s + 1U)) - cycles)) {
		s++;
	}
	if (s > ANACTRL_SCALE_MAX) {
		/* A window this long cannot be represented; reject rather than
		 * silently shorten it.
		 */
		return -ERANGE;
	}
	if (s < ANACTRL_SCALE_MIN) {
		s = ANACTRL_SCALE_MIN;
	}

	*out = (uint8_t)s;
	return 0;
}

/* Nominal measurement duration in microseconds for a given SCALE / ref rate,
 * floored at 1 us so the poll always makes forward progress.
 */
static uint32_t anactrl_meas_us(uint8_t scale, uint32_t ref_hz)
{
	uint64_t ns = ((uint64_t)((1ULL << scale) - 1ULL) * (uint64_t)NSEC_PER_SEC) / ref_hz;
	uint32_t us = (uint32_t)(ns / NSEC_PER_USEC);

	return (us == 0U) ? 1U : us;
}

/* Route a source's INPUTMUX connection to its ANACTRL freq-measure
 * destination via the mux subsystem. Returns 0 on success, negative errno on failure.
 */
static int nxp_anactrl_route(const struct nxp_anactrl_source *src)
{
	if (!device_is_ready(src->mux_dev)) {
		LOG_ERR("inputmux device not ready");
		return -ENODEV;
	}
	return mux_state_apply(src->mux_dev, src->mux_state);
}

/*
 * Resolve a source's frequency in Hz. A fixed provider is returned verbatim;
 * otherwise the clock_control rate is queried. Returns -EIO when the rate is
 * unavailable or zero.
 */
static int nxp_anactrl_src_rate(const struct nxp_anactrl_source *src, uint32_t *hz)
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
static int nxp_anactrl_find_src(const struct nxp_anactrl_source *srcs, uint8_t count,
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

/* The default (power-on) source index: the child marked default-source. A
 * BUILD_ASSERT guarantees exactly one exists; 0 is a defensive fallback.
 */
static uint8_t nxp_anactrl_default_idx(const struct nxp_anactrl_source *srcs, uint8_t count)
{
	for (uint8_t i = 0U; i < count; i++) {
		if (srcs[i].is_default) {
			return i;
		}
	}
	return 0U;
}

static int nxp_anactrl_configure(const struct device *dev,
				 const struct clock_monitor_config *cfg)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;
	uint8_t ref_idx;
	uint32_t ref_hz = 0U;
	uint8_t scale;
	int ret;

	/* ANACTRL freq-measure hardware support one-shot measurement only. */
	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE) {
		return -ENOTSUP;
	}
	if (cfg->measure.window_ns == 0U) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	ref_idx = data->cur_ref_idx;
	k_spin_unlock(&data->lock, key);

	ret = nxp_anactrl_src_rate(&config->ref_srcs[ref_idx], &ref_hz);
	if (ret != 0) {
		return ret;
	}

	ret = anactrl_window_to_scale(cfg->measure.window_ns, ref_hz, &scale);
	if (ret != 0) {
		LOG_ERR("window_ns=%u out of range for ref_hz=%u",
			cfg->measure.window_ns, ref_hz);
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	/* Busy, or the selection moved while the rate was resolved - just retry.
	 * Nothing was written to hardware above, so there is nothing to undo.
	 */
	if (nxp_anactrl_busy(data->state) || data->cur_ref_idx != ref_idx) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->cfg = *cfg;
	data->ref_hz = ref_hz;
	data->scale = scale;
	data->meas_us = anactrl_meas_us(scale, ref_hz);
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	data->state = NXP_ANACTRL_STATE_CONFIGURED;

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_anactrl_start(const struct device *dev)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state != NXP_ANACTRL_STATE_CONFIGURED) {
		enum nxp_anactrl_state st = data->state;

		k_spin_unlock(&data->lock, key);
		/* IDLE is "not configured"; RUNNING / CONFIGURING are busy. */
		return (st == NXP_ANACTRL_STATE_IDLE) ? -EINVAL : -EBUSY;
	}

	data->state = NXP_ANACTRL_STATE_RUNNING;
	data->retries_left = ANACTRL_POLL_RETRIES;

	/* Arm the one-shot measurement (PROG + SCALE) and schedule the
	 * completion poll one nominal window later.
	 */
	config->base->FREQ_ME_CTRL =
		ANACTRL_FREQ_ME_CTRL_PROG_MASK | ANACTRL_FREQ_ME_CTRL_CAPVAL_SCALE(data->scale);
	(void)k_work_reschedule(&data->poll_work, K_USEC(data->meas_us));

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_anactrl_stop(const struct device *dev)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_ANACTRL_STATE_RUNNING) {
		/* Abort the in-flight measurement and disarm the poll. */
		config->base->FREQ_ME_CTRL = 0U;
		(void)k_work_cancel_delayable(&data->poll_work);
		data->state = NXP_ANACTRL_STATE_CONFIGURED;
	}

	k_spin_unlock(&data->lock, key);
	return 0;
}

static void nxp_anactrl_poll_work(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct nxp_anactrl_data *data =
		CONTAINER_OF(dwork, struct nxp_anactrl_data, poll_work);
	const struct device *dev = data->dev;
	const struct nxp_anactrl_config *config = dev->config;
	uint32_t evts = 0U;
	uint32_t rate = 0U;
	clock_monitor_callback_t cb = NULL;
	void *user_data = NULL;
	k_spinlock_key_t key;
	bool timed_out = false;
	uint32_t ctrl;
	uint32_t capval = 0U;
	uint32_t ref_hz;

	key = k_spin_lock(&data->lock);

	/* stop() may have torn the cycle down while this run waited for the lock:
	 * k_work_cancel_delayable() cannot stop a handler that already started.
	 */
	if (data->state != NXP_ANACTRL_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	ref_hz = data->ref_hz;
	/* One read serves both jobs: PROG (bit 31) is the busy flag hardware
	 * clears when the reference window elapses, and once it is clear the low
	 * 31 bits hold CAPVAL instead of the SCALE that was written there.
	 */
	ctrl = config->base->FREQ_ME_CTRL;

	if ((ctrl & ANACTRL_FREQ_ME_CTRL_PROG_MASK) != 0U) {
		/* PROG still set: the window has not elapsed. Re-arm the poll
		 * within budget, else the reference clock is not advancing.
		 */
		if (data->retries_left > 0U) {
			data->retries_left--;
			(void)k_work_reschedule(&data->poll_work,
						K_USEC(MAX(1U, data->meas_us / 4U)));
			k_spin_unlock(&data->lock, key);
			return;
		}
		data->clock_lost = true;
		/* Still armed - that is why it timed out. Clearing PROG terminates
		 * it, so no stable state ever has the block running.
		 */
		config->base->FREQ_ME_CTRL = 0U;
		timed_out = true;
		evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
	} else {
		/* PROG cleared, so the low 31 bits are now CAPVAL: the target
		 * edges counted during the window.
		 */
		capval = ctrl & ANACTRL_FREQ_ME_CTRL_CAPVAL_SCALE_MASK;

		if (capval == 0U) {
			data->clock_lost = true;
			evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
		} else {
			/* RM: Ftarget = CAPVAL * Fref / (2^SCALE - 1). */
			uint64_t denom = (1ULL << data->scale) - 1ULL;

			rate = (uint32_t)(((uint64_t)capval * (uint64_t)data->ref_hz) / denom);
			data->last_rate_hz = rate;
			data->has_rate = true;
			/* A fresh good measurement clears any prior CLOCK_LOST so
			 * get_rate() reflects the most recent completed cycle.
			 */
			data->clock_lost = false;
			evts |= CLOCK_MONITOR_EVT_MEASURE_DONE;
		}
	}

	/* Auto-disarm before the callback so it may restart from the callback. */
	data->state = NXP_ANACTRL_STATE_CONFIGURED;
	cb = data->cfg.callback;
	user_data = data->cfg.user_data;
	k_spin_unlock(&data->lock, key);

	/* Outside the lock: immediate-mode logging must not run with irqs off. */
	if (timed_out) {
		LOG_WRN("%s: measurement timed out (FREQ_ME_CTRL=0x%08x, ref_hz=%u) - "
			"reference clock not running?", dev->name, ctrl, ref_hz);
	} else if ((evts & CLOCK_MONITOR_EVT_CLOCK_LOST) != 0U) {
		LOG_WRN("%s: no target edges counted (ref_hz=%u) - "
			"target clock not running/routed?", dev->name, ref_hz);
	} else {
		LOG_DBG("%s: measured %u Hz (capval=%u, ref_hz=%u)",
			dev->name, rate, capval, ref_hz);
	}

	if (cb != NULL) {
		struct clock_monitor_event_data evt = {
			.events = evts,
			.measured_hz = rate,
		};
		cb(dev, &evt, user_data);
	}
}

static int nxp_anactrl_get_rate(const struct device *dev, uint32_t *rate_hz)
{
	struct nxp_anactrl_data *data = dev->data;
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

/*
 * Re-route one side (reference or target) and commit its index right away on success.
 */
static int nxp_anactrl_apply_source(const struct device *dev, struct nxp_anactrl_data *data,
				    const struct nxp_anactrl_source *src, uint8_t new_idx,
				    uint8_t *cur_idx, const char *side)
{
	k_spinlock_key_t key;
	int ret;

	ret = nxp_anactrl_route(src);
	if (ret != 0) {
		LOG_ERR("%s: failed to re-route %s source (%d)", dev->name, side, ret);
		return ret;
	}

	key = k_spin_lock(&data->lock);
	*cur_idx = new_idx;
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_anactrl_set_source(const struct device *dev, uint32_t reference, uint32_t target)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;
	enum nxp_anactrl_state prev_state;
	uint8_t new_ref_idx = ANACTRL_SRC_KEEP;
	uint8_t new_tar_idx = ANACTRL_SRC_KEEP;
	int ret;

	/* Nothing requested at all - a caller error, not a no-op. */
	if (reference == NXP_ANACTRL_SOURCE_UNCHANGED &&
	    target == NXP_ANACTRL_SOURCE_UNCHANGED) {
		return -EINVAL;
	}

	/* Resolve each requested cookie. An unknown cookie - including one for
	 * the wrong axis, which simply isn't in that table - fails with -EINVAL
	 * without touching hardware or state. An axis the caller left at the
	 * "unchanged" sentinel keeps SRC_KEEP.
	 */
	if (reference != NXP_ANACTRL_SOURCE_UNCHANGED &&
	    nxp_anactrl_find_src(config->ref_srcs, config->ref_srcs_count,
				 reference, &new_ref_idx) != 0) {
		return -EINVAL;
	}
	if (target != NXP_ANACTRL_SOURCE_UNCHANGED &&
	    nxp_anactrl_find_src(config->tar_srcs, config->tar_srcs_count,
				 target, &new_tar_idx) != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (nxp_anactrl_busy(data->state)) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	/* Fold "requested, but already selected" into SRC_KEEP as well, so from
	 * here on a non-KEEP index means exactly "this axis must be re-routed".
	 */
	if (new_ref_idx == data->cur_ref_idx) {
		new_ref_idx = ANACTRL_SRC_KEEP;
	}
	if (new_tar_idx == data->cur_tar_idx) {
		new_tar_idx = ANACTRL_SRC_KEEP;
	}
	if (new_ref_idx == ANACTRL_SRC_KEEP && new_tar_idx == ANACTRL_SRC_KEEP) {
		/* Both axes already point where the caller asked. */
		k_spin_unlock(&data->lock, key);
		return 0;
	}
	prev_state = data->state;
	data->state = NXP_ANACTRL_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	/* Re-route the changed source(s). */
	if (new_ref_idx != ANACTRL_SRC_KEEP) {
		ret = nxp_anactrl_apply_source(dev, data, &config->ref_srcs[new_ref_idx],
					       new_ref_idx, &data->cur_ref_idx, "reference");
		if (ret != 0) {
			goto out;
		}
	}
	if (new_tar_idx != ANACTRL_SRC_KEEP) {
		ret = nxp_anactrl_apply_source(dev, data, &config->tar_srcs[new_tar_idx],
					       new_tar_idx, &data->cur_tar_idx, "target");
		if (ret != 0) {
			goto out;
		}
	}

	ret = 0;

out:
	/* Abort the hardware and drop back to IDLE. */
	if (prev_state == NXP_ANACTRL_STATE_CONFIGURED) {
		config->base->FREQ_ME_CTRL = 0U;
	}

	key = k_spin_lock(&data->lock);
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	data->state = NXP_ANACTRL_STATE_IDLE;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int nxp_anactrl_init(const struct device *dev)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;
	int ret;

	data->dev = dev;
	data->state = NXP_ANACTRL_STATE_IDLE;
	data->cur_ref_idx = nxp_anactrl_default_idx(config->ref_srcs,
						    config->ref_srcs_count);
	data->cur_tar_idx = nxp_anactrl_default_idx(config->tar_srcs,
						    config->tar_srcs_count);
	k_work_init_delayable(&data->poll_work, nxp_anactrl_poll_work);

	/* Enable the ANACTRL peripheral gate through clock_control (equivalent
	 * to ANACTRL_Init(), which only calls CLOCK_EnableClock).
	 */
	if (!device_is_ready(config->gate_clk_dev)) {
		LOG_ERR("%s: gate clock device not ready", dev->name);
		return -ENODEV;
	}
	ret = clock_control_on(config->gate_clk_dev, config->gate_clk_subsys);
	if (ret != 0) {
		LOG_ERR("%s: failed to enable gate clock (%d)", dev->name, ret);
		return ret;
	}

	const struct nxp_anactrl_source *ref = &config->ref_srcs[data->cur_ref_idx];
	const struct nxp_anactrl_source *tar = &config->tar_srcs[data->cur_tar_idx];

	ret = nxp_anactrl_route(ref);
	if (ret != 0) {
		LOG_ERR("%s: failed to route reference clock (%d)", dev->name, ret);
		return ret;
	}
	ret = nxp_anactrl_route(tar);
	if (ret != 0) {
		LOG_ERR("%s: failed to route target clock (%d)", dev->name, ret);
		return ret;
	}

	return 0;
}

static DEVICE_API(clock_monitor, nxp_anactrl_api) = {
	.configure  = nxp_anactrl_configure,
	.start      = nxp_anactrl_start,
	.stop       = nxp_anactrl_stop,
	.get_rate   = nxp_anactrl_get_rate,
	.set_source = nxp_anactrl_set_source,
};

/* Frequency provider: a fixed clock-frequency takes precedence; otherwise a
 * clocks phandle is used; a source with neither is rejected by BUILD_ASSERT.
 */
#define NXP_ANACTRL_SRC_CLK_DEV(node)                                          \
	COND_CODE_1(DT_NODE_HAS_PROP(node, clock_frequency), (NULL),           \
		    (COND_CODE_1(DT_NODE_HAS_PROP(node, clocks),               \
				 (DEVICE_DT_GET(DT_CLOCKS_CTLR(node))),        \
				 (NULL))))

#define NXP_ANACTRL_SRC_CLK_SUBSYS(node)                                       \
	COND_CODE_1(DT_NODE_HAS_PROP(node, clock_frequency),                   \
		    ((clock_control_subsys_t)0),                               \
		    (COND_CODE_1(DT_NODE_HAS_PROP(node, clocks),               \
				 ((clock_control_subsys_t)(uintptr_t)          \
					  DT_CLOCKS_CELL(node, name)),         \
				 ((clock_control_subsys_t)0))))

/* Emit static mux_state storage for one source child node (file scope). */
#define NXP_ANACTRL_SRC_MUX_DEFINE(node) MUX_STATE_DT_SPEC_DEFINE(node);

#define NXP_ANACTRL_SRC_ENTRY(node)                                            \
	{                                                                      \
		.mux_dev   = MUX_STATE_DT_DEV_GET(node),                       \
		.mux_state = MUX_STATE_DT_GET(node),                           \
		.clk_dev = NXP_ANACTRL_SRC_CLK_DEV(node),                      \
		.clk_subsys = NXP_ANACTRL_SRC_CLK_SUBSYS(node),                \
		.fixed_hz = (uint32_t)DT_PROP_OR(node, clock_frequency, 0),    \
		.has_fixed_hz = DT_NODE_HAS_PROP(node, clock_frequency),       \
		.is_default = DT_PROP(node, default_source),                   \
	}

/* Per-source compile-time checks: a usable frequency provider on references,
 * and a nonzero fixed frequency when one is given.
 */
#define NXP_ANACTRL_SRC_ASSERT(node)                                           \
	BUILD_ASSERT(DT_NODE_HAS_PROP(node, clock_frequency) ||                \
			     DT_NODE_HAS_PROP(node, clocks),                   \
		     "nxp,anactrl-freqme source needs clocks or clock-frequency"); \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node, clock_frequency) ||               \
			     DT_PROP_OR(node, clock_frequency, 0) != 0,        \
		     "nxp,anactrl-freqme clock-frequency must be nonzero");

/* Target sources may omit a frequency provider (the target rate is measured);
 * only reject a given fixed frequency that is zero.
 */
#define NXP_ANACTRL_TAR_SRC_ASSERT(node)                                       \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node, clock_frequency) ||               \
			     DT_PROP_OR(node, clock_frequency, 0) != 0,        \
		     "nxp,anactrl-freqme clock-frequency must be nonzero");

/* Count of default-source children in a container (summed in a BUILD_ASSERT). */
#define NXP_ANACTRL_DEFAULT_INC(node) + DT_PROP(node, default_source)

#define NXP_ANACTRL_REF_NODE(inst) DT_INST_CHILD(inst, reference_sources)
#define NXP_ANACTRL_TAR_NODE(inst) DT_INST_CHILD(inst, target_sources)

#define NXP_ANACTRL_DEVICE_INIT(inst)                                          \
	BUILD_ASSERT(DT_NODE_EXISTS(NXP_ANACTRL_REF_NODE(inst)),               \
		     "nxp,anactrl-freqme: missing reference-sources node");    \
	BUILD_ASSERT(DT_NODE_EXISTS(NXP_ANACTRL_TAR_NODE(inst)),               \
		     "nxp,anactrl-freqme: missing target-sources node");       \
	BUILD_ASSERT(DT_CHILD_NUM(NXP_ANACTRL_REF_NODE(inst)) >= 1,            \
		     "nxp,anactrl-freqme: reference-sources needs >= 1 source"); \
	BUILD_ASSERT(DT_CHILD_NUM(NXP_ANACTRL_TAR_NODE(inst)) >= 1,            \
		     "nxp,anactrl-freqme: target-sources needs >= 1 source");  \
	BUILD_ASSERT((0 DT_FOREACH_CHILD(NXP_ANACTRL_REF_NODE(inst),           \
					 NXP_ANACTRL_DEFAULT_INC)) == 1,       \
		     "nxp,anactrl-freqme: reference-sources needs exactly one default-source"); \
	BUILD_ASSERT((0 DT_FOREACH_CHILD(NXP_ANACTRL_TAR_NODE(inst),           \
					 NXP_ANACTRL_DEFAULT_INC)) == 1,       \
		     "nxp,anactrl-freqme: target-sources needs exactly one default-source"); \
	DT_FOREACH_CHILD(NXP_ANACTRL_REF_NODE(inst), NXP_ANACTRL_SRC_ASSERT)   \
	DT_FOREACH_CHILD(NXP_ANACTRL_TAR_NODE(inst), NXP_ANACTRL_TAR_SRC_ASSERT) \
	DT_FOREACH_CHILD(NXP_ANACTRL_REF_NODE(inst), NXP_ANACTRL_SRC_MUX_DEFINE) \
	DT_FOREACH_CHILD(NXP_ANACTRL_TAR_NODE(inst), NXP_ANACTRL_SRC_MUX_DEFINE) \
	static const struct nxp_anactrl_source nxp_anactrl_ref_srcs_##inst[] = { \
		DT_FOREACH_CHILD_SEP(NXP_ANACTRL_REF_NODE(inst),               \
				     NXP_ANACTRL_SRC_ENTRY, (,))               \
	};                                                                     \
	static const struct nxp_anactrl_source nxp_anactrl_tar_srcs_##inst[] = { \
		DT_FOREACH_CHILD_SEP(NXP_ANACTRL_TAR_NODE(inst),               \
				     NXP_ANACTRL_SRC_ENTRY, (,))               \
	};                                                                     \
	static struct nxp_anactrl_data nxp_anactrl_data_##inst;                \
	static const struct nxp_anactrl_config nxp_anactrl_cfg_##inst = {      \
		.base = (ANACTRL_Type *)DT_INST_REG_ADDR(inst),                \
		.gate_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),      \
		.gate_clk_subsys = (clock_control_subsys_t)(uintptr_t)         \
			DT_INST_CLOCKS_CELL(inst, name),                       \
		.ref_srcs = nxp_anactrl_ref_srcs_##inst,                       \
		.ref_srcs_count =                                              \
			(uint8_t)ARRAY_SIZE(nxp_anactrl_ref_srcs_##inst),      \
		.tar_srcs = nxp_anactrl_tar_srcs_##inst,                       \
		.tar_srcs_count =                                              \
			(uint8_t)ARRAY_SIZE(nxp_anactrl_tar_srcs_##inst),      \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_anactrl_init, NULL,                    \
			      &nxp_anactrl_data_##inst, &nxp_anactrl_cfg_##inst, \
			      POST_KERNEL, CONFIG_CLOCK_MONITOR_INIT_PRIORITY, \
			      &nxp_anactrl_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_ANACTRL_DEVICE_INIT)
