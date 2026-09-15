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
 * after refresh_interval_ms. Caller holds the lock.
 */
static void enter_holding(const struct device *dev)
{
	const struct relay_pwm_config *cfg = (const struct relay_pwm_config *)dev->config;
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;

	data->phase = RELAY_PHASE_HOLDING;
	(void)program_duty(dev, cfg->hold_duty_percent);
	if (cfg->refresh_interval_ms > 0) {
		(void)k_work_reschedule(&data->work, K_MSEC(cfg->refresh_interval_ms));
	}
}

/* PULL_IN: drive the pull-in pulse, then fall to HOLDING after pull_in_time_ms.
 * With pull_in_time_ms == 0 there is no distinct pulse and the coil goes
 * straight to the hold duty. Caller holds the lock.
 */
static void enter_pull_in(const struct device *dev)
{
	const struct relay_pwm_config *cfg = (const struct relay_pwm_config *)dev->config;
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;

	if (cfg->pull_in_time_ms == 0) {
		enter_holding(dev);
		return;
	}

	data->phase = RELAY_PHASE_PULL_IN;
	(void)program_duty(dev, cfg->pull_in_duty_percent);
	(void)k_work_reschedule(&data->work, K_MSEC(cfg->pull_in_time_ms));
}

/* Deferred tick: reconcile the coil drive toward its requested target. Sole
 * owner of data->phase and the coil's PWM; set_state only publishes the target
 * and expedites this handler, so a stale or duplicate tick is harmless.
 */
static void relay_refresh_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct relay_pwm_data *data = CONTAINER_OF(dwork, struct relay_pwm_data, work);
	const struct device *dev = data->dev;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	if (data->state == RELAY_STATE_OFF) {
		/* Release the coil once, then stay settled (no reschedule). */
		if (data->phase != RELAY_PHASE_OFF) {
			data->phase = RELAY_PHASE_OFF;
			(void)program_duty(dev, 0);
		}
	} else {
		switch (data->phase) {
		case RELAY_PHASE_OFF:
		case RELAY_PHASE_HOLDING:
			enter_pull_in(dev);
			break;
		case RELAY_PHASE_PULL_IN:
			enter_holding(dev);
			break;
		}
	}
	k_mutex_unlock(&data->lock);
}

static int relay_pwm_set_state(const struct device *dev, enum relay_state state)
{
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	if (data->state != state) {
		data->state = state;
		/* Publish the target and expedite the handler; it owns the coil
		 * drive and reconciles to this target on its next run.
		 */
		(void)k_work_reschedule(&data->work, K_NO_WAIT);
	}
	k_mutex_unlock(&data->lock);
	return 0;
}

static int relay_pwm_get_state(const struct device *dev, enum relay_state *state)
{
	struct relay_pwm_data *data = (struct relay_pwm_data *)dev->data;

	(void)k_mutex_lock(&data->lock, K_FOREVER);
	*state = data->state;
	k_mutex_unlock(&data->lock);
	return 0;
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
