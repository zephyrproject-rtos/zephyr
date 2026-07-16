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
 * state so the next configure() re-derives the measurement parameters. The
 * peripheral gate is enabled through clock_control.
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
#include <fsl_inputmux.h>

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
 * A selectable ANACTRL freq-measure clock source: an INPUTMUX route paired
 * with a frequency provider. The provider is a fixed constant (has_fixed_hz)
 * or a clock_control subsystem (clk_dev / clk_subsys); a fixed provider takes
 * precedence when both are declared. Exactly one source per container is
 * flagged is_default.
 */
struct nxp_anactrl_source {
	INPUTMUX_Type          *inputmux_base;
	uint16_t                channel;
	uint32_t                connection;
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
	/* configure() is between its lock-free clock query / setup and its
	 * commit; concurrent configure()/start()/set_source() get -EBUSY.
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

/*
 * Convert a measurement window (ns) into the SCALE exponent. The ANACTRL
 * window is (2^SCALE - 1) reference-clock periods, i.e. approximately a
 * rounded base-2 logarithm of the requested cycle count, clamped to
 * [ANACTRL_SCALE_MIN, ANACTRL_SCALE_MAX].
 */
static int anactrl_window_to_scale(uint32_t window_ns, uint32_t ref_hz, uint8_t *out)
{
	uint64_t cycles = (uint64_t)window_ns * (uint64_t)ref_hz / 1000000000ULL;

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
	uint64_t ns = ((uint64_t)((1ULL << scale) - 1ULL) * 1000000000ULL) / ref_hz;
	uint32_t us = (uint32_t)(ns / 1000ULL);

	return (us == 0U) ? 1U : us;
}

/* Route a source's INPUTMUX connection to its ANACTRL freq-measure
 * destination. Has no failure path.
 */
static void nxp_anactrl_route(const struct nxp_anactrl_source *src)
{
	INPUTMUX_Init(src->inputmux_base);
	INPUTMUX_AttachSignal(src->inputmux_base, src->channel,
			      (inputmux_connection_t)src->connection);
}

/*
 * Enable a source's clock. Fixed-frequency sources need no gating and are a
 * no-op; clock_control-backed sources are turned on (after a readiness check).
 */
static int nxp_anactrl_src_enable(const struct nxp_anactrl_source *src)
{
	if (src->has_fixed_hz || src->clk_dev == NULL) {
		return 0;
	}
	if (!device_is_ready(src->clk_dev)) {
		return -ENODEV;
	}
	return clock_control_on(src->clk_dev, src->clk_subsys);
}

/*
 * Resolve a source's frequency in Hz. A fixed provider is returned verbatim;
 * otherwise the clock_control rate is queried. Returns -EIO when the rate is
 * unavailable or zero.
 */
static int nxp_anactrl_src_rate(const struct nxp_anactrl_source *src, uint32_t *hz)
{
	uint32_t rate = 0U;
	int ret;

	if (src->has_fixed_hz) {
		*hz = src->fixed_hz;
		return 0;
	}
	if (src->clk_dev == NULL) {
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
static int nxp_anactrl_find_src(const struct nxp_anactrl_source *srcs, uint8_t count,
				uint32_t cookie, uint8_t *out_idx)
{
	for (uint8_t i = 0U; i < count; i++) {
		if (srcs[i].connection == cookie) {
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
	enum nxp_anactrl_state prev_state;
	uint8_t ref_idx;
	uint8_t tar_idx;
	int ret;

	/* ANACTRL freq-measure hardware is one-shot only; WINDOW has no HW. */
	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE) {
		return -ENOTSUP;
	}
	if (cfg->measure.window_ns == 0U) {
		return -EINVAL;
	}

	/* Claim the configure transaction and snapshot the current source
	 * selection: CONFIGURING makes concurrent configure()/start()/
	 * set_source() callers fail with -EBUSY, so the clock query and setup
	 * below run against a stable ref_idx / tar_idx outside the spinlock.
	 */
	key = k_spin_lock(&data->lock);
	if (data->state == NXP_ANACTRL_STATE_RUNNING ||
	    data->state == NXP_ANACTRL_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	prev_state = data->state;
	ref_idx = data->cur_ref_idx;
	tar_idx = data->cur_tar_idx;
	data->state = NXP_ANACTRL_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	const struct nxp_anactrl_source *ref = &config->ref_srcs[ref_idx];
	const struct nxp_anactrl_source *tar = &config->tar_srcs[tar_idx];

	/* Ensure both selected sources are clocked before measuring. */
	ret = nxp_anactrl_src_enable(ref);
	if (ret != 0) {
		goto restore;
	}
	ret = nxp_anactrl_src_enable(tar);
	if (ret != 0) {
		goto restore;
	}

	uint32_t ref_hz = 0U;

	ret = nxp_anactrl_src_rate(ref, &ref_hz);
	if (ret != 0 || ref_hz == 0U) {
		ret = -EIO;
		goto restore;
	}

	uint8_t scale;

	ret = anactrl_window_to_scale(cfg->measure.window_ns, ref_hz, &scale);
	if (ret != 0) {
		LOG_ERR("window_ns=%u out of range for ref_hz=%u",
			cfg->measure.window_ns, ref_hz);
		ret = -EINVAL;
		goto restore;
	}

	/* Route both selected sources through INPUTMUX. */
	nxp_anactrl_route(ref);
	nxp_anactrl_route(tar);

	key = k_spin_lock(&data->lock);
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

restore:
	/* Hardware untouched: the previous configuration is still valid. */
	key = k_spin_lock(&data->lock);
	data->state = prev_state;
	k_spin_unlock(&data->lock, key);
	return ret;
}

/* Kick a fresh measurement cycle: set PROG with the cached SCALE. */
static inline void nxp_anactrl_kick(ANACTRL_Type *base, uint8_t scale)
{
	base->FREQ_ME_CTRL =
		ANACTRL_FREQ_ME_CTRL_PROG_MASK | ANACTRL_FREQ_ME_CTRL_CAPVAL_SCALE(scale);
}

static int nxp_anactrl_start(const struct device *dev)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_ANACTRL_STATE_RUNNING ||
	    data->state == NXP_ANACTRL_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	if (data->state != NXP_ANACTRL_STATE_CONFIGURED) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/* State transition, HW kick and poll arm share one critical section so
	 * the poll handler can never observe a RUNNING measurement with a
	 * not-yet-RUNNING state machine.
	 */
	data->state = NXP_ANACTRL_STATE_RUNNING;
	data->retries_left = ANACTRL_POLL_RETRIES;
	nxp_anactrl_kick(config->base, data->scale);
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
		data->state = NXP_ANACTRL_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);

	/* Cancel outside the spinlock; the state guard above makes a
	 * concurrently-running handler a no-op.
	 */
	(void)k_work_cancel_delayable(&data->poll_work);
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

	key = k_spin_lock(&data->lock);

	/* A pending poll may run after stop() (or a fresh configure()) has
	 * already left the RUNNING state; discard it.
	 */
	if (data->state != NXP_ANACTRL_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	uint32_t ctrl = config->base->FREQ_ME_CTRL;

	if ((ctrl & ANACTRL_FREQ_ME_CTRL_PROG_MASK) != 0U) {
		/* Measurement still in progress: re-arm within budget, else the
		 * target clock is stuck.
		 */
		if (data->retries_left > 0U) {
			data->retries_left--;
			(void)k_work_reschedule(&data->poll_work,
						K_USEC(MAX(1U, data->meas_us / 4U)));
			k_spin_unlock(&data->lock, key);
			return;
		}
		data->clock_lost = true;
		config->base->FREQ_ME_CTRL = 0U;
		evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
	} else {
		uint32_t capval = ctrl & ANACTRL_FREQ_ME_CTRL_CAPVAL_SCALE_MASK;

		if (capval == 0U) {
			/* No target edges counted: clock lost. */
			data->clock_lost = true;
			evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
		} else {
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

static int nxp_anactrl_set_source(const struct device *dev, uint32_t reference, uint32_t target)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;
	enum nxp_anactrl_state prev_state;
	uint8_t req_ref_idx = 0U;
	uint8_t req_tar_idx = 0U;
	uint8_t new_ref_idx;
	uint8_t new_tar_idx;
	bool ref_req = (reference != NXP_ANACTRL_SOURCE_UNCHANGED);
	bool tar_req = (target != NXP_ANACTRL_SOURCE_UNCHANGED);
	bool ref_changed;
	bool tar_changed;
	int ret;

	if (!ref_req && !tar_req) {
		return -EINVAL;
	}

	/* Resolve the requested cookies up front. An unknown cookie - including
	 * one for the wrong axis, which simply isn't in that table - fails with
	 * -EINVAL without touching hardware or state.
	 */
	if (ref_req && nxp_anactrl_find_src(config->ref_srcs, config->ref_srcs_count,
					    reference, &req_ref_idx) != 0) {
		return -EINVAL;
	}
	if (tar_req && nxp_anactrl_find_src(config->tar_srcs, config->tar_srcs_count,
					    target, &req_tar_idx) != 0) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_ANACTRL_STATE_RUNNING ||
	    data->state == NXP_ANACTRL_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	new_ref_idx = ref_req ? req_ref_idx : data->cur_ref_idx;
	new_tar_idx = tar_req ? req_tar_idx : data->cur_tar_idx;
	ref_changed = (new_ref_idx != data->cur_ref_idx);
	tar_changed = (new_tar_idx != data->cur_tar_idx);
	if (!ref_changed && !tar_changed) {
		/* Resolved no-op: already selected. */
		k_spin_unlock(&data->lock, key);
		return 0;
	}
	prev_state = data->state;
	data->state = NXP_ANACTRL_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	/* Enable first (may fail), route second (cannot fail): if an enable
	 * fails nothing has been re-routed yet, so restore is clean. The
	 * previously selected source's clock is intentionally left on - it may
	 * be shared with other consumers.
	 */
	if (ref_changed) {
		ret = nxp_anactrl_src_enable(&config->ref_srcs[new_ref_idx]);
		if (ret != 0) {
			goto restore;
		}
	}
	if (tar_changed) {
		ret = nxp_anactrl_src_enable(&config->tar_srcs[new_tar_idx]);
		if (ret != 0) {
			goto restore;
		}
	}
	if (ref_changed) {
		nxp_anactrl_route(&config->ref_srcs[new_ref_idx]);
	}
	if (tar_changed) {
		nxp_anactrl_route(&config->tar_srcs[new_tar_idx]);
	}

	/* A prior configuration measured the old source(s). Abort the hardware
	 * and drop back to IDLE: the reference rate may have changed, so the
	 * cached SCALE is stale and a fresh configure() must re-derive it.
	 */
	if (prev_state == NXP_ANACTRL_STATE_CONFIGURED) {
		config->base->FREQ_ME_CTRL = 0U;
	}

	key = k_spin_lock(&data->lock);
	data->cur_ref_idx = new_ref_idx;
	data->cur_tar_idx = new_tar_idx;
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	data->state = NXP_ANACTRL_STATE_IDLE;
	k_spin_unlock(&data->lock, key);
	return 0;

restore:
	/* Enable failed before any re-route: the previous selection still holds. */
	key = k_spin_lock(&data->lock);
	data->state = prev_state;
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

	ret = nxp_anactrl_src_enable(ref);
	if (ret != 0) {
		LOG_ERR("%s: failed to enable reference clock (%d)", dev->name, ret);
		return ret;
	}
	/* The target rate is not needed by MEASURE, so a failure to enable it
	 * is non-fatal.
	 */
	ret = nxp_anactrl_src_enable(tar);
	if (ret != 0) {
		LOG_WRN("%s: failed to enable target clock (%d)", dev->name, ret);
	}

	nxp_anactrl_route(ref);
	nxp_anactrl_route(tar);

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

#define NXP_ANACTRL_SRC_ENTRY(node)                                            \
	{                                                                      \
		.inputmux_base = (INPUTMUX_Type *)DT_REG_ADDR(                 \
			DT_PHANDLE_BY_IDX(node, inputmux_connection, 0)),      \
		.channel = (uint16_t)DT_PHA_BY_IDX(node, inputmux_connection,  \
						   0, channel),                \
		.connection = (uint32_t)DT_PHA_BY_IDX(node, inputmux_connection, \
						      0, connection),          \
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
