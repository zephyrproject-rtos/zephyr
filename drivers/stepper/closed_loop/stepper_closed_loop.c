/*
 * Copyright (c) 2026 Contributors to the Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_stepper_closed_loop

#include <stdlib.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/stepper/stepper_ctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/pid.h>
#include <zephyr/sys/util.h>

#include <stepper_ctrl_event_handler.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(stepper_closed_loop, CONFIG_STEPPER_LOG_LEVEL);

struct stepper_cl_config {
	const struct device *inner_ctrl;
	const struct device *encoder;
	enum sensor_channel encoder_channel;
	uint32_t encoder_counts_per_rev;
	uint32_t motor_steps_per_rev;
	struct pid_cfg pid;
	int32_t position_error_threshold;
	uint32_t feedback_interval_us;
	int32_t settling_window;
	uint8_t settling_count;
};

struct stepper_cl_data {
	struct stepper_ctrl_event_handler_data event_common;
	struct k_spinlock lock;
	struct pid_state pid_state;
	struct k_thread control_thread;
	K_KERNEL_STACK_MEMBER(control_stack, CONFIG_STEPPER_CLOSED_LOOP_THREAD_STACK_SIZE);
	struct k_sem loop_sem;
	atomic_t loop_active;
	int32_t target_position_enc;
	int32_t encoder_offset;
	enum stepper_ctrl_run_mode run_mode;
	uint8_t settle_counter;
	uint8_t encoder_fail_counter;
};

STEPPER_CONTROLLER_EVENT_COMMON_STRUCT_CHECK(struct stepper_cl_data);

/**
 * @brief Convert micro-steps to encoder counts.
 */
static int32_t usteps_to_enc(const struct stepper_cl_config *cfg, int32_t usteps)
{
	return (int32_t)(((int64_t)usteps * cfg->encoder_counts_per_rev) /
			 (cfg->motor_steps_per_rev));
}

/**
 * @brief Convert encoder counts to micro-steps.
 */
static int32_t enc_to_usteps(const struct stepper_cl_config *cfg, int32_t enc_counts)
{
	return (int32_t)(((int64_t)enc_counts * cfg->motor_steps_per_rev) /
			 (cfg->encoder_counts_per_rev));
}

/**
 * @brief Read the current encoder position in encoder counts.
 */
static int read_encoder_position(const struct stepper_cl_config *cfg,
				 const struct stepper_cl_data *data,
				 int32_t *position)
{
	struct sensor_value val;
	int ret;

	ret = sensor_sample_fetch(cfg->encoder);
	if (ret != 0) {
		return ret;
	}

	ret = sensor_channel_get(cfg->encoder, cfg->encoder_channel, &val);
	if (ret != 0) {
		return ret;
	}

	if (cfg->encoder_channel == SENSOR_CHAN_ROTATION) {
		/*
		 * SENSOR_CHAN_ROTATION returns degrees. Convert to encoder counts.
		 * degrees * counts_per_rev / 360
		 */
		int64_t degrees_micro = (int64_t)val.val1 * 1000000 + val.val2;

		*position = (int32_t)((degrees_micro * cfg->encoder_counts_per_rev) /
				      (360LL * 1000000));
	} else {
		/* SENSOR_CHAN_ENCODER_COUNT returns raw counts directly */
		*position = val.val1;
	}

	*position -= data->encoder_offset;

	return 0;
}

/**
 * @brief Closed-loop control thread entry point.
 */
static void stepper_cl_control_loop(void *p1, void *p2, void *p3)
{
	const struct device *dev = p1;
	const struct stepper_cl_config *cfg = dev->config;
	struct stepper_cl_data *data = dev->data;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		/* Sleep until a move command activates the loop */
		k_sem_take(&data->loop_sem, K_FOREVER);

		LOG_DBG("Control loop activated, target=%d enc counts",
			data->target_position_enc);

		pid_reset(&data->pid_state);
		data->settle_counter = 0;
		data->encoder_fail_counter = 0;

		while (atomic_get(&data->loop_active)) {
			int32_t actual_enc;
			int ret;

			ret = read_encoder_position(cfg, data, &actual_enc);
			if (ret != 0) {
				data->encoder_fail_counter++;
				LOG_WRN("Encoder read failed (%d), count=%u",
					ret, data->encoder_fail_counter);

				if (data->encoder_fail_counter >=
				    CONFIG_STEPPER_CLOSED_LOOP_MAX_ENCODER_FAILURES) {
					LOG_ERR("Encoder failure limit reached, stopping");
					stepper_ctrl_stop(cfg->inner_ctrl);
					atomic_set(&data->loop_active, 0);
					stepper_ctrl_event_handler_process_cb(
						dev, STEPPER_CTRL_EVENT_STOPPED);
					break;
				}

				k_usleep(cfg->feedback_interval_us);
				continue;
			}

			data->encoder_fail_counter = 0;

			int32_t error_enc;
			k_spinlock_key_t key = k_spin_lock(&data->lock);

			error_enc = data->target_position_enc - actual_enc;
			k_spin_unlock(&data->lock, key);

			/* Check position error threshold */
			if (cfg->position_error_threshold > 0 &&
			    abs(error_enc) > cfg->position_error_threshold) {
				LOG_WRN("Position error %d exceeds threshold %d",
					error_enc, cfg->position_error_threshold);
				stepper_ctrl_event_handler_process_cb(
					dev, STEPPER_CTRL_EVENT_POSITION_ERROR);
			}

			/* Check if settled */
			if (abs(error_enc) <= cfg->settling_window) {
				data->settle_counter++;
				if (data->settle_counter >= cfg->settling_count) {
					LOG_DBG("Position settled at enc=%d", actual_enc);
					atomic_set(&data->loop_active, 0);
					stepper_ctrl_event_handler_process_cb(
						dev, STEPPER_CTRL_EVENT_STEPS_COMPLETED);
					break;
				}
			} else {
				data->settle_counter = 0;
			}

			/* PID correction */
			int32_t correction_enc = pid_update(
				&cfg->pid, &data->pid_state,
				data->target_position_enc, actual_enc);

			if (correction_enc != 0) {
				int32_t correction_usteps = enc_to_usteps(cfg, correction_enc);

				if (correction_usteps != 0) {
					ret = stepper_ctrl_move_by(cfg->inner_ctrl,
								   correction_usteps);
					if (ret != 0) {
						LOG_ERR("Inner ctrl move_by failed: %d", ret);
						atomic_set(&data->loop_active, 0);
						stepper_ctrl_event_handler_process_cb(
							dev, STEPPER_CTRL_EVENT_STOPPED);
						break;
					}
				}
			}

			k_usleep(cfg->feedback_interval_us);
		}
	}
}

/* --- stepper_ctrl API implementation --- */

static int stepper_cl_set_reference_position(const struct device *dev, const int32_t value)
{
	const struct stepper_cl_config *cfg = dev->config;
	struct stepper_cl_data *data = dev->data;
	int32_t actual_enc;
	int ret;

	ret = read_encoder_position(cfg, data, &actual_enc);
	if (ret != 0) {
		return ret;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->encoder_offset = actual_enc - usteps_to_enc(cfg, value);
	k_spin_unlock(&data->lock, key);

	/* Also update the inner controller's reference */
	return stepper_ctrl_set_reference_position(cfg->inner_ctrl, value);
}

static int stepper_cl_get_actual_position(const struct device *dev, int32_t *value)
{
	const struct stepper_cl_config *cfg = dev->config;
	struct stepper_cl_data *data = dev->data;
	int32_t actual_enc;
	int ret;

	ret = read_encoder_position(cfg, data, &actual_enc);
	if (ret != 0) {
		return ret;
	}

	*value = enc_to_usteps(cfg, actual_enc);
	return 0;
}

static int stepper_cl_set_event_cb(const struct device *dev,
				   stepper_ctrl_event_callback_t callback,
				   void *user_data)
{
	return stepper_ctrl_event_handler_set_event_cb(dev, callback, user_data);
}

static int stepper_cl_set_microstep_interval(const struct device *dev,
					     const uint64_t microstep_interval_ns)
{
	const struct stepper_cl_config *cfg = dev->config;

	return stepper_ctrl_set_microstep_interval(cfg->inner_ctrl, microstep_interval_ns);
}

static int stepper_cl_configure_ramp(const struct device *dev,
				     const struct stepper_ctrl_ramp *ramp)
{
	const struct stepper_cl_config *cfg = dev->config;

	return stepper_ctrl_configure_ramp(cfg->inner_ctrl, ramp);
}

static int stepper_cl_move_by(const struct device *dev, const int32_t micro_steps)
{
	const struct stepper_cl_config *cfg = dev->config;
	struct stepper_cl_data *data = dev->data;
	int32_t actual_enc;
	int ret;

	ret = read_encoder_position(cfg, data, &actual_enc);
	if (ret != 0) {
		return ret;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->target_position_enc = actual_enc + usteps_to_enc(cfg, micro_steps);
	data->run_mode = STEPPER_CTRL_RUN_MODE_POSITION;
	k_spin_unlock(&data->lock, key);

	/* Issue the initial open-loop move */
	ret = stepper_ctrl_move_by(cfg->inner_ctrl, micro_steps);
	if (ret != 0) {
		return ret;
	}

	/* Activate feedback loop */
	atomic_set(&data->loop_active, 1);
	k_sem_give(&data->loop_sem);

	return 0;
}

static int stepper_cl_move_to(const struct device *dev, const int32_t micro_steps)
{
	const struct stepper_cl_config *cfg = dev->config;
	struct stepper_cl_data *data = dev->data;
	int ret;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->target_position_enc = usteps_to_enc(cfg, micro_steps);
	data->run_mode = STEPPER_CTRL_RUN_MODE_POSITION;
	k_spin_unlock(&data->lock, key);

	/* Issue the initial open-loop move */
	ret = stepper_ctrl_move_to(cfg->inner_ctrl, micro_steps);
	if (ret != 0) {
		return ret;
	}

	/* Activate feedback loop */
	atomic_set(&data->loop_active, 1);
	k_sem_give(&data->loop_sem);

	return 0;
}

static int stepper_cl_run(const struct device *dev,
			  const enum stepper_ctrl_direction direction)
{
	const struct stepper_cl_config *cfg = dev->config;
	struct stepper_cl_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	data->run_mode = STEPPER_CTRL_RUN_MODE_VELOCITY;
	k_spin_unlock(&data->lock, key);

	return stepper_ctrl_run(cfg->inner_ctrl, direction);
}

static int stepper_cl_stop(const struct device *dev)
{
	const struct stepper_cl_config *cfg = dev->config;
	struct stepper_cl_data *data = dev->data;

	atomic_set(&data->loop_active, 0);

	int ret = stepper_ctrl_stop(cfg->inner_ctrl);

	stepper_ctrl_event_handler_process_cb(dev, STEPPER_CTRL_EVENT_STOPPED);

	return ret;
}

static int stepper_cl_is_moving(const struct device *dev, bool *is_moving)
{
	struct stepper_cl_data *data = dev->data;

	*is_moving = atomic_get(&data->loop_active) != 0;

	return 0;
}

static DEVICE_API(stepper_ctrl, stepper_cl_api) = {
	.set_reference_position = stepper_cl_set_reference_position,
	.get_actual_position = stepper_cl_get_actual_position,
	.set_event_cb = stepper_cl_set_event_cb,
	.set_microstep_interval = stepper_cl_set_microstep_interval,
	.configure_ramp = stepper_cl_configure_ramp,
	.move_by = stepper_cl_move_by,
	.move_to = stepper_cl_move_to,
	.run = stepper_cl_run,
	.stop = stepper_cl_stop,
	.is_moving = stepper_cl_is_moving,
};

static int stepper_cl_init(const struct device *dev)
{
	const struct stepper_cl_config *cfg = dev->config;
	struct stepper_cl_data *data = dev->data;
	int ret;

	if (!device_is_ready(cfg->inner_ctrl)) {
		LOG_ERR("Inner stepper controller device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(cfg->encoder)) {
		LOG_ERR("Encoder sensor device not ready");
		return -ENODEV;
	}

	pid_init(&data->pid_state);
	k_sem_init(&data->loop_sem, 0, 1);
	atomic_set(&data->loop_active, 0);

	ret = stepper_ctrl_event_handler_init(dev);
	if (ret != 0) {
		LOG_ERR("Event handler init failed: %d", ret);
		return ret;
	}

	k_thread_create(&data->control_thread,
			data->control_stack,
			CONFIG_STEPPER_CLOSED_LOOP_THREAD_STACK_SIZE,
			stepper_cl_control_loop,
			(void *)dev, NULL, NULL,
			CONFIG_STEPPER_CLOSED_LOOP_THREAD_PRIORITY,
			0, K_NO_WAIT);

	k_thread_name_set(&data->control_thread, "stepper_cl");

	LOG_DBG("Closed-loop stepper initialized: enc_cpr=%u motor_spr=%u",
		cfg->encoder_counts_per_rev, cfg->motor_steps_per_rev);

	return 0;
}

#define STEPPER_CL_ENCODER_CHANNEL(inst)                                                            \
	(DT_INST_ENUM_IDX(inst, encoder_channel) == 0 ? SENSOR_CHAN_ROTATION                      \
						       : SENSOR_CHAN_ENCODER_COUNT)

#define STEPPER_CL_INIT(inst)                                                                      \
                                                                                                   \
	static const struct stepper_cl_config stepper_cl_config_##inst = {                         \
		.inner_ctrl = DEVICE_DT_GET(DT_INST_PHANDLE(inst, stepper_ctrl)),                  \
		.encoder = DEVICE_DT_GET(DT_INST_PHANDLE(inst, encoder)),                          \
		.encoder_channel = STEPPER_CL_ENCODER_CHANNEL(inst),                               \
		.encoder_counts_per_rev = DT_INST_PROP(inst, encoder_counts_per_rev),              \
		.motor_steps_per_rev = DT_INST_PROP(inst, motor_steps_per_rev),                    \
		.pid =                                                                             \
			{                                                                          \
				.kp = DT_INST_PROP(inst, pid_kp),                                  \
				.ki = DT_INST_PROP(inst, pid_ki),                                  \
				.kd = DT_INST_PROP(inst, pid_kd),                                  \
				.output_min = -(int32_t)DT_INST_PROP(inst, pid_output_max),        \
				.output_max = (int32_t)DT_INST_PROP(inst, pid_output_max),         \
				.integral_min = -(int32_t)DT_INST_PROP(inst, pid_output_max) * 10, \
				.integral_max = (int32_t)DT_INST_PROP(inst, pid_output_max) * 10,  \
			},                                                                         \
		.position_error_threshold = DT_INST_PROP(inst, position_error_threshold),          \
		.feedback_interval_us = DT_INST_PROP(inst, feedback_interval_us),                  \
		.settling_window = DT_INST_PROP(inst, settling_window),                            \
		.settling_count = DT_INST_PROP(inst, settling_count),                              \
	};                                                                                         \
                                                                                                   \
	static struct stepper_cl_data stepper_cl_data_##inst = {                                   \
		.event_common = STEPPER_CTRL_EVENT_HANDLER_DT_INST_DATA_INIT(inst),                \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, stepper_cl_init, NULL, &stepper_cl_data_##inst,                \
			      &stepper_cl_config_##inst, POST_KERNEL,                              \
			      CONFIG_STEPPER_CLOSED_LOOP_INIT_PRIORITY, &stepper_cl_api);

DT_INST_FOREACH_STATUS_OKAY(STEPPER_CL_INIT)
