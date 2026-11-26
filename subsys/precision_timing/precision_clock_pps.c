/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/precision_timing/precision_clock_pps.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

/** Fixed output period; every instance arms a one-pulse-per-second waveform. */
#define PPS_OUTPUT_PERIOD_NS ((precision_time_t)NSEC_PER_SEC)

/*
 * Allow one transient status error. Trigger recovery once per uninterrupted
 * outage; repeatedly restarting an armed waveform can suppress its first edge.
 */
#define PPS_OUTPUT_STATUS_ERROR_REARM_THRESHOLD 2U

/*
 * Single subsystem-owned dedicated workqueue shared by every instance. The
 * instance poll work never runs on the system workqueue, so an application
 * callback cannot stall unrelated system work, and the self-stop deadlock
 * check has one well-known thread to compare against.
 */
static struct k_work_q pps_output_work_q;
static K_KERNEL_STACK_DEFINE(pps_output_work_q_stack,
			     CONFIG_PRECISION_CLOCK_PPS_WORKQUEUE_STACK_SIZE);
static atomic_t pps_output_work_q_ready = ATOMIC_INIT(0);
static K_MUTEX_DEFINE(pps_output_work_q_lock);

static void pps_output_work_handler(struct k_work *work);

/*
 * Start the shared workqueue exactly once, lazily from
 * precision_clock_pps_start(). Serialized by a mutex and guarded by an
 * atomic so that concurrent start() callers converge on a single
 * k_work_queue_start(). The statically allocated stack remains reserved for
 * the whole build; lazy init only avoids an active thread while no instance
 * is started.
 */
static void pps_output_work_q_ensure_started(void)
{
	if (atomic_get(&pps_output_work_q_ready) != 0) {
		return;
	}

	(void)k_mutex_lock(&pps_output_work_q_lock, K_FOREVER);
	if (atomic_get(&pps_output_work_q_ready) == 0) {
		static const struct k_work_queue_config cfg = {
			.name = "pps_output",
		};

		k_work_queue_start(&pps_output_work_q, pps_output_work_q_stack,
				   K_KERNEL_STACK_SIZEOF(pps_output_work_q_stack),
				   CONFIG_PRECISION_CLOCK_PPS_WORKQUEUE_PRIORITY, &cfg);
		atomic_set(&pps_output_work_q_ready, 1);
	}
	(void)k_mutex_unlock(&pps_output_work_q_lock);
}

static precision_time_t pps_output_monotonic_ns(void)
{
	int64_t ticks = k_uptime_ticks();

	return (precision_time_t)k_ticks_to_ns_floor64(ticks);
}

static int validate_config(const struct precision_clock_pps_config *config)
{
	switch (config->width_policy) {
	case PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT:
		break;
	case PRECISION_CLOCK_OUTPUT_WIDTH_EXACT:
		if (config->pulse_width_ns <= 0 || config->pulse_width_ns >= PPS_OUTPUT_PERIOD_NS) {
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (config->start_guard_ns < 0 || config->step_limit_ns < 0 ||
	    config->poll_interval_ms == 0U) {
		return -EINVAL;
	}

	/* Check before K_MSEC() narrows to 32 bits or produces K_FOREVER. */
	if (!IS_ENABLED(CONFIG_TIMEOUT_64BIT) &&
	    k_ms_to_ticks_ceil64(config->poll_interval_ms) >= UINT32_MAX) {
		return -ERANGE;
	}

	return 0;
}

static int validate_caps(const struct precision_clock_pps_config *config,
			 const struct precision_clock_output_channel_caps *caps)
{
	bool exact = config->width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT;

	if ((caps->flags & PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM) == 0U) {
		return -ENOTSUP;
	}
	if (exact && (caps->flags & PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH) == 0U) {
		return -ENOTSUP;
	}

	if (caps->resolution_ns <= 0 || caps->min_lead_time_ns < 0) {
		return -EINVAL;
	}

	if (PPS_OUTPUT_PERIOD_NS < caps->min_period_ns ||
	    PPS_OUTPUT_PERIOD_NS > caps->max_period_ns ||
	    (PPS_OUTPUT_PERIOD_NS % caps->resolution_ns) != 0) {
		return -ERANGE;
	}

	if (exact && (config->pulse_width_ns < caps->min_pulse_width_ns ||
		      config->pulse_width_ns > caps->max_pulse_width_ns ||
		      (config->pulse_width_ns % caps->resolution_ns) != 0)) {
		return -ERANGE;
	}

	return 0;
}

/*
 * Decide whether a reported waveform is the one this instance owns. The width
 * is compared only for the exact policy; a provider-default waveform retains
 * that policy while its implementation-defined resolved width is not exposed.
 */
static bool waveform_config_matches(const struct precision_clock_pps *pps,
				    const struct precision_clock_output_waveform_config *waveform)
{
	if ((waveform->first_rising_time % PPS_OUTPUT_PERIOD_NS) != 0) {
		return false;
	}
	if (waveform->period_ns != PPS_OUTPUT_PERIOD_NS) {
		return false;
	}
	if (waveform->width_policy != pps->config.width_policy) {
		return false;
	}
	if (pps->config.width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT &&
	    waveform->pulse_width_ns != pps->config.pulse_width_ns) {
		return false;
	}

	return true;
}

/*
 * A valid hardware-active observation reports an armed/running generator,
 * including before the first edge and between pulses. An inactive generator
 * requires recovery. If the observation is unavailable, the configured,
 * matching waveform remains the source of truth.
 */
static bool waveform_status_ok(const struct precision_clock_pps *pps,
			       const struct precision_clock_output_status *status)
{
	if (!status->configured || status->config.kind != PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM) {
		return false;
	}
	if (!waveform_config_matches(pps, &status->config.waveform)) {
		return false;
	}
	if (status->hardware_active_valid && !status->hardware_active) {
		return false;
	}

	return true;
}

static bool continuity_hard_step(const struct precision_clock_pps *pps, precision_time_t now,
				 precision_time_t before_ns, precision_time_t after_ns)
{
	precision_time_t phc_elapsed;
	precision_time_t elapsed_min;
	precision_time_t elapsed_max;
	precision_time_t error_ns = 0;
	int ret;

	ret = precision_time_sub(now, pps->clock_phc_ns, &phc_elapsed);
	if (ret == 0) {
		ret = precision_time_sub(before_ns, pps->clock_mono_post_ns, &elapsed_min);
	}
	if (ret == 0) {
		ret = precision_time_sub(after_ns, pps->clock_mono_pre_ns, &elapsed_max);
	}
	if (ret == 0 && (elapsed_min < 0 || elapsed_max < elapsed_min)) {
		ret = -ERANGE;
	}
	if (ret == 0) {
		/*
		 * Only the part outside the possible elapsed-time interval is
		 * evidence of a step. Include uncertainty from both reads, so a
		 * fast poll following a slow read cannot invent a reverse step.
		 */
		if (phc_elapsed > elapsed_max) {
			ret = precision_time_sub(phc_elapsed, elapsed_max, &error_ns);
		} else if (phc_elapsed < elapsed_min) {
			ret = precision_time_sub(phc_elapsed, elapsed_min, &error_ns);
		} else {
			return false; /* No measurable step within the sampling uncertainty. */
		}
	}
	return ret != 0 || error_ns > pps->config.step_limit_ns ||
	       error_ns < -pps->config.step_limit_ns;
}

/* The PHC sample was taken somewhere between the two monotonic observations. */
static bool sample_clock(struct precision_clock_pps *pps, bool check_step, uint32_t *events)
{
	precision_time_t now;
	precision_time_t before_ns = pps_output_monotonic_ns();
	int ret = precision_clock_read(pps->precision_clk, &now);
	precision_time_t after_ns = pps_output_monotonic_ns();

	if (ret != 0) {
		if (!pps->read_failed) {
			*events |= PRECISION_CLOCK_PPS_EVENT_READ_ERROR;
			pps->read_failed = true;
		}
		return false;
	}

	if (pps->read_failed) {
		*events |= PRECISION_CLOCK_PPS_EVENT_READ_RECOVERED;
		pps->read_failed = false;
	}
	if (check_step && pps->clock_read_done &&
	    continuity_hard_step(pps, now, before_ns, after_ns)) {
		*events |= PRECISION_CLOCK_PPS_EVENT_HARD_STEP;
	}

	pps->clock_phc_ns = now;
	pps->clock_mono_pre_ns = before_ns;
	pps->clock_mono_post_ns = after_ns;
	pps->clock_read_done = true;
	return true;
}

static void require_arm(struct precision_clock_pps *pps)
{
	if (pps->state.phase == PRECISION_CLOCK_PPS_STARTING) {
		pps->state.phase = PRECISION_CLOCK_PPS_ARMING;
		return;
	}
	if (pps->state.phase == PRECISION_CLOCK_PPS_ACTIVE) {
		pps->state.phase = PRECISION_CLOCK_PPS_REARMING;
	}
}

static void start_from_read(struct precision_clock_pps *pps, uint32_t *events)
{
	bool exact = pps->config.width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT;
	struct precision_clock_output_config config = {
		.kind = PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM,
		.waveform.period_ns = PPS_OUTPUT_PERIOD_NS,
		.waveform.width_policy = pps->config.width_policy,
		.waveform.pulse_width_ns = exact ? pps->config.pulse_width_ns : 0,
	};
	int ret;

	ret = precision_clock_output_next_start_time(pps->clock_phc_ns, PPS_OUTPUT_PERIOD_NS,
						     pps->start_lead_time_ns,
						     &config.waveform.first_rising_time);
	if (ret == 0) {
		ret = precision_clock_output_start(pps->precision_clk, pps->config.channel,
						   &config);
	}
	if (ret < 0) {
		*events |= PRECISION_CLOCK_PPS_EVENT_START_ERROR;
		return;
	}

	*events |= pps->state.phase == PRECISION_CLOCK_PPS_REARMING
			   ? PRECISION_CLOCK_PPS_EVENT_RECOVERED
			   : PRECISION_CLOCK_PPS_EVENT_ARMED;
	pps->state.config = config.waveform;
	pps->state.phase = PRECISION_CLOCK_PPS_ACTIVE;
}

static void poll_status(struct precision_clock_pps *pps, uint32_t *events)
{
	struct precision_clock_output_status status;
	int ret =
		precision_clock_output_get_status(pps->precision_clk, pps->config.channel, &status);

	if (ret != 0) {
		if (pps->consecutive_status_errors == 0U) {
			*events |= PRECISION_CLOCK_PPS_EVENT_STATUS_ERROR;
		}
		if (pps->consecutive_status_errors < PPS_OUTPUT_STATUS_ERROR_REARM_THRESHOLD) {
			pps->consecutive_status_errors++;
			if (pps->consecutive_status_errors ==
			    PPS_OUTPUT_STATUS_ERROR_REARM_THRESHOLD) {
				require_arm(pps);
			}
		}
		return;
	}

	if (pps->consecutive_status_errors != 0U) {
		*events |= PRECISION_CLOCK_PPS_EVENT_STATUS_RECOVERED;
		pps->consecutive_status_errors = 0U;
	}

	if (!waveform_status_ok(pps, &status)) {
		if (pps->state.phase == PRECISION_CLOCK_PPS_ACTIVE ||
		    pps->state.phase == PRECISION_CLOCK_PPS_REARMING) {
			*events |= PRECISION_CLOCK_PPS_EVENT_OUTPUT_INACTIVE;
		}
		require_arm(pps);
		return;
	}

	/* Only initial discovery may adopt an existing waveform. */
	if (pps->state.phase == PRECISION_CLOCK_PPS_STARTING) {
		pps->state.phase = PRECISION_CLOCK_PPS_ACTIVE;
		*events |= PRECISION_CLOCK_PPS_EVENT_ARMED;
	}
	if (pps->state.phase == PRECISION_CLOCK_PPS_ACTIVE) {
		pps->state.config = status.config.waveform;
	}
}

/* One stop and one start at most per poll; failures retry on the next poll. */
static uint32_t pps_output_poll_locked(struct precision_clock_pps *pps)
{
	uint32_t events = PRECISION_CLOCK_PPS_EVENT_NONE;
	bool read_ok = sample_clock(pps, true, &events);

	if ((events & PRECISION_CLOCK_PPS_EVENT_HARD_STEP) != 0U) {
		require_arm(pps);
	}
	poll_status(pps, &events);

	if (pps->state.phase != PRECISION_CLOCK_PPS_ARMING &&
	    pps->state.phase != PRECISION_CLOCK_PPS_REARMING) {
		return events;
	}
	if (precision_clock_output_stop(pps->precision_clk, pps->config.channel) < 0) {
		return events | PRECISION_CLOCK_PPS_EVENT_STOP_ERROR;
	}
	if (!read_ok) {
		return events;
	}

	/* A synchronous provider stop can cross the target second boundary. */
	if (sample_clock(pps, false, &events)) {
		start_from_read(pps, &events);
	}

	return events;
}

static void pps_output_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct precision_clock_pps *pps = CONTAINER_OF(dwork, struct precision_clock_pps, work);
	precision_clock_pps_callback_t callback = NULL;
	uint32_t events;

	(void)k_mutex_lock(&pps->lock, K_FOREVER);
	if (pps->state.phase == PRECISION_CLOCK_PPS_STOPPED ||
	    pps->state.phase == PRECISION_CLOCK_PPS_STOPPING) {
		(void)k_mutex_unlock(&pps->lock);
		return;
	}

	events = pps_output_poll_locked(pps);
	if (events != PRECISION_CLOCK_PPS_EVENT_NONE) {
		callback = pps->config.callback;
		if (callback != NULL) {
			pps->state.events = events;
		} else {
			pps->state.events |= events;
		}
	}
	(void)k_work_reschedule_for_queue(&pps_output_work_q, &pps->work,
					  K_MSEC(pps->config.poll_interval_ms));
	(void)k_mutex_unlock(&pps->lock);

	if (callback != NULL) {
		callback(pps);
	}
}

/*
 * A callback invoked by the instance's own poll cannot wait for that same poll
 * to finish on the shared workqueue thread. A stop targeting a different
 * instance from a callback is allowed because that instance's work is not the
 * one currently running on the single-threaded queue.
 */
static bool pps_output_self_context(const struct precision_clock_pps *pps)
{
	return k_current_get() == k_work_queue_thread_get(&pps_output_work_q) &&
	       (k_work_delayable_busy_get(&pps->work) & K_WORK_RUNNING) != 0U;
}

int precision_clock_pps_init(struct precision_clock_pps *pps,
			     const struct precision_clock *precision_clk,
			     const struct precision_clock_pps_config *config)
{
	const struct precision_clock_output_channel_caps *channel_caps;
	struct precision_clock_output_caps caps;
	precision_time_t lead_time_ns;
	int ret;

	if (pps == NULL || precision_clk == NULL || config == NULL) {
		return -EINVAL;
	}

	ret = validate_config(config);
	if (ret != 0) {
		return ret;
	}

	ret = precision_clock_output_get_caps(precision_clk, &caps);
	if (ret != 0) {
		return ret;
	}
	if (config->channel >= caps.channel_count) {
		return -ENOTSUP;
	}
	channel_caps = &caps.channels[config->channel];
	ret = validate_caps(config, channel_caps);
	if (ret != 0) {
		return ret;
	}
	ret = precision_time_add(channel_caps->min_lead_time_ns, config->start_guard_ns,
				 &lead_time_ns);
	if (ret != 0) {
		return ret;
	}

	memset(pps, 0, sizeof(*pps));
	pps->precision_clk = precision_clk;
	pps->config = *config;
	pps->start_lead_time_ns = lead_time_ns;
	(void)k_mutex_init(&pps->lock);
	k_work_init_delayable(&pps->work, pps_output_work_handler);

	return 0;
}

int precision_clock_pps_start(struct precision_clock_pps *pps)
{
	int ret;

	if (pps == NULL) {
		return -EINVAL;
	}

	(void)k_mutex_lock(&pps->lock, K_FOREVER);
	if (pps->state.phase != PRECISION_CLOCK_PPS_STOPPED) {
		(void)k_mutex_unlock(&pps->lock);
		return -EALREADY;
	}

	pps_output_work_q_ensure_started();
	ret = k_work_schedule_for_queue(&pps_output_work_q, &pps->work, K_NO_WAIT);
	if (ret < 0) {
		(void)k_mutex_unlock(&pps->lock);
		return ret;
	}

	pps->state.phase = PRECISION_CLOCK_PPS_STARTING;
	pps->state.events = PRECISION_CLOCK_PPS_EVENT_NONE;
	pps->clock_read_done = false;
	pps->read_failed = false;
	pps->consecutive_status_errors = 0U;
	(void)k_mutex_unlock(&pps->lock);

	return 0;
}

int precision_clock_pps_state_get(struct precision_clock_pps *pps,
				  struct precision_clock_pps_state *state)
{
	if (pps == NULL || state == NULL) {
		return -EINVAL;
	}

	(void)k_mutex_lock(&pps->lock, K_FOREVER);
	*state = pps->state;
	if (pps->config.callback == NULL) {
		pps->state.events = PRECISION_CLOCK_PPS_EVENT_NONE;
	}
	(void)k_mutex_unlock(&pps->lock);

	return 0;
}

int precision_clock_pps_stop(struct precision_clock_pps *pps)
{
	enum precision_clock_pps_phase previous_phase;
	int ret;

	if (pps == NULL) {
		return -EINVAL;
	}

	(void)k_mutex_lock(&pps->lock, K_FOREVER);
	previous_phase = pps->state.phase;
	if (previous_phase == PRECISION_CLOCK_PPS_STOPPED) {
		(void)k_mutex_unlock(&pps->lock);
		return -EALREADY;
	}
	if (pps_output_self_context(pps)) {
		(void)k_mutex_unlock(&pps->lock);
		return -EDEADLK;
	}
	if (previous_phase == PRECISION_CLOCK_PPS_STOPPING) {
		(void)k_mutex_unlock(&pps->lock);
		return -EBUSY;
	}

	pps->state.phase = PRECISION_CLOCK_PPS_STOPPING;
	(void)k_mutex_unlock(&pps->lock);

	(void)k_work_cancel_delayable_sync(&pps->work, &pps->work_sync);
	ret = precision_clock_output_stop(pps->precision_clk, pps->config.channel);

	(void)k_mutex_lock(&pps->lock, K_FOREVER);
	if (ret < 0) {
		pps->state.phase = previous_phase;
		(void)k_work_reschedule_for_queue(&pps_output_work_q, &pps->work, K_NO_WAIT);
		(void)k_mutex_unlock(&pps->lock);
		return ret;
	}

	pps->state.phase = PRECISION_CLOCK_PPS_STOPPED;
	(void)k_mutex_unlock(&pps->lock);

	return 0;
}
