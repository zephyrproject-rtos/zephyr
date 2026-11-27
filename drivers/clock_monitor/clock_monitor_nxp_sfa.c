/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr clock_monitor backend for the NXP Signal Frequency Analyser (SFA),
 * built on the MCUX SDK fsl_sfa HAL. Both abstraction modes are supported:
 *
 *   - MEASURE: one measurement per start(); the completion interrupt reports
 *     MEASURE_DONE (or CLOCK_LOST) and get_rate() returns the result.
 *   - WINDOW:  SFA monitor mode measures until a result leaves the limits,
 *     then reports FREQ_HIGH / FREQ_LOW and re-arms. It runs until stop().
 */

#define DT_DRV_COMPAT nxp_sfa

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_monitor.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <fsl_sfa.h>

LOG_MODULE_REGISTER(clock_monitor_nxp_sfa, CONFIG_CLOCK_MONITOR_LOG_LEVEL);

/* Each WINDOW limit is widened by this many counts. The measured count is an
 * integer, so a clock on the tolerance edge reads one count higher or lower.
 */
#define NXP_SFA_WINDOW_MARGIN 2U
/* MODE 0 REF timeout, as a multiple of the expected measurement time. */
#define NXP_SFA_REF_TIMEOUT_FACTOR 4U
/* REF ticks added to the MODE 0 timeout for the domain synchronizers. */
#define NXP_SFA_SYNC_SLACK 16U
/* Poll limit for CUT_STOPPED to clear after the REF_CNT write. */
#define NXP_SFA_STOP_CLEAR_TRIES 64U
/* MEASURE guard = 2 * hardware timeout + this, to cover timer granularity
 * and ISR entry delay.
 */
#define NXP_SFA_GUARD_EXTRA_US 10000U

#define NXP_SFA_ALL_IRQS ((uint32_t)kSFA_InterruptEnable |                     \
			  (uint32_t)kSFA_FreqGreaterThanMaxInterruptEnable |   \
			  (uint32_t)kSFA_FreqLessThanMinInterruptEnable)
#define NXP_SFA_W1C_FLAGS ((uint32_t)kSFA_InterruptRequestFlag |               \
			   (uint32_t)kSFA_FreqGreaterThanMaxInterruptFlag |    \
			   (uint32_t)kSFA_FreqLessThanMinInterruptFlag)

struct nxp_sfa_config {
	SFA_Type *base;
	const struct device    *gate_clk_dev;
	clock_control_subsys_t  gate_clk_subsys;
	uint32_t ref_hz;
	/* Expected target rate; 0 if unknown. */
	uint32_t tar_hz;
	uint8_t ref_sel;
	uint8_t cut_sel;
	void (*irq_config_func)(void);
};

enum nxp_sfa_state {
	NXP_SFA_STATE_IDLE = 0,
	NXP_SFA_STATE_CONFIGURING,
	NXP_SFA_STATE_CONFIGURED,
	NXP_SFA_STATE_RUNNING,
};

enum nxp_sfa_rate_state {
	NXP_SFA_RATE_NONE = 0,
	NXP_SFA_RATE_VALID,
	NXP_SFA_RATE_LOST,
};

/* Register values for one configuration. */
struct nxp_sfa_hw {
	sfa_measurement_mode_t mode;
	uint32_t ref_target;
	uint32_t cut_target;
	/* WINDOW limits: REF_*_LIMIT_CNT in MODE 0, CUT_*_LIMIT_CNT in MODE 1. */
	uint32_t low;
	uint32_t high;
	uint32_t irq_mask;
	k_timeout_t guard;
};

struct nxp_sfa_data {
	/* Protects the fields below against the ISR, the timer and API calls. */
	struct k_spinlock lock;
	enum nxp_sfa_state state;
	enum clock_monitor_mode mode;
	clock_monitor_callback_t cb;
	void *user_data;
	uint32_t irq_mask;
	k_timeout_t guard_timeout;
	enum nxp_sfa_rate_state rate_state;
	uint32_t rate_hz;
	struct k_timer guard;
	const struct device *dev;
};

/* Fill @p hw from @p cfg. @p expected_hz is the rate the target should run
 * at, 0 if unknown. Returns -EINVAL if the values do not fit the registers.
 */
static int nxp_sfa_calc_hw(const struct clock_monitor_config *cfg, uint32_t ref_hz,
			  uint32_t expected_hz, struct nxp_sfa_hw *hw)
{
	bool window = (cfg->mode == CLOCK_MONITOR_MODE_WINDOW);
	uint32_t window_ns = window ? cfg->window.window_ns : cfg->measure.window_ns;
	uint32_t tol_ppm = window ? cfg->window.tolerance_ppm : 0U;
	/* REF ticks one measurement should take. */
	uint64_t ticks;
	uint64_t timeout;

	if (expected_hz != 0U && expected_hz < ref_hz) {
		/* MODE 0: mearsure slow clock, the result spans cut_target - 1 target periods. */
		uint64_t edges = DIV_ROUND_CLOSEST((uint64_t)window_ns * expected_hz,
						   NSEC_PER_SEC);

		edges = MAX(edges, 1U);
		ticks = edges * ref_hz / expected_hz;

		hw->mode = kSFA_FrequencyMeasurement0;
		hw->cut_target = (uint32_t)edges + 1U;

		if (window) {
			/* The REF count falls as the clock rises: a fast clock
			 * gives fewer ticks, so the tolerance divides. Both
			 * limits are widened outward by MARGIN.
			 */
			uint64_t lo = ticks * 1000000U / (1000000U + tol_ppm);
			uint64_t hi = ticks * 1000000U / (1000000U - tol_ppm) +
				      NXP_SFA_WINDOW_MARGIN;

			/* A count below this limit means the clock is too fast
			 * and FREQ_GT_MAX fires. A limit of 0 disables the
			 * check, because a count is never below 0.
			 */
			hw->low = (lo > NXP_SFA_WINDOW_MARGIN) ?
				  (uint32_t)(lo - NXP_SFA_WINDOW_MARGIN) : 0U;

			/* REF_TARGET gets SYNC_SLACK added below, so high plus
			 * slack must still fit in 32 bits.
			 */
			if (hi > UINT32_MAX - NXP_SFA_SYNC_SLACK) {
				return -EINVAL;
			}
			hw->high = (uint32_t)hi;
		}

		/* REF_TARGET is the timeout here. In WINDOW it must stay above
		 * the high limit, or a slow clock ends as a timeout. SYNC_SLACK
		 * covers the domain synchronizers.
		 */
		timeout = MAX(ticks * NXP_SFA_REF_TIMEOUT_FACTOR,
			      window ? (uint64_t)hw->high : 0U) + NXP_SFA_SYNC_SLACK;
		hw->ref_target = (uint32_t)MIN(timeout, UINT32_MAX);
	} else {
		/* MODE 1: measure fast clock, count target edges in a fixed REF window. */
		ticks = DIV_ROUND_CLOSEST((uint64_t)window_ns * ref_hz, NSEC_PER_SEC);
		if (ticks == 0U || ticks > UINT32_MAX) {
			return -EINVAL;
		}

		hw->mode = kSFA_FrequencyMeasurement1;
		hw->ref_target = (uint32_t)ticks;
		/* No CUT timeout flag exists: keep CUT_TARGET out of reach. */
		hw->cut_target = UINT32_MAX;

		if (window) {
			/* The count rises with the clock here, so the tolerance
			 * multiplies. The +1 is the register offset, not margin:
			 * CUT_CNT reads edges + 1 and the HAL subtracts 1 again.
			 */
			uint64_t edges = (uint64_t)expected_hz * ticks / ref_hz;
			uint64_t lo = edges * (1000000U - tol_ppm) / 1000000U + 1U;
			uint64_t hi = edges * (1000000U + tol_ppm) / 1000000U + 1U +
				      NXP_SFA_WINDOW_MARGIN;

			/* No SYNC_SLACK here: REF_TARGET is the measurement
			 * window, not a timeout.
			 */
			if (edges == 0U || hi >= UINT32_MAX) {
				return -EINVAL;
			}
			hw->low = (lo > NXP_SFA_WINDOW_MARGIN) ?
				  (uint32_t)(lo - NXP_SFA_WINDOW_MARGIN) : 0U;
			hw->high = (uint32_t)hi;
		}
		timeout = ticks;
	}

	if (window) {
		hw->irq_mask = (uint32_t)kSFA_FreqGreaterThanMaxInterruptEnable |
			       (uint32_t)kSFA_FreqLessThanMinInterruptEnable;
	} else {
		uint64_t us = timeout * USEC_PER_SEC / ref_hz;

		hw->irq_mask = (uint32_t)kSFA_InterruptEnable;
		hw->guard = K_USEC(MIN(2U * us + NXP_SFA_GUARD_EXTRA_US, UINT32_MAX));
	}

	return 0;
}

/* Stop any cycle and leave the block disabled with no IRQ enabled. */
static void nxp_sfa_hw_abort(SFA_Type *base)
{
	SFA_DisableInterrupts(base, NXP_SFA_ALL_IRQS);
	/* REF_CNT only clears the flags when no measurement is running. */
	SFA_AbortMeasureSequence(base);
	SFA_ClearStatusFlag(base, (uint32_t)kSFA_AllStatusFlags);
}

/* Start one cycle without waiting for MEAS_STARTED. */
static void nxp_sfa_hw_begin(SFA_Type *base, uint32_t irq_mask)
{
	base->CTRL |= SFA_CTRL_SFA_EN_MASK;

	/* SFA_IRQ and the limit flags are W1C. */
	SFA_ClearStatusFlag(base, NXP_SFA_W1C_FLAGS);

	/* CUT_STOPPED, REF_STOPPED and MEAS_STARTED only clear on a REF_CNT write. */
	base->REF_CNT = 0U;
	for (uint32_t i = 0U; i < NXP_SFA_STOP_CLEAR_TRIES; i++) {
		if ((SFA_GetStatusFlags(base) & (uint32_t)kSFA_CutStoppedFlag) == 0U) {
			break;
		}
	}

	/* Arm the IRQ before the write below starts the counters. */
	SFA_EnableInterrupts(base, irq_mask);

	/* Starts both counters. */
	base->CUT_CNT = 0U;
}

static void nxp_sfa_hw_apply(const struct nxp_sfa_config *config, bool window,
			     const struct nxp_sfa_hw *hw)
{
	SFA_Type *base = config->base;
	sfa_config_t sfa_cfg;

	/* SFA_EN must be low before any clock select change. */
	nxp_sfa_hw_abort(base);

	SFA_GetDefaultConfig(&sfa_cfg);
	sfa_cfg.mode = hw->mode;
	sfa_cfg.cutSelect = (sfa_cut_select_t)config->cut_sel;
	sfa_cfg.refSelect = (sfa_ref_select_t)config->ref_sel;
	sfa_cfg.refTarget = hw->ref_target;
	sfa_cfg.cutTarget = hw->cut_target;
	SFA_SetMeasureConfig(base, &sfa_cfg);

	if (!window) {
		return;
	}

	if (hw->mode == kSFA_FrequencyMeasurement0) {
		SFA_SetREFLowLimitClockCount(base, hw->low);
		SFA_SetREFHighLimitClockCount(base, hw->high);
	} else {
		SFA_SetCUTLowLimitClockCount(base, hw->low);
		SFA_SetCUTHighLimitClockCount(base, hw->high);
	}
	/* MONITOR_EN may be set with SFA_EN high. */
	base->CTRL |= SFA_CTRL_MONITOR_EN_MASK;
}

/* Convert a finished MEASURE cycle to Hz. Returns false if it is not valid. */
static bool nxp_sfa_result(SFA_Type *base, uint32_t status, uint32_t ref_hz,
			   uint32_t *hz)
{
	uint8_t mode = SFA_GetMode(base);
	uint32_t cut = SFA_GetCUTCounter(base);

	/* The HAL formula needs cut - 1 periods: cut < 2 is no data. */
	if (cut < 2U) {
		return false;
	}

	if (mode == (uint8_t)kSFA_FrequencyMeasurement0) {
		/* Reject a partial cycle: REF timeout, short CUT count, no REF span. */
		if (((status & (uint32_t)kSFA_ReferenceCounterTimeOutFlag) != 0U) ||
		    (cut < SFA_GetCUTTargetCount(base)) ||
		    (SFA_GetREFEndCount(base) <= SFA_GetREFStartCount(base))) {
			return false;
		}
	} else if (mode == (uint8_t)kSFA_FrequencyMeasurement1) {
		/* MODE 1 must end on REF_TARGET, not on CUT_TARGET. */
		if (((status & (uint32_t)kSFA_RefStoppedFlag) == 0U) ||
		    (cut >= SFA_GetCUTTargetCount(base))) {
			return false;
		}
	} else {
		return false;
	}

	*hz = SFA_CalculateFrequencyOrPeriod(base, ref_hz);
	return true;
}

static int nxp_sfa_configure(const struct device *dev,
			     const struct clock_monitor_config *cfg)
{
	const struct nxp_sfa_config *config = dev->config;
	struct nxp_sfa_data *data = dev->data;
	bool window = (cfg->mode == CLOCK_MONITOR_MODE_WINDOW);
	uint32_t expected_hz = config->tar_hz;
	struct nxp_sfa_hw hw = {0};
	enum nxp_sfa_state prev_state;
	k_spinlock_key_t key;
	int ret;

	if (cfg->mode != CLOCK_MONITOR_MODE_MEASURE && !window) {
		return -ENOTSUP;
	}
	if (window) {
		if (cfg->window.window_ns == 0U || cfg->window.tolerance_ppm >= 1000000U) {
			return -EINVAL;
		}
		if (cfg->window.expected_hz != 0U) {
			expected_hz = cfg->window.expected_hz;
		} else if (expected_hz == 0U) {
			/* No expected_hz and no devicetree target-frequency. */
			return -EINVAL;
		}
	} else if (cfg->measure.window_ns == 0U) {
		return -EINVAL;
	}

	/* Take CONFIGURING so that a concurrent configure() or start() gets
	 * -EBUSY.
	 */
	key = k_spin_lock(&data->lock);

	if (data->state == NXP_SFA_STATE_RUNNING ||
	    data->state == NXP_SFA_STATE_CONFIGURING) {
		k_spin_unlock(&data->lock, key);
		return -EBUSY;
	}

	prev_state = data->state;
	data->state = NXP_SFA_STATE_CONFIGURING;

	k_spin_unlock(&data->lock, key);

	ret = nxp_sfa_calc_hw(cfg, config->ref_hz, expected_hz, &hw);
	if (ret != 0) {
		LOG_ERR("%s: window cannot be represented (ref %u Hz, target %u Hz)",
			dev->name, config->ref_hz, expected_hz);

		/* The hardware is unchanged, so the previous config applies. */
		key = k_spin_lock(&data->lock);
		data->state = prev_state;
		k_spin_unlock(&data->lock, key);
		return ret;
	}

	key = k_spin_lock(&data->lock);

	nxp_sfa_hw_apply(config, window, &hw);

	data->mode = cfg->mode;
	data->cb = cfg->callback;
	data->user_data = cfg->user_data;
	data->irq_mask = hw.irq_mask;
	data->guard_timeout = hw.guard;
	data->rate_state = NXP_SFA_RATE_NONE;
	data->state = NXP_SFA_STATE_CONFIGURED;

	k_spin_unlock(&data->lock, key);

	LOG_DBG("%s: MODE %u, ref_target %u, cut_target %u, limits %u..%u",
		dev->name, (uint32_t)hw.mode, hw.ref_target, hw.cut_target,
		hw.low, hw.high);
	return 0;
}

static int nxp_sfa_start(const struct device *dev)
{
	const struct nxp_sfa_config *config = dev->config;
	struct nxp_sfa_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	if (data->state != NXP_SFA_STATE_CONFIGURED) {
		enum nxp_sfa_state st = data->state;

		k_spin_unlock(&data->lock, key);
		return (st == NXP_SFA_STATE_IDLE) ? -EINVAL : -EBUSY;
	}

	/* State and hardware change together under the lock. */
	data->state = NXP_SFA_STATE_RUNNING;
	nxp_sfa_hw_begin(config->base, data->irq_mask);
	if (data->mode == CLOCK_MONITOR_MODE_MEASURE) {
		k_timer_start(&data->guard, data->guard_timeout, K_NO_WAIT);
	}

	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_sfa_stop(const struct device *dev)
{
	const struct nxp_sfa_config *config = dev->config;
	struct nxp_sfa_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	if (data->state == NXP_SFA_STATE_RUNNING) {
		k_timer_stop(&data->guard);
		nxp_sfa_hw_abort(config->base);
		data->state = NXP_SFA_STATE_CONFIGURED;
	}
	k_spin_unlock(&data->lock, key);
	return 0;
}

static int nxp_sfa_get_rate(const struct device *dev, uint32_t *rate_hz)
{
	struct nxp_sfa_data *data = dev->data;
	k_spinlock_key_t key;
	int ret;

	key = k_spin_lock(&data->lock);
	if (data->rate_state == NXP_SFA_RATE_VALID) {
		*rate_hz = data->rate_hz;
		ret = 0;
	} else {
		ret = (data->rate_state == NXP_SFA_RATE_LOST) ? -EIO : -EAGAIN;
	}
	k_spin_unlock(&data->lock, key);
	return ret;
}

static void nxp_sfa_report(const struct device *dev, clock_monitor_callback_t cb,
			   void *user_data, uint32_t events, uint32_t hz)
{
	if (cb != NULL) {
		struct clock_monitor_event_data evt = {
			.events = events,
			.measured_hz = hz,
		};

		cb(dev, &evt, user_data);
	}
}

static void nxp_sfa_isr(const struct device *dev)
{
	const struct nxp_sfa_config *config = dev->config;
	struct nxp_sfa_data *data = dev->data;
	SFA_Type *base = config->base;
	uint32_t flags = SFA_GetStatusFlags(base);
	uint32_t evts = 0U;
	uint32_t hz = 0U;
	clock_monitor_callback_t cb;
	void *user_data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);

	/* Flag left by a cycle that stop() or the guard already ended. */
	if (data->state != NXP_SFA_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (data->mode == CLOCK_MONITOR_MODE_MEASURE) {
		if ((flags & (uint32_t)kSFA_InterruptRequestFlag) == 0U) {
			k_spin_unlock(&data->lock, key);
			return;
		}
		k_timer_stop(&data->guard);
		SFA_DisableInterrupts(base, (uint32_t)kSFA_InterruptEnable);
		SFA_ClearStatusFlag(base, (uint32_t)kSFA_InterruptRequestFlag);

		if (nxp_sfa_result(base, flags, config->ref_hz, &hz)) {
			data->rate_hz = hz;
			data->rate_state = NXP_SFA_RATE_VALID;
			evts = CLOCK_MONITOR_EVT_MEASURE_DONE;
		} else {
			data->rate_state = NXP_SFA_RATE_LOST;
			evts = CLOCK_MONITOR_EVT_CLOCK_LOST;
		}
		/* Set CONFIGURED before the callback, which may call start(). */
		data->state = NXP_SFA_STATE_CONFIGURED;
	} else {
		if ((flags & (uint32_t)kSFA_FreqGreaterThanMaxInterruptFlag) != 0U) {
			evts |= CLOCK_MONITOR_EVT_FREQ_HIGH;
		}
		if ((flags & (uint32_t)kSFA_FreqLessThanMinInterruptFlag) != 0U) {
			evts |= CLOCK_MONITOR_EVT_FREQ_LOW;
		}
		if (evts == 0U) {
			k_spin_unlock(&data->lock, key);
			return;
		}
		/* The hardware stops on the first out-of-range result: re-arm.
		 * No SFA_ClearStatusFlag() here: nxp_sfa_hw_begin() clears the
		 * W1C flags (SFA_IRQ, FREQ_GT_MAX, FREQ_LT_MIN) itself before it
		 * restarts the counters, and it also clears the stop flags that
		 * only a REF_CNT write can clear.
		 */
		nxp_sfa_hw_begin(base, data->irq_mask);
	}

	cb = data->cb;
	user_data = data->user_data;
	k_spin_unlock(&data->lock, key);

	nxp_sfa_report(dev, cb, user_data, evts, hz);
}

static void nxp_sfa_guard_expired(struct k_timer *timer)
{
	struct nxp_sfa_data *data = CONTAINER_OF(timer, struct nxp_sfa_data, guard);
	const struct device *dev = data->dev;
	const struct nxp_sfa_config *config = dev->config;
	clock_monitor_callback_t cb;
	void *user_data;
	k_spinlock_key_t key;
	uint32_t flags;

	key = k_spin_lock(&data->lock);

	/* The completion ISR already ended the cycle. */
	if (data->state != NXP_SFA_STATE_RUNNING) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	flags = SFA_GetStatusFlags(config->base);
	nxp_sfa_hw_abort(config->base);
	data->rate_state = NXP_SFA_RATE_LOST;
	data->state = NXP_SFA_STATE_CONFIGURED;
	cb = data->cb;
	user_data = data->user_data;

	k_spin_unlock(&data->lock, key);

	/* REF_STOPPED without completion: the target clock did not catch up. */
	if ((flags & (uint32_t)kSFA_RefStoppedFlag) != 0U) {
		LOG_WRN("%s: no completion (CNT_STAT=0x%02x) - target clock not running?",
			dev->name, flags);
	} else {
		LOG_WRN("%s: no completion (CNT_STAT=0x%02x) - reference clock not running?",
			dev->name, flags);
	}

	nxp_sfa_report(dev, cb, user_data, CLOCK_MONITOR_EVT_CLOCK_LOST, 0U);
}

static int nxp_sfa_init(const struct device *dev)
{
	const struct nxp_sfa_config *config = dev->config;
	struct nxp_sfa_data *data = dev->data;
	int ret;

	data->dev = dev;
	k_timer_init(&data->guard, nxp_sfa_guard_expired, NULL);

	if (!device_is_ready(config->gate_clk_dev)) {
		LOG_ERR("%s: gate clock device not ready", dev->name);
		return -ENODEV;
	}

	/* Enable the gate clock here instead of in SFA_Init(). */
	ret = clock_control_on(config->gate_clk_dev, config->gate_clk_subsys);
	if (ret != 0) {
		LOG_ERR("%s: failed to enable gate clock (%d)", dev->name, ret);
		return ret;
	}

	nxp_sfa_hw_abort(config->base);
	config->irq_config_func();
	return 0;
}

static DEVICE_API(clock_monitor, nxp_sfa_api) = {
	.configure = nxp_sfa_configure,
	.start     = nxp_sfa_start,
	.stop      = nxp_sfa_stop,
	.get_rate  = nxp_sfa_get_rate,
};

#define NXP_SFA_DEVICE_INIT(inst)                                              \
	BUILD_ASSERT(DT_INST_PROP(inst, reference_frequency) != 0,             \
		     "nxp,sfa: reference-frequency must be nonzero");          \
	static void nxp_sfa_irq_cfg_##inst(void)                               \
	{                                                                      \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),   \
			    nxp_sfa_isr, DEVICE_DT_INST_GET(inst), 0);         \
		irq_enable(DT_INST_IRQN(inst));                                \
	}                                                                      \
	static struct nxp_sfa_data nxp_sfa_data_##inst;                        \
	static const struct nxp_sfa_config nxp_sfa_cfg_##inst = {              \
		.base = (SFA_Type *)DT_INST_REG_ADDR(inst),                    \
		.gate_clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),      \
		.gate_clk_subsys = (clock_control_subsys_t)(uintptr_t)         \
			DT_INST_CLOCKS_CELL(inst, name),                       \
		.ref_hz = DT_INST_PROP(inst, reference_frequency),             \
		.tar_hz = DT_INST_PROP_OR(inst, target_frequency, 0),          \
		.ref_sel = DT_INST_PROP(inst, reference_select),               \
		.cut_sel = DT_INST_PROP(inst, target_select),                  \
		.irq_config_func = nxp_sfa_irq_cfg_##inst,                     \
	};                                                                     \
	DEVICE_DT_INST_DEFINE(inst, nxp_sfa_init, NULL,                        \
			      &nxp_sfa_data_##inst, &nxp_sfa_cfg_##inst,       \
			      POST_KERNEL, CONFIG_CLOCK_MONITOR_INIT_PRIORITY, \
			      &nxp_sfa_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SFA_DEVICE_INIT)
