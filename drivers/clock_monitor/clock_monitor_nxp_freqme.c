/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for the NXP FREQME (Frequency Measurement)
 * module. Built on the FREQME frequency-measurement operate mode, it exposes
 * both clock_monitor abstraction modes:
 *
 *   - MEASURE: one-shot measurement; the result-ready interrupt delivers a
 *     MEASURE_DONE event and get_rate() returns the measured frequency.
 *   - WINDOW:  continuous measurement against MIN/MAX thresholds derived from
 *     the expected frequency and tolerance; the over-/under-range interrupts
 *     deliver FREQ_HIGH / FREQ_LOW events.
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
 * The source "clocks" phandle is only queried (clock_control_get_rate()) to
 * derive REF_SCALE and the WINDOW thresholds, never turned on. If a source is
 * left off the measurement simply counts zero edges and surfaces CLOCK_LOST.
 * Only the peripheral's own gate clock is enabled here, through clock_control.
 */

#define DT_DRV_COMPAT nxp_freqme

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/dt-bindings/clock-monitor/nxp-freqme.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_freqme.h>
#include <fsl_inputmux.h>

LOG_MODULE_REGISTER(clock_monitor_nxp_freqme, CONFIG_CLOCK_MONITOR_LOG_LEVEL);

/* Measurement result counter is 31 bits; the formula offset is +2. */
#define FREQME_RESULT_MAX  0x7FFFFFFFU
/* HW formula offset: Ftarget = (RESULT - 2) * Fref / 2^REF_SCALE, so the target
 * edge count is RESULT - 2. RESULT <= 2 means zero edges were counted (clock
 * lost). Fixed by silicon.
 */
#define FREQME_RESULT_BIAS 2U
/* WINDOW guard band (counts) added each side of MIN/MAX to absorb +/-1 LSB
 * quantization plus nominal truncation, so an in-tolerance clock isn't flagged.
 */
#define FREQME_WINDOW_MARGIN 2U

/*
 * A selectable FREQME clock source: an INPUTMUX route paired with a frequency
 * provider. The provider is a fixed constant (has_fixed_hz) or a clock_control
 * subsystem (clk_dev / clk_subsys); a fixed provider takes precedence when both
 * are declared. Exactly one source per container is flagged is_default.
 */
struct nxp_freqme_source {
	INPUTMUX_Type          *inputmux_base;
	uint16_t                channel;
	uint32_t                connection;
	const struct device    *clk_dev;
	clock_control_subsys_t  clk_subsys;
	uint32_t                fixed_hz;
	bool                    has_fixed_hz;
	bool                    is_default;
};

struct nxp_freqme_config {
	FREQME_Type *base;
	/* Peripheral gate clock (clocks "ipg"), enabled via clock_control_on(). */
	const struct device    *gate_clk_dev;
	clock_control_subsys_t  gate_clk_subsys;
	/* Selectable reference / target sources; the is_default source is the
	 * power-on selection.
	 */
	const struct nxp_freqme_source *ref_srcs;
	uint8_t ref_srcs_count;
	const struct nxp_freqme_source *tar_srcs;
	uint8_t tar_srcs_count;
	void (*irq_config_func)(const struct device *dev);
};

enum nxp_freqme_state {
	NXP_FREQME_STATE_IDLE = 0,
	/* configure() is between its lock-free clock query / HAL init and its
	 * commit; concurrent configure()/start()/set_source() get -EBUSY.
	 */
	NXP_FREQME_STATE_CONFIGURING,
	NXP_FREQME_STATE_CONFIGURED,
	NXP_FREQME_STATE_RUNNING,
};

struct nxp_freqme_data {
	/* Protects the state machine, cfg, source selection and result fields
	 * against the ISR and against concurrent API calls.
	 */
	struct k_spinlock lock;
	enum nxp_freqme_state state;
	struct clock_monitor_config cfg;
	/* Index into config->ref_srcs / tar_srcs for the currently selected
	 * reference / target source. Only ever written while CONFIGURING (which
	 * excludes configure()/set_source()) so lock-free reads there are safe.
	 */
	uint8_t cur_ref_idx;
	uint8_t cur_tar_idx;
	uint32_t ref_hz;     /* cached from the reference source at configure */
	uint8_t  ref_scale;  /* cached REF_SCALE for ISR formula / re-arm */
	/* Most recent completed MEASURE result; retained across reads. */
	uint32_t last_rate_hz;
	bool has_rate;
	/* CLOCK_LOST latched since configure(). */
	bool clock_lost;
};

/*
 * Convert a measurement window (ns) into the REF_SCALE power-of-two exponent.
 * The FREQME window is 2^REF_SCALE reference-clock periods, so this is a
 * rounded base-2 logarithm of the requested cycle count, clamped to [0, 31].
 */
static int freqme_window_to_ref_scale(uint32_t window_ns, uint32_t ref_hz, uint8_t *out)
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
	if (s < 31U && (cycles - (1ULL << s)) > ((1ULL << (s + 1U)) - cycles)) {
		s++;
	}
	if (s > 31U) {
		/* REF_SCALE is a 5-bit field (max 31); a window this long cannot
		 * be represented. Reject rather than silently shorten it.
		 */
		return -ERANGE;
	}

	*out = (uint8_t)s;
	return 0;
}

/*
 * Derive the MIN/MAX result-counter thresholds for WINDOW mode from an already
 * resolved expected target frequency (caller supplies it from cfg or from the
 * current target source rate). RESULT = round(f * 2^scale / ref) +
 * FREQME_RESULT_BIAS, so the nominal count is scaled by (1 +/- tol_ppm) and
 * biased; a small margin absorbs quantization jitter.
 *
 * Returns -ERANGE if the thresholds cannot be represented.
 */
static int freqme_compute_thresholds(uint32_t expected_hz, uint32_t tol_ppm,
				     uint8_t ref_scale, uint32_t ref_hz,
				     uint32_t *min_val, uint32_t *max_val)
{
	uint64_t scale = 1ULL << ref_scale;
	uint64_t nominal = (uint64_t)expected_hz * scale / (uint64_t)ref_hz;

	uint64_t hi = nominal * (1000000ULL + tol_ppm) / 1000000ULL
		      + FREQME_RESULT_BIAS + FREQME_WINDOW_MARGIN;
	uint64_t lo = nominal * (1000000ULL - tol_ppm) / 1000000ULL
		      + FREQME_RESULT_BIAS;

	if (hi > FREQME_RESULT_MAX) {
		LOG_ERR("FREQME WINDOW MAX %llu exceeds HW limit (expected_hz=%u, "
			"tol_ppm=%u, ref_scale=%u, ref_hz=%u)", hi, expected_hz,
			tol_ppm, ref_scale, ref_hz);
		return -ERANGE;
	}

	lo = (lo > FREQME_WINDOW_MARGIN) ? (lo - FREQME_WINDOW_MARGIN) : 0ULL;
	if (lo < FREQME_RESULT_BIAS) {
		lo = FREQME_RESULT_BIAS;
	}
	if (lo >= hi) {
		LOG_ERR("FREQME WINDOW thresholds collapse (min=%llu, max=%llu)", lo, hi);
		return -ERANGE;
	}

	*min_val = (uint32_t)lo;
	*max_val = (uint32_t)hi;
	return 0;
}

/* Route a source's INPUTMUX connection to its FREQME destination. Has no
 * failure path.
 */
static void nxp_freqme_route(const struct nxp_freqme_source *src)
{
	INPUTMUX_Init(src->inputmux_base);
	INPUTMUX_AttachSignal(src->inputmux_base, src->channel,
			      (inputmux_connection_t)src->connection);
	INPUTMUX_Deinit(src->inputmux_base);
}

/*
 * Resolve a source's frequency in Hz. A fixed provider is returned verbatim;
 * otherwise the clock_control rate is queried. The driver does not enable the
 * source clock - the caller owns that - so this only reads the rate. Returns
 * -EIO when the provider device is not ready or the rate is unavailable/zero.
 */
static int nxp_freqme_src_rate(const struct nxp_freqme_source *src, uint32_t *hz)
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
static int nxp_freqme_find_src(const struct nxp_freqme_source *srcs, uint8_t count,
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
static uint8_t nxp_freqme_default_idx(const struct nxp_freqme_source *srcs, uint8_t count)
{
	for (uint8_t i = 0U; i < count; i++) {
		if (srcs[i].is_default) {
			return i;
		}
	}
	return 0U;
}

static int nxp_freqme_configure(const struct device *dev,
				const struct clock_monitor_config *cfg)
{
	const struct nxp_freqme_config *config = dev->config;
	struct nxp_freqme_data *data = dev->data;
	k_spinlock_key_t key;
	enum nxp_freqme_state prev_state;
	uint8_t ref_idx;
	uint8_t tar_idx;
	int ret;

	/* Input-only validation, no state needed. */
	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE &&
	    cfg->mode != CLOCK_MONITOR_MODE_WINDOW) {
		return -ENOTSUP;
	}

	if (cfg->mode == CLOCK_MONITOR_MODE_MEASURE) {
		if (cfg->measure.window_ns == 0U) {
			return -EINVAL;
		}
	} else {
		if (cfg->window.window_ns == 0U ||
		    cfg->window.tolerance_ppm >= 1000000U) {
			return -EINVAL;
		}
	}

	/* Claim the configure transaction and snapshot the current source
	 * selection: CONFIGURING makes concurrent configure()/start()/
	 * set_source() callers fail with -EBUSY, so the clock query and HAL init
	 * below run against a stable ref_idx / tar_idx outside the spinlock.
	 */
	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FREQME_STATE_RUNNING ||
	    data->state == NXP_FREQME_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	prev_state = data->state;
	ref_idx = data->cur_ref_idx;
	tar_idx = data->cur_tar_idx;
	data->state = NXP_FREQME_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	const struct nxp_freqme_source *ref = &config->ref_srcs[ref_idx];
	const struct nxp_freqme_source *tar = &config->tar_srcs[tar_idx];

	/* The caller must have already enabled both selected source clocks; this
	 * driver only queries their rate, never turns them on.
	 */
	uint32_t ref_hz = 0U;

	ret = nxp_freqme_src_rate(ref, &ref_hz);
	if (ret != 0) {
		goto restore;
	}

	uint32_t window_ns = (cfg->mode == CLOCK_MONITOR_MODE_MEASURE)
				     ? cfg->measure.window_ns
				     : cfg->window.window_ns;
	uint8_t ref_scale;

	ret = freqme_window_to_ref_scale(window_ns, ref_hz, &ref_scale);
	if (ret != 0) {
		LOG_ERR("window_ns=%u out of range for ref_hz=%u", window_ns, ref_hz);
		ret = -EINVAL;
		goto restore;
	}

	uint32_t min_val = 0U;
	uint32_t max_val = FREQME_RESULT_MAX;
	uint32_t int_mask;

	if (cfg->mode == CLOCK_MONITOR_MODE_WINDOW) {
		uint32_t expected_hz = cfg->window.expected_hz;

		if (expected_hz == 0U) {
			/* Auto-derive the expected frequency from the current
			 * target source.
			 */
			ret = nxp_freqme_src_rate(tar, &expected_hz);
			if (ret != 0) {
				ret = -EINVAL;
				goto restore;
			}
		}
		ret = freqme_compute_thresholds(expected_hz,
						cfg->window.tolerance_ppm,
						ref_scale, ref_hz,
						&min_val, &max_val);
		if (ret != 0) {
			ret = -EINVAL;
			goto restore;
		}
		int_mask = (uint32_t)kFREQME_OverflowInterruptEnable |
			   (uint32_t)kFREQME_UnderflowInterruptEnable;
	} else {
		int_mask = (uint32_t)kFREQME_ReadyInterruptEnable;
	}

	freq_measure_config_t hal_cfg;

	FREQME_GetDefaultConfig(&hal_cfg);
	hal_cfg.operateMode = kFREQME_FreqMeasurementMode;
	hal_cfg.operateModeAttribute.refClkScaleFactor = ref_scale;
	hal_cfg.enableContinuousMode = (cfg->mode == CLOCK_MONITOR_MODE_WINDOW);
	hal_cfg.startMeasurement = false;

	FREQME_Init(config->base, &hal_cfg);
	FREQME_SetMinExpectedValue(config->base, min_val);
	FREQME_SetMaxExpectedValue(config->base, max_val);
	FREQME_EnableInterrupts(config->base, int_mask);

	key = k_spin_lock(&data->lock);
	data->cfg = *cfg;
	data->ref_hz = ref_hz;
	data->ref_scale = ref_scale;
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	data->state = NXP_FREQME_STATE_CONFIGURED;
	k_spin_unlock(&data->lock, key);
	return 0;

restore:
	/* Hardware untouched: the previous configuration is still valid. */
	key = k_spin_lock(&data->lock);
	data->state = prev_state;
	k_spin_unlock(&data->lock, key);
	return ret;
}

static int nxp_freqme_start(const struct device *dev)
{
	const struct nxp_freqme_config *config = dev->config;
	struct nxp_freqme_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FREQME_STATE_RUNNING ||
	    data->state == NXP_FREQME_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	if (data->state != NXP_FREQME_STATE_CONFIGURED) {
		k_spin_unlock(&data->lock, key);
		return -EINVAL;
	}

	/* State transition and HW kick share one critical section so the
	 * completion ISR can never observe RUNNING hardware with a
	 * not-yet-RUNNING state machine.
	 */
	data->state = NXP_FREQME_STATE_RUNNING;
	if (data->cfg.mode == CLOCK_MONITOR_MODE_WINDOW) {
		FREQME_EnableContinuousMode(config->base, true);
	}
	FREQME_StartMeasurementCycle(config->base);
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_freqme_stop(const struct device *dev)
{
	const struct nxp_freqme_config *config = dev->config;
	struct nxp_freqme_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FREQME_STATE_RUNNING) {
		if (data->cfg.mode == CLOCK_MONITOR_MODE_WINDOW) {
			FREQME_EnableContinuousMode(config->base, false);
		}
		FREQME_TerminateMeasurementCycle(config->base);
		data->state = NXP_FREQME_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_freqme_get_rate(const struct device *dev, uint32_t *rate_hz)
{
	struct nxp_freqme_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&data->lock);
	if (data->cfg.mode == CLOCK_MONITOR_MODE_WINDOW) {
		ret = -EAGAIN;
	} else if (data->clock_lost) {
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

static int nxp_freqme_set_source(const struct device *dev, uint32_t reference, uint32_t target)
{
	const struct nxp_freqme_config *config = dev->config;
	struct nxp_freqme_data *data = dev->data;
	k_spinlock_key_t key;
	enum nxp_freqme_state prev_state;
	uint8_t req_ref_idx = 0U;
	uint8_t req_tar_idx = 0U;
	uint8_t new_ref_idx;
	uint8_t new_tar_idx;
	bool ref_req = (reference != NXP_FREQME_SOURCE_UNCHANGED);
	bool tar_req = (target != NXP_FREQME_SOURCE_UNCHANGED);
	bool ref_changed;
	bool tar_changed;

	if (!ref_req && !tar_req) {
		return -EINVAL;
	}

	/* Resolve the requested cookies up front. An unknown cookie - including
	 * one for the wrong axis, which simply isn't in that table - fails with
	 * -EINVAL without touching hardware or state. -ENOTSUP is reserved by
	 * the API for back-ends that cannot switch sources at all.
	 */
	if (ref_req && nxp_freqme_find_src(config->ref_srcs, config->ref_srcs_count,
					   reference, &req_ref_idx) != 0) {
		return -EINVAL;
	}
	if (tar_req && nxp_freqme_find_src(config->tar_srcs, config->tar_srcs_count,
					   target, &req_tar_idx) != 0) {
		return -EINVAL;
	}

	/* Claim the transaction against the current selection. */
	key = k_spin_lock(&data->lock);
	if (data->state == NXP_FREQME_STATE_RUNNING ||
	    data->state == NXP_FREQME_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}
	new_ref_idx = ref_req ? req_ref_idx : data->cur_ref_idx;
	new_tar_idx = tar_req ? req_tar_idx : data->cur_tar_idx;
	ref_changed = (new_ref_idx != data->cur_ref_idx);
	tar_changed = (new_tar_idx != data->cur_tar_idx);
	if (!ref_changed && !tar_changed) {
		/* Resolved no-op: already selected. Leave state and configured
		 * measurement parameters intact.
		 */
		k_spin_unlock(&data->lock, key);
		return 0;
	}
	prev_state = data->state;
	data->state = NXP_FREQME_STATE_CONFIGURING;
	k_spin_unlock(&data->lock, key);

	/* Re-route the changed source(s). The driver does not enable source
	 * clocks - the caller owns that - and routing cannot fail, so there is
	 * no failure path here. The previously selected source's clock is left
	 * running: it may be shared with other consumers and is not the driver's
	 * to gate off.
	 */
	if (ref_changed) {
		nxp_freqme_route(&config->ref_srcs[new_ref_idx]);
	}
	if (tar_changed) {
		nxp_freqme_route(&config->tar_srcs[new_tar_idx]);
	}

	/* A prior configuration measured the old source(s). Disarm the hardware
	 * (without FREQME_Deinit) and drop back to IDLE: the reference rate may
	 * have changed, so the cached REF_SCALE and WINDOW thresholds are stale
	 * and a fresh configure() must re-derive them.
	 */
	if (prev_state == NXP_FREQME_STATE_CONFIGURED) {
		if (data->cfg.mode == CLOCK_MONITOR_MODE_WINDOW) {
			FREQME_EnableContinuousMode(config->base, false);
		}
		FREQME_TerminateMeasurementCycle(config->base);
		FREQME_DisableInterrupts(config->base,
					 (uint32_t)kFREQME_ReadyInterruptEnable |
					 (uint32_t)kFREQME_OverflowInterruptEnable |
					 (uint32_t)kFREQME_UnderflowInterruptEnable);
		FREQME_ClearInterruptStatusFlags(config->base,
			FREQME_GetInterruptStatusFlags(config->base));
	}

	key = k_spin_lock(&data->lock);
	data->cur_ref_idx = new_ref_idx;
	data->cur_tar_idx = new_tar_idx;
	data->last_rate_hz = 0U;
	data->has_rate = false;
	data->clock_lost = false;
	data->state = NXP_FREQME_STATE_IDLE;
	k_spin_unlock(&data->lock, key);
	return 0;
}

static void nxp_freqme_isr(const struct device *dev)
{
	const struct nxp_freqme_config *config = dev->config;
	struct nxp_freqme_data *data = dev->data;
	uint32_t flags = FREQME_GetInterruptStatusFlags(config->base);
	uint32_t evts = 0U;
	uint32_t rate = 0U;
	clock_monitor_callback_t cb = NULL;
	void *user_data = NULL;
	k_spinlock_key_t key;

	FREQME_ClearInterruptStatusFlags(config->base, flags);

	key = k_spin_lock(&data->lock);

	/* A pending interrupt may run after stop() (or a fresh configure())
	 * has already left the RUNNING state; discard it.
	 */
	if (data->state != NXP_FREQME_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (data->cfg.mode == CLOCK_MONITOR_MODE_MEASURE) {
		if ((flags & (uint32_t)kFREQME_ReadyInterruptStatusFlag) != 0U) {
			uint32_t result = FREQME_GetMeasurementResult(config->base);

			/* target edge count = result - FREQME_RESULT_BIAS, so
			 * result <= bias means zero edges: clock lost.
			 */
			if (result <= FREQME_RESULT_BIAS) {
				data->clock_lost = true;
				evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
			} else {
				rate = FREQME_CalculateTargetClkFreq(config->base,
								     data->ref_hz);
				data->last_rate_hz = rate;
				data->has_rate = true;
				/* A fresh good measurement clears any prior
				 * CLOCK_LOST so get_rate() reflects the most
				 * recent completed cycle.
				 */
				data->clock_lost = false;
				evts |= CLOCK_MONITOR_EVT_MEASURE_DONE;
			}
			/* Auto-disarm before the callback so it may restart. */
			data->state = NXP_FREQME_STATE_CONFIGURED;
		}
	} else {
		if ((flags & (uint32_t)kFREQME_OverflowInterruptStatusFlag) != 0U) {
			evts |= CLOCK_MONITOR_EVT_FREQ_HIGH;
		}
		if ((flags & (uint32_t)kFREQME_UnderflowInterruptStatusFlag) != 0U) {
			evts |= CLOCK_MONITOR_EVT_FREQ_LOW;
		}
		/* An out-of-range result auto-clears CONTINUOUS_MODE_EN in
		 * hardware; re-arm so monitoring continues.
		 */
		if (evts != 0U) {
			FREQME_EnableContinuousMode(config->base, true);
			FREQME_StartMeasurementCycle(config->base);
		}
	}

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

static int nxp_freqme_init(const struct device *dev)
{
	const struct nxp_freqme_config *config = dev->config;
	struct nxp_freqme_data *data = dev->data;
	int ret;

	data->state = NXP_FREQME_STATE_IDLE;
	data->cur_ref_idx = nxp_freqme_default_idx(config->ref_srcs,
						   config->ref_srcs_count);
	data->cur_tar_idx = nxp_freqme_default_idx(config->tar_srcs,
						   config->tar_srcs_count);

	if (!device_is_ready(config->gate_clk_dev)) {
		LOG_ERR("%s: gate clock device not ready", dev->name);
		return -ENODEV;
	}

	ret = clock_control_on(config->gate_clk_dev, config->gate_clk_subsys);
	if (ret != 0) {
		LOG_ERR("%s: failed to enable gate clock (%d)", dev->name, ret);
		return ret;
	}

	/* Route the default reference/target sources to the FREQME inputs. The
	 * driver does not enable these source clocks - the caller must have them
	 * running before configure()/start().
	 */
	nxp_freqme_route(&config->ref_srcs[data->cur_ref_idx]);
	nxp_freqme_route(&config->tar_srcs[data->cur_tar_idx]);

	config->irq_config_func(dev);
	return 0;
}

static DEVICE_API(clock_monitor, nxp_freqme_api) = {
	.configure  = nxp_freqme_configure,
	.start      = nxp_freqme_start,
	.stop       = nxp_freqme_stop,
	.get_rate   = nxp_freqme_get_rate,
	.set_source = nxp_freqme_set_source,
};

/* Frequency provider: a fixed clock-frequency takes precedence; otherwise a
 * clocks phandle is used; a source with neither is rejected by BUILD_ASSERT.
 */
#define NXP_FREQME_SRC_CLK_DEV(node)                                           \
	COND_CODE_1(DT_NODE_HAS_PROP(node, clock_frequency), (NULL),           \
		    (COND_CODE_1(DT_NODE_HAS_PROP(node, clocks),               \
				 (DEVICE_DT_GET(DT_CLOCKS_CTLR(node))),        \
				 (NULL))))

#define NXP_FREQME_SRC_CLK_SUBSYS(node)                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(node, clock_frequency),                   \
		    ((clock_control_subsys_t)0),                               \
		    (COND_CODE_1(DT_NODE_HAS_PROP(node, clocks),               \
				 ((clock_control_subsys_t)(uintptr_t)          \
					  DT_CLOCKS_CELL(node, name)),         \
				 ((clock_control_subsys_t)0))))

#define NXP_FREQME_SRC_ENTRY(node)                                             \
	{                                                                      \
		.inputmux_base = (INPUTMUX_Type *)DT_REG_ADDR(                 \
			DT_PHANDLE_BY_IDX(node, inputmux_connection, 0)),      \
		.channel = (uint16_t)DT_PHA_BY_IDX(node, inputmux_connection,  \
						   0, channel),                \
		.connection = (uint32_t)DT_PHA_BY_IDX(node, inputmux_connection, \
						      0, connection),          \
		.clk_dev = NXP_FREQME_SRC_CLK_DEV(node),                       \
		.clk_subsys = NXP_FREQME_SRC_CLK_SUBSYS(node),                 \
		.fixed_hz = (uint32_t)DT_PROP_OR(node, clock_frequency, 0),    \
		.has_fixed_hz = DT_NODE_HAS_PROP(node, clock_frequency),       \
		.is_default = DT_PROP(node, default_source),          \
	}

/* Per-source compile-time checks: a usable frequency provider, and a nonzero
 * fixed frequency when one is given.
 */
#define NXP_FREQME_SRC_ASSERT(node)                                            \
	BUILD_ASSERT(DT_NODE_HAS_PROP(node, clock_frequency) ||                \
			     DT_NODE_HAS_PROP(node, clocks),                   \
		     "nxp,freqme source needs clocks or clock-frequency");     \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node, clock_frequency) ||               \
			     DT_PROP_OR(node, clock_frequency, 0) != 0,        \
		     "nxp,freqme clock-frequency must be nonzero");

/* Target sources may omit a frequency provider (the target rate is measured,
 * not predicted); only reject a given fixed frequency that is zero.
 */
#define NXP_FREQME_TAR_SRC_ASSERT(node) \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node, clock_frequency) ||               \
			     DT_PROP_OR(node, clock_frequency, 0) != 0,        \
		     "nxp,freqme clock-frequency must be nonzero");

/* Count of default-source children in a container (summed in a BUILD_ASSERT). */
#define NXP_FREQME_DEFAULT_INC(node) + DT_PROP(node, default_source)

#define NXP_FREQME_REF_NODE(inst) DT_INST_CHILD(inst, reference_sources)
#define NXP_FREQME_TAR_NODE(inst) DT_INST_CHILD(inst, target_sources)

#define NXP_FREQME_DEVICE_INIT(inst)                                           \
	BUILD_ASSERT(DT_NODE_EXISTS(NXP_FREQME_REF_NODE(inst)),                \
		     "nxp,freqme: missing reference-sources node");            \
	BUILD_ASSERT(DT_NODE_EXISTS(NXP_FREQME_TAR_NODE(inst)),                \
		     "nxp,freqme: missing target-sources node");               \
	BUILD_ASSERT(DT_CHILD_NUM(NXP_FREQME_REF_NODE(inst)) >= 1,             \
		     "nxp,freqme: reference-sources needs >= 1 source");       \
	BUILD_ASSERT(DT_CHILD_NUM(NXP_FREQME_TAR_NODE(inst)) >= 1,             \
		     "nxp,freqme: target-sources needs >= 1 source");          \
	BUILD_ASSERT((0 DT_FOREACH_CHILD(NXP_FREQME_REF_NODE(inst),            \
					 NXP_FREQME_DEFAULT_INC)) == 1,        \
		     "nxp,freqme: reference-sources needs exactly one default-source"); \
	BUILD_ASSERT((0 DT_FOREACH_CHILD(NXP_FREQME_TAR_NODE(inst),            \
					 NXP_FREQME_DEFAULT_INC)) == 1,        \
		     "nxp,freqme: target-sources needs exactly one default-source"); \
	DT_FOREACH_CHILD(NXP_FREQME_REF_NODE(inst), NXP_FREQME_SRC_ASSERT)     \
	DT_FOREACH_CHILD(NXP_FREQME_TAR_NODE(inst), NXP_FREQME_TAR_SRC_ASSERT)     \
	static void nxp_freqme_irq_cfg_##inst(const struct device *dev)        \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),   \
			    nxp_freqme_isr, DEVICE_DT_INST_GET(inst), 0);      \
		irq_enable(DT_INST_IRQN(inst));                                \
	}                                                                      \
	static const struct nxp_freqme_source nxp_freqme_ref_srcs_##inst[] = { \
		DT_FOREACH_CHILD_SEP(NXP_FREQME_REF_NODE(inst),                \
				     NXP_FREQME_SRC_ENTRY, (,))                \
	};                                                                     \
	static const struct nxp_freqme_source nxp_freqme_tar_srcs_##inst[] = { \
		DT_FOREACH_CHILD_SEP(NXP_FREQME_TAR_NODE(inst),                \
				     NXP_FREQME_SRC_ENTRY, (,))                \
	};                                                                     \
	static struct nxp_freqme_data nxp_freqme_data_##inst;                  \
	static const struct nxp_freqme_config nxp_freqme_cfg_##inst = {        \
		.base = (FREQME_Type *)DT_INST_REG_ADDR(inst),                 \
		.gate_clk_dev =                                                \
			DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)), \
		.gate_clk_subsys = (clock_control_subsys_t)(uintptr_t)         \
			DT_INST_CLOCKS_CELL(inst, name),          \
		.ref_srcs = nxp_freqme_ref_srcs_##inst,                        \
		.ref_srcs_count =                                              \
			(uint8_t)ARRAY_SIZE(nxp_freqme_ref_srcs_##inst),       \
		.tar_srcs = nxp_freqme_tar_srcs_##inst,                        \
		.tar_srcs_count =                                              \
			(uint8_t)ARRAY_SIZE(nxp_freqme_tar_srcs_##inst),       \
		.irq_config_func = nxp_freqme_irq_cfg_##inst,                  \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_freqme_init, NULL,                     \
			      &nxp_freqme_data_##inst, &nxp_freqme_cfg_##inst, \
			      POST_KERNEL, CONFIG_CLOCK_MONITOR_INIT_PRIORITY, \
			      &nxp_freqme_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_FREQME_DEVICE_INIT)
