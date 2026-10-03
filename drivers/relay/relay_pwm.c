/*
 * Copyright (c) 2026 Siemens AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_pwm_relay

#include <zephyr/drivers/relay/relay.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <stdint.h>

LOG_MODULE_REGISTER(relay_pwm, CONFIG_RELAY_LOG_LEVEL);

#define MAX_PERCENT 100U

struct relay_pwm_config {
	struct pwm_dt_spec pwm;
	uint32_t pull_in_time_ms;
	uint32_t refresh_interval_ms;
	uint8_t pull_in_duty_percent;
	uint8_t hold_duty_percent;
};

/* Coil drive phases cycled while the relay is switched on. */
enum relay_phase {
	RELAY_PHASE_OFF,     /* Coil released; no deferred work scheduled. */
	RELAY_PHASE_PULL_IN, /* Coil driven at pull-in duty for pull_in_time_ms. */
	RELAY_PHASE_HOLDING, /* Coil driven at hold duty for refresh_interval_ms. */
};

struct relay_pwm_data {
	struct k_mutex lock;
	/* Steady-state target drive (RELAY_STATE_OFF == released). */
	enum relay_state state;
	enum relay_phase phase;
	/* Result of the most recent coil-drive attempt; 0 clears it on success. */
	int last_err;
	struct k_work_delayable work;
	const struct device *dev;
};

/* Program the coil PWM to the given duty (0-100 %). Caller holds the lock. */
static int program_duty(const struct device *dev, uint8_t duty_percent)
{
	const struct relay_pwm_config *cfg = (const struct relay_pwm_config *)dev->config;
	const struct pwm_dt_spec *spec = &cfg->pwm;

	uint32_t pulse = (uint32_t)(((uint64_t)spec->period * duty_percent) / MAX_PERCENT);
	int err = pwm_set_dt(spec, spec->period, pulse);

	if (err != 0) {
		LOG_ERR("%s: pwm_set failed (err %d)", dev->name, err);
	}
	return err;
}

/* HOLDING: drive the hold duty. With refresh_interval_ms == 0 the coil holds
 * indefinitely with no further ticks; otherwise it is re-pulsed via PULL_IN
 * after refresh_interval_ms. Phase and the next tick are committed only after a
 * successful write; a failed write keeps the last confirmed phase and arms
 * nothing, leaving the retry to the caller. Caller holds the lock.
 */
static int enter_holding(const struct device *dev)
{
	const struct relay_pwm_config *cfg = (const struct relay_pwm_config *)dev->config;
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;
	int err;

	err = program_duty(dev, cfg->hold_duty_percent);
	if (err == 0) {
		data->phase = RELAY_PHASE_HOLDING;
		if (cfg->refresh_interval_ms > 0) {
			(void)k_work_reschedule(&data->work, K_MSEC(cfg->refresh_interval_ms));
		}
	}
	return err;
}

/* PULL_IN: drive the pull-in pulse, then fall to HOLDING after pull_in_time_ms.
 * With pull_in_time_ms == 0 there is no distinct pulse and the coil goes
 * straight to the hold duty. Phase and the next tick are committed only after a
 * successful write; a failed write keeps the last confirmed phase and arms
 * nothing, so the coil never advances to hold on a failed pull-in. Caller holds
 * the lock.
 */
static int enter_pull_in(const struct device *dev)
{
	const struct relay_pwm_config *cfg = (const struct relay_pwm_config *)dev->config;
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;
	int err;

	if (cfg->pull_in_time_ms == 0) {
		return enter_holding(dev);
	}

	err = program_duty(dev, cfg->pull_in_duty_percent);
	if (err == 0) {
		data->phase = RELAY_PHASE_PULL_IN;
		(void)k_work_reschedule(&data->work, K_MSEC(cfg->pull_in_time_ms));
	}
	return err;
}

/* Drive the coil one step toward data->state and return the hardware result.
 * Sole mutator of data->phase and the coil PWM, shared by the synchronous
 * set_state path and the deferred refresh/hold timing; latches the outcome so a
 * failure in a deferred tick is not lost. Caller holds the lock.
 */
static int relay_pwm_reconcile(const struct device *dev)
{
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;
	int err = 0;

	if (data->state == RELAY_STATE_OFF) {
		/* Release the coil. Commit RELAY_PHASE_OFF and drop the timer only
		 * once the write succeeds; a failed release keeps its phase so the
		 * caller retries and no stale tick advances the coil.
		 */
		if (data->phase != RELAY_PHASE_OFF) {
			err = program_duty(dev, 0);
			if (err == 0) {
				data->phase = RELAY_PHASE_OFF;
				(void)k_work_cancel_delayable(&data->work);
			}
		}
	} else {
		switch (data->phase) {
		case RELAY_PHASE_OFF:
		case RELAY_PHASE_HOLDING:
			err = enter_pull_in(dev);
			break;
		case RELAY_PHASE_PULL_IN:
			err = enter_holding(dev);
			break;
		}
	}

	data->last_err = err;
	return err;
}

/* Deferred tick: advance the coil through its refresh/hold timing and, unlike
 * the synchronous set_state path, keep retrying a failed step. No caller
 * observes a deferred failure, so a transient bus error must self-heal here.
 * Skips itself when a newer tick has already superseded it.
 */
static void relay_refresh_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct relay_pwm_data *data = CONTAINER_OF(dwork, struct relay_pwm_data, work);
	const struct relay_pwm_config *cfg = (const struct relay_pwm_config *)data->dev->config;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	/* Skip if a newer tick (armed by set_state) has already superseded this one. */
	if ((k_work_delayable_busy_get(dwork) & (K_WORK_DELAYED | K_WORK_QUEUED)) == 0) {
		if (relay_pwm_reconcile(data->dev) != 0) {
			/* Re-arm at the failed step's cadence: the refresh interval when
			 * re-pulsing from HOLDING, else the pull-in time. Both are
			 * non-zero in their phase; guard against 0 so a stray case cannot
			 * spin the workqueue.
			 */
			uint32_t retry_ms = (data->phase == RELAY_PHASE_HOLDING)
						    ? cfg->refresh_interval_ms
						    : cfg->pull_in_time_ms;

			if (retry_ms > 0) {
				(void)k_work_reschedule(&data->work, K_MSEC(retry_ms));
			}
		}
	}
	k_mutex_unlock(&data->lock);
}

static int relay_pwm_set_state(const struct device *dev, enum relay_state state)
{
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;
	int err = 0;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	if (data->state != state) {
		enum relay_state prev = data->state;

		/* Apply the commanded change synchronously so a coil-drive failure
		 * reaches the caller; the work item only runs later refresh/hold
		 * timing. On failure restore the previous target so the switch reads
		 * as not taken and a later call retries it; reconcile arms no timer on
		 * failure, so nothing is left dangling.
		 */
		data->state = state;
		err = relay_pwm_reconcile(dev);
		if (err != 0) {
			data->state = prev;
		}
	} else {
		/* Already in the requested steady state, including after a failed
		 * transition was rolled back to it: nothing to drive, so clear any
		 * stale latched error and report success.
		 */
		data->last_err = 0;
	}
	k_mutex_unlock(&data->lock);
	return err;
}

static int relay_pwm_get_state(const struct device *dev, enum relay_state *state)
{
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;
	int err;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	*state = data->state;
	/* Surface a failure latched by a deferred refresh/hold transition. */
	err = data->last_err;
	k_mutex_unlock(&data->lock);
	return err;
}

static int relay_pwm_init(const struct device *dev)
{
	const struct relay_pwm_config *cfg = (const struct relay_pwm_config *)dev->config;
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;

	k_mutex_init(&data->lock);
	if (!pwm_is_ready_dt(&cfg->pwm)) {
		LOG_ERR("%s: pwm backend not ready", dev->name);
		return -ENODEV;
	}
	data->dev = dev;
	data->phase = RELAY_PHASE_OFF;
	k_work_init_delayable(&data->work, relay_refresh_work_handler);
	/* Drive the coil to its released state. */
	(void)program_duty(dev, 0);
	return 0;
}

static DEVICE_API(relay, relay_pwm_api) = {
	.set_state = relay_pwm_set_state,
	.get_state = relay_pwm_get_state,
};

#define RELAY_PWM_DEFINE(inst)                                                                     \
	BUILD_ASSERT(DT_INST_PROP(inst, pull_in_duty_percent) <= MAX_PERCENT,                      \
		     "pull-in-duty-percent must be in the range 0..100");                          \
	BUILD_ASSERT(DT_INST_PROP(inst, hold_duty_percent) <= MAX_PERCENT,                         \
		     "hold-duty-percent must be in the range 0..100");                             \
	/* The pull-in duty is only applied during the pull-in pulse (skipped when ms is 0). */    \
	BUILD_ASSERT(DT_INST_PROP(inst, pull_in_time_ms) > 0 ||                                    \
			     DT_INST_PROP(inst, pull_in_duty_percent) ==                           \
				     DT_INST_PROP(inst, hold_duty_percent),                        \
		     "pull-in-duty-percent has no effect unless pull-in-time-ms is set");          \
	/* The pull-in pulse is the actuation; holding harder than pulling in is backwards. */     \
	BUILD_ASSERT(DT_INST_PROP(inst, pull_in_duty_percent) >=                                   \
			     DT_INST_PROP(inst, hold_duty_percent),                                \
		     "pull-in-duty-percent must be >= hold-duty-percent");                         \
	/* A refresh re-applies the pull-in pulse, so it needs one to exist. */                    \
	BUILD_ASSERT(DT_INST_PROP(inst, refresh_interval_ms) == 0 ||                               \
			     DT_INST_PROP(inst, pull_in_time_ms) > 0,                              \
		     "refresh-interval-ms requires pull-in-time-ms to be set");                    \
	static const struct relay_pwm_config relay_pwm_config_##inst = {                           \
		.pwm = PWM_DT_SPEC_INST_GET(inst),                                                 \
		.pull_in_time_ms = DT_INST_PROP(inst, pull_in_time_ms),                            \
		.refresh_interval_ms = DT_INST_PROP(inst, refresh_interval_ms),                    \
		.pull_in_duty_percent = DT_INST_PROP(inst, pull_in_duty_percent),                  \
		.hold_duty_percent = DT_INST_PROP(inst, hold_duty_percent),                        \
	};                                                                                         \
	static struct relay_pwm_data relay_pwm_data_##inst = {                                     \
		.state = RELAY_STATE_OFF,                                                          \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, relay_pwm_init, NULL, &relay_pwm_data_##inst,                  \
			      &relay_pwm_config_##inst, POST_KERNEL,                               \
			      CONFIG_RELAY_PWM_INIT_PRIORITY, &relay_pwm_api);

DT_INST_FOREACH_STATUS_OKAY(RELAY_PWM_DEFINE)
