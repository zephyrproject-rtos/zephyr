/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor back-end for the frequency-measurement feature of the
 * NXP ANACTRL (Analog Control) module.
 *
 * The reference and target clocks are configured by devicetree: the node's
 * "reference-source" and "target-source" children each pair a MUX route
 * with a frequency provider (a fixed constant or a clock_control subsystem).
 *
 * This driver does NOT enable the reference/target source clocks: both must be
 * running before configure()/start(). Where the SoC gates a source into the
 * measurement block (SYSCON CLOCK_CTRL on MCXW2xx), that gate is the
 * application's responsibility too.
 *
 * Power management
 * ----------------
 * The clock to the analog controller module is enabled during boot time by
 * the bootloader and must always remain enabled(MCXW23, RM 11.2).
 * SUSPEND clears FREQ_ME_CTRL, cancels the deferred poll and releases the
 * power lock; a MEASURE cycle interrupted by SUSPEND is not restarted on
 * RESUME, the caller must call clock_monitor_start() again.
 *
 * While a cycle is in flight the driver holds the device power lock derived
 * from the node's "zephyr,disabling-power-states"; it is taken in start() and
 * released on completion, on timeout and on stop().
 *
 * With CONFIG_PM_DEVICE_RUNTIME the consumer holds the runtime reference:
 * pm_device_runtime_get() before configure(), kept until the monitor is no
 * longer needed. configure() and start() return -EBUSY while the device is not
 * PM-active; get_rate() always answers from RAM.
 *
 * Only SUSPEND and RESUME are implemented; TURN_ON and TURN_OFF return
 * -ENOTSUP. The first bring-up (gate clock + INPUTMUX routes) is therefore done
 * by the RESUME that pm_device_driver_init() issues from init(), and RESUME
 * repeats it after a low-power state.
 *
 * The INPUTMUX routes are outside the ANACTRL block, do not survive a system
 * standby and reset to the 12 MHz FRO (RM tables 523/524); start() and RESUME
 * re-assert them.
 */

#define DT_DRV_COMPAT nxp_anactrl_freqme

#include <zephyr/device.h>
#include <zephyr/devicetree/mux.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/policy.h>
#include <zephyr/sys/util.h>

#include <fsl_anactrl.h>

LOG_MODULE_REGISTER(clock_monitor_nxp_anactrl, CONFIG_CLOCK_MONITOR_LOG_LEVEL);

/* SCALE is a 5-bit field programming the reference counter to (2^SCALE - 1)
 * reference-clock edges; the HAL requires 2..31.
 */
#define ANACTRL_SCALE_MIN 2U
#define ANACTRL_SCALE_MAX 31U

/* Extra quarter-window polls allowed before a still-set PROG bit is reported
 * as CLOCK_LOST: worst-case wait is 1x + ANACTRL_POLL_RETRIES * 0.25x the
 * nominal window.
 */
#define ANACTRL_POLL_RETRIES 16U

/*
 * An ANACTRL freq-measure clock source: a MUX route paired with a frequency
 * provider. The provider is a fixed constant (has_fixed_hz) or a
 * clock_control subsystem (clk_dev / clk_subsys); a fixed provider takes
 * precedence when both are declared.
 */
struct nxp_anactrl_source {
	const struct device    *mux_dev;
	const struct mux_state *mux_state;
	const struct device    *clk_dev;
	clock_control_subsys_t  clk_subsys;
	uint32_t                fixed_hz;
	bool                    has_fixed_hz;
};

struct nxp_anactrl_config {
	ANACTRL_Type *base;
	/* Peripheral gate clock, enabled via clock_control_on(). */
	const struct device    *gate_clk_dev;
	clock_control_subsys_t  gate_clk_subsys;
	const struct nxp_anactrl_source *ref_src;
	const struct nxp_anactrl_source *tar_src;
#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	/* The node declares "zephyr,disabling-power-states". */
	bool pm_device_constraints;
#endif
};

enum nxp_anactrl_state {
	NXP_ANACTRL_STATE_IDLE = 0,
	NXP_ANACTRL_STATE_CONFIGURED,
	NXP_ANACTRL_STATE_RUNNING,
};

struct nxp_anactrl_data {
	/* Protects the state machine, cfg and result fields against the poll
	 * work handler and against concurrent API calls.
	 */
	struct k_spinlock lock;
	enum nxp_anactrl_state state;
	struct clock_monitor_config cfg;
	uint32_t ref_hz;      /* cached from the reference source at configure */
	uint8_t  scale;       /* cached SCALE for the poll formula / re-arm */
	uint32_t meas_us;     /* nominal measurement window in microseconds */
	uint32_t retries_left;
	/* Most recent completed MEASURE result; retained across reads. */
	uint32_t last_rate_hz;
	bool has_rate;
	/* CLOCK_LOST latched since configure(). */
	bool clock_lost;
	/* Register access is permitted: PM-active and brought up. */
	bool hw_active;
#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	bool power_locked;
#endif
	struct k_work_delayable poll_work;
	const struct device *dev;
};

static void nxp_anactrl_power_lock_get(const struct device *dev)
{
#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;

	if (config->pm_device_constraints && !data->power_locked) {
		pm_policy_device_power_lock_get(dev);
		data->power_locked = true;
	}
#else
	ARG_UNUSED(dev);
#endif
}

static void nxp_anactrl_power_lock_put(const struct device *dev)
{
#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
	struct nxp_anactrl_data *data = dev->data;

	if (data->power_locked) {
		pm_policy_device_power_lock_put(dev);
		data->power_locked = false;
	}
#else
	ARG_UNUSED(dev);
#endif
}

/* True when the device is PM-active; always true without CONFIG_PM_DEVICE. */
static bool nxp_anactrl_is_active(const struct device *dev)
{
	enum pm_device_state state;

	(void)pm_device_state_get(dev, &state);

	return state == PM_DEVICE_STATE_ACTIVE;
}

/*
 * Convert a measurement window (ns) into the SCALE exponent. The ANACTRL window
 * is (2^SCALE - 1) reference-clock periods. Returns -ERANGE when the window
 * maps to zero cycles or cannot be represented.
 */
static int anactrl_window_to_scale(uint32_t window_ns, uint32_t ref_hz, uint8_t *out)
{
	uint64_t cycles = (uint64_t)window_ns * (uint64_t)ref_hz / (uint64_t)NSEC_PER_SEC;
	uint64_t v = cycles;
	uint32_t s = 0U;

	if (cycles == 0U) {
		return -ERANGE;
	}

	while ((v >> 1) != 0U) {
		v >>= 1;
		s++;
	}

	/* Round to the nearer power of two. */
	if (s < ANACTRL_SCALE_MAX &&
	    (cycles - (1ULL << s)) > ((1ULL << (s + 1U)) - cycles)) {
		s++;
	}

	if (s > ANACTRL_SCALE_MAX || s < ANACTRL_SCALE_MIN) {
		return -ERANGE;
	}

	*out = (uint8_t)s;
	return 0;
}

/* Nominal measurement window in microseconds for a SCALE/ref rate, floored at
 * 1 us so the poll always makes progress.
 */
static uint32_t anactrl_meas_us(uint8_t scale, uint32_t ref_hz)
{
	uint64_t ns = ((uint64_t)((1ULL << scale) - 1ULL) * (uint64_t)NSEC_PER_SEC) / ref_hz;
	uint64_t us = ns / NSEC_PER_USEC;

	/* Poll and retry arithmetic are 32-bit: saturate, do not wrap. */
	if (us > UINT32_MAX) {
		LOG_WRN("poll interval for SCALE=%u, ref_hz=%u exceeds 32 bits; "
			"saturating at %u us", scale, ref_hz, (uint32_t)UINT32_MAX);
		return UINT32_MAX;
	}

	return (us == 0U) ? 1U : (uint32_t)us;
}

/* Apply a source's mux-state so its clock reaches the ANACTRL freq-measure
 * input.
 */
static int nxp_anactrl_route(const struct nxp_anactrl_source *src)
{
	if (!device_is_ready(src->mux_dev)) {
		LOG_ERR("mux device not ready");
		return -ENODEV;
	}
	return mux_state_apply(src->mux_dev, src->mux_state);
}

/*
 * Re-apply the reference and target INPUTMUX routes. The routes live outside the
 * ANACTRL block and a system standby resets them to the 12 MHz FRO (RM tables
 * 523/524).
 */
static int nxp_anactrl_apply_routes(const struct device *dev)
{
	const struct nxp_anactrl_config *config = dev->config;
	int ret;

	ret = nxp_anactrl_route(config->ref_src);
	if (ret != 0) {
		LOG_ERR("%s: failed to route reference clock (%d)", dev->name, ret);
		return ret;
	}

	ret = nxp_anactrl_route(config->tar_src);
	if (ret != 0) {
		LOG_ERR("%s: failed to route target clock (%d)", dev->name, ret);
		return ret;
	}

	return 0;
}

/*
 * Open the peripheral gate and re-apply both INPUTMUX routes. Called from init()
 * and from RESUME. Must not be called with data->lock held: the mux and
 * clock_control calls are not spinlock-safe.
 */
static int nxp_anactrl_hw_bringup(const struct device *dev)
{
	const struct nxp_anactrl_config *config = dev->config;
	int ret;

	/* The clock gate must be open first: a register write issued while it is
	 * closed is silently dropped.
	 */
	ret = clock_control_on(config->gate_clk_dev, config->gate_clk_subsys);
	if (ret != 0) {
		LOG_ERR("%s: failed to enable gate clock (%d)", dev->name, ret);
		return ret;
	}

	return nxp_anactrl_apply_routes(dev);
}

/*
 * Terminate a running cycle. The caller holds data->lock and has established
 * that register access is permitted. Clearing FREQ_ME_CTRL drops PROG without
 * loading CAPVAL; the clock gate stays open.
 */
static void nxp_anactrl_hw_stop(const struct device *dev)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;

	if (data->state == NXP_ANACTRL_STATE_RUNNING) {
		config->base->FREQ_ME_CTRL = 0U;
		data->state = NXP_ANACTRL_STATE_CONFIGURED;
	}
}

/*
 * Resolve a source's frequency in Hz. If clock frequency is provided in device
 * tree, use this frequency; otherwise get it from the clock_control API.
 * Returns -EIO when the rate is unavailable or zero.
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

static int nxp_anactrl_configure(const struct device *dev,
				 const struct clock_monitor_config *cfg)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t ref_hz = 0U;
	uint32_t meas_us;
	uint8_t scale;
	int ret;

	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE) {
		return -ENOTSUP;
	}
	if (cfg->measure.window_ns == 0U) {
		return -EINVAL;
	}

	/* The reference rate query below needs the device PM-active: a suspended
	 * device can legitimately report 0 Hz, which would surface as -EIO instead
	 * of the real reason.
	 */
	if (!nxp_anactrl_is_active(dev)) {
		LOG_ERR("%s: device is not active", dev->name);
		return -EBUSY;
	}

	ret = nxp_anactrl_src_rate(config->ref_src, &ref_hz);
	if (ret != 0) {
		return ret;
	}

	ret = anactrl_window_to_scale(cfg->measure.window_ns, ref_hz, &scale);
	if (ret != 0) {
		LOG_ERR("window_ns=%u out of range for ref_hz=%u",
			cfg->measure.window_ns, ref_hz);
		return -EINVAL;
	}

	meas_us = anactrl_meas_us(scale, ref_hz);

	key = k_spin_lock(&data->lock);

	/* Nothing was written to hardware above, so there is nothing to undo. */
	if (!data->hw_active) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("%s: hardware is not available", dev->name);
		return -EBUSY;
	}
	if (data->state == NXP_ANACTRL_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	data->cfg = *cfg;
	data->ref_hz = ref_hz;
	data->scale = scale;
	data->meas_us = meas_us;
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
	k_timeout_t delay;
	k_spinlock_key_t key;
	int ret;

	if (!nxp_anactrl_is_active(dev)) {
		LOG_ERR("%s: device is not active", dev->name);
		return -EBUSY;
	}

	/* A standby since the last cycle may have reset the routes. */
	ret = nxp_anactrl_apply_routes(dev);
	if (ret != 0) {
		return ret;
	}

	key = k_spin_lock(&data->lock);
	if (!data->hw_active) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("%s: hardware is not available", dev->name);
		return -EBUSY;
	}
	if (data->state != NXP_ANACTRL_STATE_CONFIGURED) {
		enum nxp_anactrl_state st = data->state;

		k_spin_unlock(&data->lock, key);
		/* IDLE is "not configured"; RUNNING is busy. */
		return (st == NXP_ANACTRL_STATE_IDLE) ? -EINVAL : -EBUSY;
	}

	data->state = NXP_ANACTRL_STATE_RUNNING;
	data->retries_left = ANACTRL_POLL_RETRIES;

	/* Held until completion, timeout or stop(). */
	nxp_anactrl_power_lock_get(dev);

	/* Arm the one-shot measurement and capture this cycle's poll delay. */
	config->base->FREQ_ME_CTRL =
		ANACTRL_FREQ_ME_CTRL_PROG_MASK | ANACTRL_FREQ_ME_CTRL_CAPVAL_SCALE(data->scale);
	delay = K_USEC(data->meas_us);
	k_spin_unlock(&data->lock, key);

	/* A SUSPEND before this call leaves a queued poll; it exits on hw_active. */
	(void)k_work_reschedule(&data->poll_work, delay);
	return 0;
}

static int nxp_anactrl_stop(const struct device *dev)
{
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;

	/* An already-running handler exits on the hw_active/state check. */
	(void)k_work_cancel_delayable(&data->poll_work);

	key = k_spin_lock(&data->lock);
	nxp_anactrl_power_lock_put(dev);
	if (data->state == NXP_ANACTRL_STATE_RUNNING && data->hw_active &&
	    nxp_anactrl_is_active(dev)) {
		nxp_anactrl_hw_stop(dev);
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

	/* Nothing may touch the block unless the device is active and armed. */
	if (!data->hw_active || data->state != NXP_ANACTRL_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	ref_hz = data->ref_hz;
	/* While PROG is set the low 31 bits read 0, once hardware clears
	 * PROG, CAPVAL holds the capture value.
	 */
	ctrl = config->base->FREQ_ME_CTRL;

	if ((ctrl & ANACTRL_FREQ_ME_CTRL_PROG_MASK) != 0U) {
		/* PROG still set: the window has not elapsed, re-arm within budget. */
		if (data->retries_left > 0U) {
			k_timeout_t delay = K_USEC(MAX(1U, data->meas_us / 4U));

			data->retries_left--;
			k_spin_unlock(&data->lock, key);
			(void)k_work_reschedule(&data->poll_work, delay);
			return;
		}
		data->clock_lost = true;
		/* Timeout: terminate the cycle so the block is never left armed. */
		config->base->FREQ_ME_CTRL = 0U;
		timed_out = true;
		evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
	} else {
		/* PROG clear: the low 31 bits are CAPVAL, the target edges counted. */
		capval = ctrl & ANACTRL_FREQ_ME_CTRL_CAPVAL_SCALE_MASK;

		if (capval == 0U) {
			data->clock_lost = true;
			evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
		} else {
			/* RM: Ftarget = CAPVAL * Fref / (2^SCALE - 1). */
			uint64_t denom = (1ULL << data->scale) - 1ULL;

			rate = (uint32_t)(((uint64_t)capval * (uint64_t)data->ref_hz) / denom);

			/* A nonzero CAPVAL can still floor to 0 Hz: report CLOCK_LOST
			 * rather than a valid 0 Hz result.
			 */
			if (rate == 0U) {
				data->clock_lost = true;
				evts |= CLOCK_MONITOR_EVT_CLOCK_LOST;
			} else {
				data->last_rate_hz = rate;
				data->has_rate = true;
				/* A good measurement clears a latched CLOCK_LOST. */
				data->clock_lost = false;
				evts |= CLOCK_MONITOR_EVT_MEASURE_DONE;
			}
		}
	}

	/* Auto-disarm before the callback so it may restart from the callback. */
	data->state = NXP_ANACTRL_STATE_CONFIGURED;
	nxp_anactrl_power_lock_put(dev);
	cb = data->cfg.callback;
	user_data = data->cfg.user_data;
	k_spin_unlock(&data->lock, key);

	/* Outside the lock: immediate-mode logging must not run with irqs off. */
	if (timed_out) {
		LOG_WRN("%s: measurement timed out (FREQ_ME_CTRL=0x%08x, ref_hz=%u) - "
			"reference clock stopped, or its measure gate is off?",
			dev->name, ctrl, ref_hz);
	} else if ((evts & CLOCK_MONITOR_EVT_CLOCK_LOST) != 0U) {
		LOG_WRN("%s: target clock unusable (capval=%u, ref_hz=%u) - stopped, "
			"not routed, too slow for this window, or its measure gate is off?",
			dev->name, capval, ref_hz);
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

	/* Answered from dev->data only; valid while suspended. */
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

static int nxp_anactrl_pm_action(const struct device *dev, enum pm_device_action action)
{
	struct nxp_anactrl_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* The gate and the routes do not survive every low-power state. */
		ret = nxp_anactrl_hw_bringup(dev);
		if (ret != 0) {
			return ret;
		}

		key = k_spin_lock(&data->lock);
		data->hw_active = true;
		k_spin_unlock(&data->lock, key);

		/* An aborted cycle is not restarted; the caller re-arms. */
		return 0;

	case PM_DEVICE_ACTION_SUSPEND:
		key = k_spin_lock(&data->lock);
		if (data->hw_active) {
			nxp_anactrl_hw_stop(dev);
			data->hw_active = false;
		}
		nxp_anactrl_power_lock_put(dev);
		k_spin_unlock(&data->lock, key);

		/* A running handler exits on the hw_active check. */
		(void)k_work_cancel_delayable(&data->poll_work);

		/* The clock gate is intentionally left open. */
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int nxp_anactrl_init(const struct device *dev)
{
	const struct nxp_anactrl_config *config = dev->config;
	struct nxp_anactrl_data *data = dev->data;

	data->dev = dev;
	data->state = NXP_ANACTRL_STATE_IDLE;
	k_work_init_delayable(&data->poll_work, nxp_anactrl_poll_work);

	if (!device_is_ready(config->gate_clk_dev)) {
		LOG_ERR("%s: gate clock device not ready", dev->name);
		return -ENODEV;
	}

	/* Bring-up is left to PM_DEVICE_ACTION_RESUME: pm_device_driver_init()
	 * runs it as soon as the device is meant to be active. Doing it here as
	 * well would open the gate and write the routes twice, and would leave the
	 * block powered while the PM core records the device as suspended (the
	 * configuration that enables device runtime PM at init time).
	 */
	return pm_device_driver_init(dev, nxp_anactrl_pm_action);
}

static DEVICE_API(clock_monitor, nxp_anactrl_api) = {
	.configure  = nxp_anactrl_configure,
	.start      = nxp_anactrl_start,
	.stop       = nxp_anactrl_stop,
	.get_rate   = nxp_anactrl_get_rate,
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
	}

/* Reference sources need a frequency provider; a given clock-frequency must be
 * nonzero.
 */
#define NXP_ANACTRL_SRC_ASSERT(node)                                           \
	BUILD_ASSERT(DT_NODE_HAS_PROP(node, clock_frequency) ||                \
			     DT_NODE_HAS_PROP(node, clocks),                   \
		     "nxp,anactrl-freqme source needs clocks or clock-frequency"); \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node, clock_frequency) ||               \
			     DT_PROP_OR(node, clock_frequency, 0) != 0,        \
		     "nxp,anactrl-freqme clock-frequency must be nonzero");

/* Target sources may omit a frequency provider; a given clock-frequency must be
 * nonzero.
 */
#define NXP_ANACTRL_TAR_SRC_ASSERT(node)                                       \
	BUILD_ASSERT(!DT_NODE_HAS_PROP(node, clock_frequency) ||               \
			     DT_PROP_OR(node, clock_frequency, 0) != 0,        \
		     "nxp,anactrl-freqme clock-frequency must be nonzero");

#define NXP_ANACTRL_REF_NODE(inst) DT_INST_CHILD(inst, reference_source)
#define NXP_ANACTRL_TAR_NODE(inst) DT_INST_CHILD(inst, target_source)

#if defined(CONFIG_PM_POLICY_DEVICE_CONSTRAINTS)
#define NXP_ANACTRL_PM_DEVICE_CONSTRAINTS_INIT(inst)                           \
	.pm_device_constraints =                                               \
		DT_INST_NODE_HAS_PROP(inst, zephyr_disabling_power_states),
#else
#define NXP_ANACTRL_PM_DEVICE_CONSTRAINTS_INIT(inst)
#endif

#define NXP_ANACTRL_DEVICE_INIT(inst)                                          \
	BUILD_ASSERT(DT_NODE_EXISTS(NXP_ANACTRL_REF_NODE(inst)),               \
		     "nxp,anactrl-freqme: missing reference-source node");     \
	BUILD_ASSERT(DT_NODE_EXISTS(NXP_ANACTRL_TAR_NODE(inst)),               \
		     "nxp,anactrl-freqme: missing target-source node");        \
	NXP_ANACTRL_SRC_ASSERT(NXP_ANACTRL_REF_NODE(inst))                     \
	NXP_ANACTRL_TAR_SRC_ASSERT(NXP_ANACTRL_TAR_NODE(inst))                 \
	NXP_ANACTRL_SRC_MUX_DEFINE(NXP_ANACTRL_REF_NODE(inst))                 \
	NXP_ANACTRL_SRC_MUX_DEFINE(NXP_ANACTRL_TAR_NODE(inst))                 \
	static const struct nxp_anactrl_source nxp_anactrl_ref_src_##inst =    \
		NXP_ANACTRL_SRC_ENTRY(NXP_ANACTRL_REF_NODE(inst));             \
	static const struct nxp_anactrl_source nxp_anactrl_tar_src_##inst =    \
		NXP_ANACTRL_SRC_ENTRY(NXP_ANACTRL_TAR_NODE(inst));             \
	static struct nxp_anactrl_data nxp_anactrl_data_##inst;                \
	static const struct nxp_anactrl_config nxp_anactrl_cfg_##inst = {      \
		.base = (ANACTRL_Type *)DT_INST_REG_ADDR(inst),                \
		.gate_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),      \
		.gate_clk_subsys = (clock_control_subsys_t)(uintptr_t)         \
			DT_INST_CLOCKS_CELL(inst, name),                       \
		.ref_src = &nxp_anactrl_ref_src_##inst,                        \
		.tar_src = &nxp_anactrl_tar_src_##inst,                        \
		NXP_ANACTRL_PM_DEVICE_CONSTRAINTS_INIT(inst)                   \
	};                                                                     \
	PM_DEVICE_DT_INST_DEFINE(inst, nxp_anactrl_pm_action);                 \
	DEVICE_DT_INST_DEFINE(inst, nxp_anactrl_init,                          \
			      PM_DEVICE_DT_INST_GET(inst),                     \
			      &nxp_anactrl_data_##inst, &nxp_anactrl_cfg_##inst, \
			      POST_KERNEL, CONFIG_CLOCK_MONITOR_INIT_PRIORITY, \
			      &nxp_anactrl_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_ANACTRL_DEVICE_INIT)
