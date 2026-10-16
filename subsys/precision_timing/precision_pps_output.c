/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/precision_timing/precision_pps_output.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

/** Fixed output period; every instance arms a one-pulse-per-second waveform. */
#define PPS_OUTPUT_PERIOD_NS ((precision_time_t)NSEC_PER_SEC)

/*
 * Preserve a configured output across one transient status-query failure, but
 * force a stop/rearm when the provider keeps returning an error.
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
			     CONFIG_PRECISION_PPS_OUTPUT_WORKQUEUE_STACK_SIZE);
static atomic_t pps_output_work_q_ready = ATOMIC_INIT(0);
static K_MUTEX_DEFINE(pps_output_work_q_lock);

static void pps_output_work_handler(struct k_work *work);

/*
 * Start the shared workqueue exactly once. Serialized by a mutex and guarded
 * by an atomic so that the SYS_INIT entry and any early precision_pps_output_start()
 * caller converge on a single k_work_queue_start().
 */
static void pps_output_work_q_ensure_started(void)
{
	if (atomic_get(&pps_output_work_q_ready) != 0) {
		return;
	}

	k_mutex_lock(&pps_output_work_q_lock, K_FOREVER);
	if (atomic_get(&pps_output_work_q_ready) == 0) {
		static const struct k_work_queue_config cfg = {
			.name = "pps_output",
		};

		k_work_queue_start(&pps_output_work_q, pps_output_work_q_stack,
				   K_KERNEL_STACK_SIZEOF(pps_output_work_q_stack),
				   CONFIG_PRECISION_PPS_OUTPUT_WORKQUEUE_PRIORITY, &cfg);
		atomic_set(&pps_output_work_q_ready, 1);
	}
	k_mutex_unlock(&pps_output_work_q_lock);
}

static int pps_output_work_q_init(void)
{
	pps_output_work_q_ensure_started();
	return 0;
}

SYS_INIT(pps_output_work_q_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static precision_time_t pps_output_monotonic_ns(void)
{
	return (precision_time_t)k_ticks_to_ns_floor64(k_uptime_ticks());
}

static void counter_increment(uint32_t *counter)
{
	if (*counter < UINT32_MAX) {
		(*counter)++;
	}
}

/*
 * Reset only the volatile runtime state ahead of a fresh start. The lock and
 * delayable work are kernel objects initialized once by precision_pps_output_init()
 * and are deliberately left untouched here.
 */
static void pps_output_reset_runtime(struct precision_pps_output *pps)
{
	pps->caps = (struct precision_clock_output_caps){0};
	pps->state = (struct precision_pps_output_state){
		.rearm_pending = true,
	};
	pps->previous_phc_ns = 0;
	pps->previous_monotonic_ns = 0;
	pps->previous_monotonic_end_ns = 0;
	pps->have_previous = false;
	pps->ever_armed = false;
	pps->force_rearm = false;
	pps->read_failed = false;
	pps->status_failed = false;
	pps->consecutive_status_errors = 0U;
}

static int validate_config(const struct precision_pps_output_config *config)
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

	return 0;
}

static int validate_caps(const struct precision_pps_output *pps)
{
	const struct precision_clock_output_caps *caps = &pps->caps;
	bool exact = pps->config.width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT;

	if ((caps->flags & PRECISION_CLOCK_OUTPUT_CAP_WAVEFORM) == 0U) {
		return -ENOTSUP;
	}
	if (exact && (caps->flags & PRECISION_CLOCK_OUTPUT_CAP_PROGRAMMABLE_WIDTH) == 0U) {
		return -ENOTSUP;
	}

	if (caps->resolution_ns <= 0 || caps->min_lead_time_ns < 0 || caps->min_period_ns <= 0 ||
	    caps->max_period_ns < caps->min_period_ns) {
		return -ERANGE;
	}
	if (PPS_OUTPUT_PERIOD_NS < caps->min_period_ns ||
	    PPS_OUTPUT_PERIOD_NS > caps->max_period_ns ||
	    (PPS_OUTPUT_PERIOD_NS % caps->resolution_ns) != 0) {
		return -ERANGE;
	}

	if (exact) {
		if (caps->min_pulse_width_ns <= 0 ||
		    caps->max_pulse_width_ns < caps->min_pulse_width_ns) {
			return -ERANGE;
		}
		if (pps->effective_pulse_width_ns < caps->min_pulse_width_ns ||
		    pps->effective_pulse_width_ns > caps->max_pulse_width_ns ||
		    (pps->effective_pulse_width_ns % caps->resolution_ns) != 0) {
			return -ERANGE;
		}
	}

	return 0;
}

/*
 * Decide whether a reported waveform is the one this instance owns. The width
 * is compared only for the exact policy; a provider-default waveform retains
 * that policy while its implementation-defined resolved width is not exposed.
 */
static bool waveform_config_matches(const struct precision_pps_output *pps,
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
	    waveform->pulse_width_ns != pps->effective_pulse_width_ns) {
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
static bool waveform_status_ok(const struct precision_pps_output *pps,
			       const struct precision_clock_output_status *status)
{
	if (!status->configured || status->kind != PRECISION_CLOCK_OUTPUT_KIND_WAVEFORM) {
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

/* The PHC sample was taken somewhere between the two monotonic observations. */
static int read_clock(struct precision_pps_output *pps, precision_time_t *now,
		      precision_time_t *before_ns, precision_time_t *after_ns)
{
	int ret;

	*before_ns = pps_output_monotonic_ns();
	ret = precision_clock_read(pps->precision_clk, now);
	*after_ns = pps_output_monotonic_ns();
	return ret;
}

static bool continuity_hard_step(struct precision_pps_output *pps, precision_time_t now,
				 precision_time_t before_ns, precision_time_t after_ns)
{
	precision_time_t phc_elapsed;
	precision_time_t elapsed_min;
	precision_time_t elapsed_max;
	precision_time_t error_ns = 0;
	int ret;

	if (!pps->have_previous) {
		return false;
	}

	ret = precision_time_sub(now, pps->previous_phc_ns, &phc_elapsed);
	if (ret == 0) {
		ret = precision_time_sub(before_ns, pps->previous_monotonic_end_ns, &elapsed_min);
	}
	if (ret == 0) {
		ret = precision_time_sub(after_ns, pps->previous_monotonic_ns, &elapsed_max);
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
		}
	}
	if (ret < 0) {
		pps->state.continuity_error_ns =
			now >= pps->previous_phc_ns ? PRECISION_TIME_MAX : PRECISION_TIME_MIN;
		return true;
	}

	pps->state.continuity_error_ns = error_ns;
	return error_ns > pps->config.step_limit_ns || error_ns < -pps->config.step_limit_ns;
}

static void remember_read(struct precision_pps_output *pps, precision_time_t now,
			  precision_time_t before_ns, precision_time_t after_ns)
{
	pps->previous_phc_ns = now;
	pps->previous_monotonic_ns = before_ns;
	pps->previous_monotonic_end_ns = after_ns;
	pps->state.phc_time_ns = now;
	pps->state.phc_read_valid = true;
	pps->have_previous = true;
}

static int stop_for_rearm(struct precision_pps_output *pps, uint32_t *events)
{
	int ret = precision_clock_output_stop(pps->precision_clk, pps->config.channel);

	if (ret < 0) {
		pps->state.last_error = ret;
		*events |= PRECISION_PPS_OUTPUT_EVENT_STOP_ERROR;
		return ret;
	}

	pps->state.active = false;
	pps->consecutive_status_errors = 0U;
	return 0;
}

static int start_from_read(struct precision_pps_output *pps, precision_time_t now, uint32_t *events)
{
	struct precision_clock_output_waveform_config waveform = {
		.period_ns = PPS_OUTPUT_PERIOD_NS,
		.width_policy = pps->config.width_policy,
		.pulse_width_ns = pps->effective_pulse_width_ns,
	};
	precision_time_t lead_time_ns;
	bool recovery = pps->ever_armed;
	int ret;

	ret = precision_time_add(pps->caps.min_lead_time_ns, pps->config.start_guard_ns,
				 &lead_time_ns);
	if (ret == 0) {
		ret = precision_clock_output_next_start_time(
			now, PPS_OUTPUT_PERIOD_NS, lead_time_ns, &waveform.first_rising_time);
	}
	if (ret == 0) {
		ret = precision_clock_output_start_waveform(pps->precision_clk, pps->config.channel,
							    &waveform);
	}
	if (ret < 0) {
		pps->state.last_error = ret;
		pps->state.rearm_pending = true;
		*events |= PRECISION_PPS_OUTPUT_EVENT_START_ERROR;
		return ret;
	}

	pps->state.config = waveform;
	pps->state.active = true;
	pps->state.rearm_pending = false;
	pps->force_rearm = false;
	pps->consecutive_status_errors = 0U;
	counter_increment(&pps->state.generation);
	if (recovery) {
		counter_increment(&pps->state.rearm_count);
		*events |= PRECISION_PPS_OUTPUT_EVENT_RECOVERED;
	} else {
		*events |= PRECISION_PPS_OUTPUT_EVENT_ARMED;
	}
	pps->ever_armed = true;

	return 0;
}

/*
 * Poll the registered clock and output channel once. At most one stop and one
 * start attempt are made per call, so a transient failure is retried by the
 * next scheduled poll rather than in a tight loop. Called with pps->lock held.
 */
static void pps_output_poll_locked(struct precision_pps_output *pps, uint32_t *events)
{
	struct precision_clock_output_status status;
	precision_time_t now;
	precision_time_t before_ns;
	precision_time_t after_ns;
	uint32_t current_events = 0;
	bool hard_step = false;
	bool read_ok;
	bool current_error = false;
	int ret;

	ret = read_clock(pps, &now, &before_ns, &after_ns);
	read_ok = ret == 0;
	if (!read_ok) {
		pps->state.phc_read_valid = false;
		pps->state.last_error = ret;
		if (!pps->read_failed) {
			current_events |= PRECISION_PPS_OUTPUT_EVENT_READ_ERROR;
		}
		pps->read_failed = true;
		current_error = true;
	} else {
		if (pps->read_failed) {
			current_events |= PRECISION_PPS_OUTPUT_EVENT_READ_RECOVERED;
		}
		pps->read_failed = false;
		hard_step = continuity_hard_step(pps, now, before_ns, after_ns);
		remember_read(pps, now, before_ns, after_ns);
		if (hard_step) {
			pps->state.rearm_pending = true;
			pps->force_rearm = true;
			current_events |= PRECISION_PPS_OUTPUT_EVENT_HARD_STEP;
		}
	}

	ret = precision_clock_output_get_status(pps->precision_clk, pps->config.channel, &status);
	if (ret != 0) {
		pps->state.last_error = ret;
		if (!pps->status_failed) {
			current_events |= PRECISION_PPS_OUTPUT_EVENT_STATUS_ERROR;
		}
		pps->status_failed = true;
		if (pps->consecutive_status_errors < UINT8_MAX) {
			pps->consecutive_status_errors++;
		}
		if (pps->consecutive_status_errors >= PPS_OUTPUT_STATUS_ERROR_REARM_THRESHOLD) {
			pps->state.rearm_pending = true;
			pps->force_rearm = true;
		}
		current_error = true;
	} else {
		if (pps->status_failed) {
			current_events |= PRECISION_PPS_OUTPUT_EVENT_STATUS_RECOVERED;
		}
		pps->status_failed = false;
		pps->consecutive_status_errors = 0U;

		if (waveform_status_ok(pps, &status)) {
			/*
			 * The channel is configured, matching, and not reported
			 * physically inactive. Adopt or heal it unless a hard
			 * step already forced a rearm this poll.
			 */
			if (!pps->force_rearm) {
				pps->state.active = true;
				pps->state.config = status.config.waveform;
				pps->state.rearm_pending = false;
				if (!pps->ever_armed) {
					pps->ever_armed = true;
					pps->state.generation = 1;
					current_events |= PRECISION_PPS_OUTPUT_EVENT_ARMED;
				}
			}
		} else {
			/* Unconfigured, wrong kind, mismatched, or inactive. */
			if (pps->ever_armed) {
				current_events |= PRECISION_PPS_OUTPUT_EVENT_OUTPUT_INACTIVE;
			}
			pps->state.active = false;
			pps->state.rearm_pending = true;
			pps->force_rearm = true;
		}
	}

	if (pps->state.rearm_pending && pps->force_rearm) {
		ret = stop_for_rearm(pps, &current_events);
		if (ret < 0) {
			current_error = true;
			goto out;
		}
		if (read_ok) {
			/* A synchronous provider stop can cross the next second boundary. */
			ret = read_clock(pps, &now, &before_ns, &after_ns);
			if (ret < 0) {
				pps->state.phc_read_valid = false;
				pps->state.last_error = ret;
				pps->read_failed = true;
				current_events |= PRECISION_PPS_OUTPUT_EVENT_READ_ERROR;
				current_error = true;
				goto out;
			}
			remember_read(pps, now, before_ns, after_ns);
			ret = start_from_read(pps, now, &current_events);
			if (ret < 0) {
				current_error = true;
			}
		}
	}

out:
	if (!current_error && pps->state.active && !pps->state.rearm_pending) {
		pps->state.last_error = 0;
	}
	*events = current_events;
}

static void pps_output_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct precision_pps_output *pps = CONTAINER_OF(dwork, struct precision_pps_output, work);
	struct precision_pps_output_state snapshot;
	precision_pps_output_callback_t callback = NULL;
	void *user_data = NULL;
	uint32_t events;
	bool notify;

	k_mutex_lock(&pps->lock, K_FOREVER);
	if (!pps->started || pps->stopping) {
		k_mutex_unlock(&pps->lock);
		return;
	}

	pps_output_poll_locked(pps, &events);

	notify = events != PRECISION_PPS_OUTPUT_EVENT_NONE;
	if (notify) {
		snapshot = pps->state;
		callback = pps->callback;
		user_data = pps->user_data;
	}
	if (pps->started && !pps->stopping) {
		(void)k_work_reschedule_for_queue(&pps_output_work_q, &pps->work,
						  K_MSEC(pps->config.poll_interval_ms));
	}
	k_mutex_unlock(&pps->lock);

	if (notify) {
		callback(pps, events, &snapshot, user_data);
	}
}

/*
 * A callback invoked by the instance's own poll cannot wait for that same poll
 * to finish on the shared workqueue thread. A stop targeting a different
 * instance from a callback is allowed because that instance's work is not the
 * one currently running on the single-threaded queue.
 */
static bool pps_output_self_context(const struct precision_pps_output *pps)
{
	return k_current_get() == k_work_queue_thread_get(&pps_output_work_q) &&
	       (k_work_delayable_busy_get(&pps->work) & K_WORK_RUNNING) != 0U;
}

/*
 * Synchronously cancel without a stack-allocated k_work_sync, which is not
 * coherent on the caller's stack when CONFIG_KERNEL_COHERENCE is set.
 */
static void pps_output_cancel_work_sync(struct precision_pps_output *pps)
{
	(void)k_work_cancel_delayable(&pps->work);

	while (k_work_delayable_busy_get(&pps->work) != 0) {
		k_sleep(K_TICKS(1));
	}
}

int precision_pps_output_init(struct precision_pps_output *pps,
			      const struct precision_clock *precision_clk,
			      const struct precision_pps_output_config *config,
			      precision_pps_output_callback_t callback, void *user_data)
{
	int ret;

	if (pps == NULL || precision_clk == NULL || config == NULL || callback == NULL) {
		return -EINVAL;
	}

	ret = validate_config(config);
	if (ret != 0) {
		return ret;
	}

	memset(pps, 0, sizeof(*pps));
	pps->precision_clk = precision_clk;
	pps->callback = callback;
	pps->user_data = user_data;
	pps->config = *config;
	if (pps->config.width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_PROVIDER_DEFAULT) {
		pps->config.pulse_width_ns = 0;
	}
	pps->effective_pulse_width_ns = config->width_policy == PRECISION_CLOCK_OUTPUT_WIDTH_EXACT
						? config->pulse_width_ns
						: 0;
	pps->state.rearm_pending = true;
	k_mutex_init(&pps->lock);
	k_work_init_delayable(&pps->work, pps_output_work_handler);

	return 0;
}

int precision_pps_output_start(struct precision_pps_output *pps)
{
	int ret;

	if (pps == NULL) {
		return -EINVAL;
	}

	pps_output_work_q_ensure_started();

	k_mutex_lock(&pps->lock, K_FOREVER);
	if (pps->started) {
		k_mutex_unlock(&pps->lock);
		return -EALREADY;
	}

	pps_output_reset_runtime(pps);

	ret = precision_clock_output_get_caps(pps->precision_clk, pps->config.channel, &pps->caps);
	if (ret == 0) {
		ret = validate_caps(pps);
	}
	if (ret != 0) {
		k_mutex_unlock(&pps->lock);
		return ret;
	}

	pps->started = true;
	ret = k_work_schedule_for_queue(&pps_output_work_q, &pps->work, K_NO_WAIT);
	if (ret < 0) {
		pps->started = false;
		k_mutex_unlock(&pps->lock);
		return ret;
	}
	k_mutex_unlock(&pps->lock);

	return 0;
}

int precision_pps_output_state_get(struct precision_pps_output *pps,
				   struct precision_pps_output_state *state)
{
	if (pps == NULL || state == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&pps->lock, K_FOREVER);
	if (!pps->started) {
		k_mutex_unlock(&pps->lock);
		return -EINVAL;
	}
	*state = pps->state;
	k_mutex_unlock(&pps->lock);

	return 0;
}

int precision_pps_output_stop(struct precision_pps_output *pps)
{
	const struct precision_clock *precision_clk;
	uint32_t channel;
	int ret;

	if (pps == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&pps->lock, K_FOREVER);
	if (!pps->started) {
		k_mutex_unlock(&pps->lock);
		return -EALREADY;
	}
	if (pps->stopping) {
		k_mutex_unlock(&pps->lock);
		return -EBUSY;
	}
	if (pps_output_self_context(pps)) {
		k_mutex_unlock(&pps->lock);
		return -EDEADLK;
	}

	pps->stopping = true;
	precision_clk = pps->precision_clk;
	channel = pps->config.channel;
	k_mutex_unlock(&pps->lock);

	pps_output_cancel_work_sync(pps);

	ret = precision_clock_output_stop(precision_clk, channel);

	k_mutex_lock(&pps->lock, K_FOREVER);
	if (ret < 0) {
		pps->state.last_error = ret;
		pps->stopping = false;
		(void)k_work_reschedule_for_queue(&pps_output_work_q, &pps->work, K_NO_WAIT);
		k_mutex_unlock(&pps->lock);
		return ret;
	}

	pps->started = false;
	pps->stopping = false;
	pps->force_rearm = false;
	pps->read_failed = false;
	pps->status_failed = false;
	pps->consecutive_status_errors = 0U;
	k_mutex_unlock(&pps->lock);

	return 0;
}
