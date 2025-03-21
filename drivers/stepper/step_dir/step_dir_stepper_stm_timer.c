/*
 * SPDX-FileCopyrightText: Copyright (c) 2025 Navimatix GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include "step_dir_stepper_stm_timer.h"
#include "zephyr/drivers/pinctrl.h"
#include <stdlib.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(step_dir_stepper_stm_timer, CONFIG_STEPPER_LOG_LEVEL);

/** Maximum number of timer channels : some stm32 soc have 6 else only 4 */
#if defined(LL_TIM_CHANNEL_CH6)
#define TIMER_HAS_6CH 1
#define TIMER_MAX_CH  6u
#else
#define TIMER_HAS_6CH 0
#define TIMER_MAX_CH  4u
#endif

/** Channel to LL mapping. */
static const uint32_t ch2ll[TIMER_MAX_CH] = {
	LL_TIM_CHANNEL_CH1, LL_TIM_CHANNEL_CH2, LL_TIM_CHANNEL_CH3, LL_TIM_CHANNEL_CH4,
#if TIMER_HAS_6CH
	LL_TIM_CHANNEL_CH5, LL_TIM_CHANNEL_CH6
#endif
};

static void stepper_trigger_callback_stm_timer(const struct device *dev, enum stepper_event event)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;

	if (!data->callback) {
		LOG_WRN_ONCE("No callback set");
		return;
	}

	if (!k_is_in_isr()) {
		data->callback(dev, event, data->event_cb_user_data);
		return;
	}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
	/* Dispatch to msgq instead of raising directly */
	int ret = k_msgq_put(&data->event_msgq, &event, K_NO_WAIT);

	if (ret != 0) {
		LOG_WRN("Failed to put event in msgq: %d", ret);
	}

	ret = k_work_submit(&data->event_callback_work);
	if (ret < 0) {
		LOG_ERR("Failed to submit work item: %d", ret);
	}
#else
	LOG_WRN_ONCE("Event callback called from ISR context without ISR safe events enabled");
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */
}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
static void stepper_work_event_handler_stm_timer(struct k_work *work)
{
	struct step_dir_stepper_stm_timer_data *data =
		CONTAINER_OF(work, struct step_dir_stepper_stm_timer_data, event_callback_work);
	enum stepper_event event;
	int ret;

	ret = k_msgq_get(&data->event_msgq, &event, K_NO_WAIT);
	if (ret != 0) {
		return;
	}

	/* Run the callback */
	if (data->callback != NULL) {
		data->callback(data->dev, event, data->event_cb_user_data);
	}

	/* If there are more pending events, resubmit this work item to handle them */
	if (k_msgq_num_used_get(&data->event_msgq) > 0) {
		k_work_submit(work);
	}
}
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */

static void step_dir_stepper_stm_timer_count_reached(const struct device *dev, void *user_data)
{
	const struct device *stepper = user_data;
	const struct step_dir_stepper_stm_timer_config *config = stepper->config;
	struct step_dir_stepper_stm_timer_data *data = stepper->data;
	int ret;

	if (data->run_mode == STEPPER_RUN_MODE_POSITION) {
		data->counter_running = false;
		ret = counter_stop(config->step_generator);
		if (ret < 0) {
			LOG_ERR("Could not stop step generator counter (%d)", ret);
		}
		stepper_trigger_callback_stm_timer(stepper, STEPPER_EVENT_STEPS_COMPLETED);
	}

	if (data->direction == STEPPER_DIRECTION_POSITIVE) {
		atomic_add(&data->actual_position, data->cfg_count.ticks);
	} else {
		atomic_sub(&data->actual_position, data->cfg_count.ticks);
	}
	LL_TIM_SetCounter(config->tim_count, 0);
}

int step_dir_stepper_stm_timer_init(const struct device *dev)
{
	const struct step_dir_stepper_stm_timer_config *config = dev->config;
	struct step_dir_stepper_stm_timer_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->dir_pin)) {
		LOG_ERR("dir pin is not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->dir_pin, GPIO_OUTPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure dir pin: %d", ret);
		return ret;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Step-Dir pinctrl setup failed (%d)", ret);
		return ret;
	}

	LL_TIM_EnableAllOutputs(config->tim_gen);

	LL_TIM_SetTriggerOutput(config->tim_gen, LL_TIM_TRGO_UPDATE);

	LL_TIM_EnableMasterSlaveMode(config->tim_count);
	LL_TIM_SetTriggerInput(config->tim_count, config->trigger_input);
	LL_TIM_SetClockSource(config->tim_count, LL_TIM_CLOCKSOURCE_EXT_MODE1);

	data->cfg_count.flags = 0;
	data->cfg_count.ticks = 100;
	data->cfg_count.callback = step_dir_stepper_stm_timer_count_reached;
	data->cfg_count.user_data = (void *)dev;

	ret = counter_set_top_value(config->step_counter, &data->cfg_count);
	if (ret < 0) {
		LOG_ERR("Could not initialize step counter (%d)", ret);
		return ret;
	}
	ret = counter_start(config->step_counter);
	if (ret < 0) {
		LOG_ERR("Could not start step counter (%d)", ret);
		return ret;
	}

#ifdef CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS
	k_msgq_init(&data->event_msgq, data->event_msgq_buffer, sizeof(enum stepper_event),
		    CONFIG_STEPPER_STEP_DIR_EVENT_QUEUE_LEN);
	k_work_init(&data->event_callback_work, stepper_work_event_handler_stm_timer);
#endif /* CONFIG_STEPPER_STEP_DIR_GENERATE_ISR_SAFE_EVENTS */

	return 0;
}

int step_dir_stepper_stm_timer_move_by(const struct device *dev, const int32_t micro_steps)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;
	const struct step_dir_stepper_stm_timer_config *config = dev->config;
	int ret;
	uint32_t pos_delta;

	if (data->microstep_interval_ns == 0) {
		LOG_ERR("Step interval not set or invalid step interval set");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		/* Stop step signal, no error handling, as counter driver always returns 0 */
		ret = counter_stop(config->step_generator);
		if (ret < 0) {
			LOG_ERR("Could not stop step generator counter (%d)", ret);
			K_SPINLOCK_BREAK;
		}

		/* Update position */
		ret = counter_get_value(config->step_counter, &pos_delta);
		if (ret < 0) {
			LOG_ERR("Could not get current position (%d)", ret);
			K_SPINLOCK_BREAK;
		}
		if (data->direction == STEPPER_DIRECTION_POSITIVE) {
			atomic_add(&data->actual_position, pos_delta);
		} else {
			atomic_sub(&data->actual_position, pos_delta);
		}
		LL_TIM_SetCounter(config->tim_count, 0);

		/* If no steps need to be taken, we are finished */
		if (micro_steps == 0) {
			data->counter_running = false;
			stepper_trigger_callback_stm_timer(dev, STEPPER_EVENT_STEPS_COMPLETED);
			ret = 0;
			K_SPINLOCK_BREAK;
		}

		/* Update Direction */
		if (micro_steps > 0) {
			ret = gpio_pin_set_dt(&config->dir_pin, 1);
			data->direction = STEPPER_DIRECTION_POSITIVE;
		} else {
			ret = gpio_pin_set_dt(&config->dir_pin, 0);
			data->direction = STEPPER_DIRECTION_NEGATIVE;
		}
		if (ret < 0) {
			LOG_ERR("Could not set direction pin (%d)", ret);
			K_SPINLOCK_BREAK;
		}

		/* Update Step Count, note that counter value gets reset automatically. */
		data->cfg_count.ticks = abs(micro_steps);
		ret = counter_set_top_value(config->step_counter, &data->cfg_count);
		if (ret < 0) {
			LOG_ERR("Could not update step counter (%d)", ret);
			K_SPINLOCK_BREAK;
		}

		data->run_mode = STEPPER_RUN_MODE_POSITION;

		/* Start step signal*/
		ret = counter_start(config->step_generator);
		if (ret < 0) {
			LOG_ERR("Could not start step generator counter (%d)", ret);
			K_SPINLOCK_BREAK;
		}
		data->counter_running = true;
	}

	return ret;
}

int step_dir_stepper_stm_timer_set_microstep_interval(const struct device *dev,
						      const uint64_t microstep_interval_ns)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;
	const struct step_dir_stepper_stm_timer_config *config = dev->config;
	int ret;
	uint32_t ticks;
	LL_TIM_OC_InitTypeDef oc_init;

	if (microstep_interval_ns == 0) {
		LOG_ERR("Step interval cannot be zero");
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->microstep_interval_ns = microstep_interval_ns;

		ticks = (uint32_t)((counter_get_frequency(config->step_generator)*microstep_interval_ns)/NSEC_PER_SEC);
		if (ticks == 0) {
			LOG_ERR("Counter ticks cannot be zero");
			ret = -EINVAL;
			K_SPINLOCK_BREAK;
		}
		data->cfg_gen.ticks = ticks;
		ret = counter_set_top_value(config->step_generator, &data->cfg_gen);
		if (ret < 0) {
			LOG_ERR("Could not update step generator counter (%d)", ret);
			K_SPINLOCK_BREAK;
		}

		LL_TIM_OC_StructInit(&oc_init);
		oc_init.OCMode = LL_TIM_OCMODE_PWM1;
		oc_init.OCState = LL_TIM_OCSTATE_ENABLE;
		oc_init.OCPolarity = LL_TIM_OCPOLARITY_HIGH;
		oc_init.CompareValue = ticks / 2;
		if (LL_TIM_OC_Init(config->tim_gen, ch2ll[config->output_channel - 1], &oc_init) !=
		    SUCCESS) {
			LOG_ERR("Could not initialize timer channel output");
			ret = -EIO;
			K_SPINLOCK_BREAK;
		}
		LL_TIM_OC_EnablePreload(config->tim_gen, 1);
	}

	return ret;
}

int step_dir_stepper_stm_timer_set_reference_position(const struct device *dev, const int32_t value)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;

	atomic_set(&data->actual_position, value);

	return 0;
}

int step_dir_stepper_stm_timer_get_actual_position(const struct device *dev, int32_t *value)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;
	const struct step_dir_stepper_stm_timer_config *config = dev->config;
	int ret = 0;
	uint32_t pos_delta;

	K_SPINLOCK(&data->lock) {
		*value = atomic_get(&data->actual_position);
		/* As data->actual_position is updated only infrequently, get difference from
		 * counter
		 */
		if (data->counter_running) {
			ret = counter_get_value(config->step_counter, &pos_delta);
			if (ret < 0) {
				LOG_ERR("Could not get step counter value (%d)", ret);
				K_SPINLOCK_BREAK;
			}
			if (data->direction == STEPPER_DIRECTION_POSITIVE) {
				*value += pos_delta;
			} else {
				*value -= pos_delta;
			}
		}
	}

	return ret;
}

int step_dir_stepper_stm_timer_move_to(const struct device *dev, const int32_t value)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;

	return step_dir_stepper_stm_timer_move_by(dev, value - atomic_get(&data->actual_position));
}

int step_dir_stepper_stm_timer_is_moving(const struct device *dev, bool *is_moving)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;
	K_SPINLOCK(&data->lock) {
		*is_moving = data->counter_running;
	}

	return 0;
}

int step_dir_stepper_stm_timer_run(const struct device *dev, const enum stepper_direction direction)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;
	const struct step_dir_stepper_stm_timer_config *config = dev->config;
	int ret;

	K_SPINLOCK(&data->lock) {
		ret = counter_stop(config->step_generator);
		if (ret < 0) {
			LOG_ERR("Could not stop step generator counter (%d)", ret);
			K_SPINLOCK_BREAK;
		}

		/* Update position */
		uint32_t pos_delta;
		ret = counter_get_value(config->step_counter, &pos_delta);
		if (ret < 0) {
			LOG_ERR("Could not get current position (%d)", ret);
			K_SPINLOCK_BREAK;
		}
		if (data->direction == STEPPER_DIRECTION_POSITIVE) {
			atomic_add(&data->actual_position, pos_delta);
		} else {
			atomic_sub(&data->actual_position, pos_delta);
		}

		/* Update Direction */
		data->direction = direction;
		if (direction == STEPPER_DIRECTION_POSITIVE) {
			ret = gpio_pin_set_dt(&config->dir_pin, 1);
		} else {
			ret = gpio_pin_set_dt(&config->dir_pin, 0);
		}
		if (ret < 0) {
			LOG_ERR("Could not set direction pin (%d)", ret);
			K_SPINLOCK_BREAK;
		}

		/* Set step count to max. The driver will only update position at that point, not
		 * stop. Note that reaching that point causes integer over/underflow, but that is an
		 * api limitation.
		 */
		data->cfg_count.ticks = UINT32_MAX;
		ret = counter_set_top_value(config->step_counter, &data->cfg_count);
		if (ret < 0) {
			LOG_ERR("Could not update step counter (%d)", ret);
			K_SPINLOCK_BREAK;
		}

		data->run_mode = STEPPER_RUN_MODE_VELOCITY;

		/* Start step signal*/
		counter_start(config->step_generator);
		if (ret < 0) {
			LOG_ERR("Could not start step generator counter (%d)", ret);
			K_SPINLOCK_BREAK;
		}
		data->counter_running = true;
	}

	return ret;
}

int step_dir_stepper_stm_timer_set_event_callback(const struct device *dev,
						  stepper_event_callback_t callback,
						  void *user_data)
{
	struct step_dir_stepper_stm_timer_data *data = dev->data;

	data->callback = callback;
	data->event_cb_user_data = user_data;

	return 0;
}

int step_dir_stepper_stm_timer_stop(const struct device *dev)
{
	const struct step_dir_stepper_stm_timer_config *config = dev->config;
	struct step_dir_stepper_stm_timer_data *data = dev->data;
	int ret;

	K_SPINLOCK(&data->lock) {
		ret = counter_stop(config->step_generator);
		if (ret < 0) {
			LOG_ERR("Could not stop step generator counter (%d)", ret);
			K_SPINLOCK_BREAK;
		}
		if (data->counter_running) {
			uint32_t pos_delta;
			ret = counter_get_value(config->step_counter, &pos_delta);
			if (ret < 0) {
				LOG_ERR("Could not get current position (%d)", ret);
				K_SPINLOCK_BREAK;
			}
			if (data->direction == STEPPER_DIRECTION_POSITIVE) {
				atomic_add(&data->actual_position, pos_delta);
			} else {
				atomic_sub(&data->actual_position, pos_delta);
			}
		}
		LL_TIM_SetCounter(config->tim_count, 0);

		data->counter_running = false;
	}

	return ret;
}
