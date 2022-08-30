/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gpio.h>

#include "ble_mesh.h"
#include "common.h"
#include "device_composition.h"
#include "state_binding.h"
#include "transition.h"

struct transition transition;

/* Function to calculate Remaining Time (Start) */

void calculate_rt(struct transition *transition)
{
	uint8_t steps, resolution;
	int32_t duration_remainder;
	int64_t now;

	if (transition->type == MOVE) {
		transition->rt = UNKNOWN_VALUE;
		return;
	}

	if (transition->just_started) {
		transition->rt = transition->tt;
	} else {
		now = k_uptime_get();
		duration_remainder = transition->total_duration -
				     (now - transition->start_timestamp);

		if (duration_remainder > 620000) {
			/* > 620 seconds -> resolution = 0b11 [10 minutes] */
			resolution = 0x03;
			steps = duration_remainder / 600000;
		} else if (duration_remainder > 62000) {
			/* > 62 seconds -> resolution = 0b10 [10 seconds] */
			resolution = 0x02;
			steps = duration_remainder / 10000;
		} else if (duration_remainder > 6200) {
			/* > 6.2 seconds -> resolution = 0b01 [1 seconds] */
			resolution = 0x01;
			steps = duration_remainder / 1000;
		} else if (duration_remainder > 0) {
			/* <= 6.2 seconds -> resolution = 0b00 [100 ms] */
			resolution = 0x00;
			steps = duration_remainder / 100;
		} else {
			resolution = 0x00;
			steps = 0x00;
		}

		transition->rt = (resolution << 6) | steps;
	}
}

/* Function to calculate Remaining Time (End) */

static bool set_transition_counter(struct transition *transition)
{
	uint8_t steps_multiplier, resolution;

	resolution = (transition->tt >> 6);
	steps_multiplier = (transition->tt & 0x3F);
	if (steps_multiplier == 0U) {
		return false;
	}

	switch (resolution) {
	case 0: /* 100ms */
		transition->total_duration = steps_multiplier * 100U;
		break;
	case 1: /* 1 second */
		transition->total_duration = steps_multiplier * 1000U;
		break;
	case 2: /* 10 seconds */
		transition->total_duration = steps_multiplier * 10000U;
		break;
	case 3: /* 10 minutes */
		transition->total_duration = steps_multiplier * 600000U;
		break;
	}

	transition->counter = ((float) transition->total_duration / 100);

	if (transition->counter > DEVICE_SPECIFIC_RESOLUTION) {
		transition->counter = DEVICE_SPECIFIC_RESOLUTION;
	}

	return true;
}

void set_transition_values(uint8_t type)
{
	if (!set_transition_counter(ctl->transition)) {
		return;
	}

	if (ctl->transition->type == MOVE) {
		ctl->transition->quo_tt = ctl->transition->total_duration;
		return;
	}

	ctl->transition->quo_tt = ctl->transition->total_duration /
				  ctl->transition->counter;

	switch (type) {
	case ONOFF:
	case LEVEL_LIGHT:
	case ACTUAL:
	case LINEAR:
		ctl->light->delta =
			((float) (ctl->light->current - ctl->light->target) /
			 ctl->transition->counter);
		break;
	case CTL_LIGHT:
		ctl->light->delta =
			((float) (ctl->light->current - ctl->light->target) /
			 ctl->transition->counter);

		ctl->temp->delta =
			((float) (ctl->temp->current - ctl->temp->target) /
			 ctl->transition->counter);

		ctl->duv->delta =
			((float) (ctl->duv->current - ctl->duv->target) /
			 ctl->transition->counter);
		break;
	case LEVEL_TEMP:
		ctl->temp->delta =
			((float) (ctl->temp->current - ctl->temp->target) /
			 ctl->transition->counter);
		break;
	case CTL_TEMP:
		ctl->temp->delta =
			((float) (ctl->temp->current - ctl->temp->target) /
			 ctl->transition->counter);

		ctl->duv->delta =
			((float) (ctl->duv->current - ctl->duv->target) /
			 ctl->transition->counter);
		break;
	default:
		return;
	}
}

/* Timers related handlers & threads (Start) */
static void onoff_work_handler(struct k_work *work)
{
	if (ctl->transition->just_started) {
		ctl->transition->just_started = false;

		if (ctl->transition->counter == 0U) {
			update_light_state();
			k_timer_stop(&ctl->transition->timer);
		} else {
			ctl->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	ctl->transition->counter--;
	if (ctl->transition->counter) {
		ctl->light->current -= ctl->light->delta;
		update_light_state();
	} else {
		ctl->light->current = ctl->light->target;
		update_light_state();
		k_timer_stop(&ctl->transition->timer);
	}
}

static void level_move_lightness_work_handler(void)
{
	int light;

	light = 0;

	if (ctl->light->current) {
		light = ctl->light->current + ctl->light->delta;
	} else {
		if (ctl->light->delta < 0) {
			light = UINT16_MAX + ctl->light->delta;
		} else if (ctl->light->delta > 0) {
			light = ctl->light->delta;
		}
	}

	if (light > ctl->light->range_max) {
		light = ctl->light->range_max;
	} else if (light < ctl->light->range_min) {
		light = ctl->light->range_min;
	}

	ctl->light->current = light;
	update_light_state();

	if (ctl->light->target == light) {
		ctl->transition->counter = 0;
		k_timer_stop(&ctl->transition->timer);
	}
}

static void level_lightness_work_handler(struct k_work *work)
{
	if (ctl->transition->just_started) {
		ctl->transition->just_started = false;

		if (ctl->transition->counter == 0U) {
			update_light_state();
			k_timer_stop(&ctl->transition->timer);
		} else {
			ctl->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	if (ctl->transition->type == MOVE) {
		level_move_lightness_work_handler();
		return;
	}

	ctl->transition->counter--;
	if (ctl->transition->counter) {
		ctl->light->current -= ctl->light->delta;
		update_light_state();
	} else {
		ctl->light->current = ctl->light->target;
		update_light_state();
		k_timer_stop(&ctl->transition->timer);
	}
}

static void level_move_temp_work_handler(void)
{
	int temp;

	temp = 0;

	if (ctl->temp->current) {
		temp = ctl->temp->current + ctl->temp->delta;
	} else {
		if (ctl->temp->delta < 0) {
			temp = UINT16_MAX + ctl->temp->delta;
		} else if (ctl->temp->delta > 0) {
			temp = ctl->temp->delta;
		}
	}

	if (temp > ctl->temp->range_max) {
		temp = ctl->temp->range_max;
	} else if (temp < ctl->temp->range_min) {
		temp = ctl->temp->range_min;
	}

	ctl->temp->current = temp;
	update_light_state();

	if (ctl->temp->target == temp) {
		ctl->transition->counter = 0;
		k_timer_stop(&ctl->transition->timer);
	}
}

static void level_temp_work_handler(struct k_work *work)
{
	if (ctl->transition->just_started) {
		ctl->transition->just_started = false;

		if (ctl->transition->counter == 0U) {
			update_light_state();
			k_timer_stop(&ctl->transition->timer);
		} else {
			ctl->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	if (ctl->transition->type == MOVE) {
		level_move_temp_work_handler();
		return;
	}

	ctl->transition->counter--;
	if (ctl->transition->counter) {
		ctl->temp->current -= ctl->temp->delta;
		update_light_state();
	} else {
		ctl->temp->current = ctl->temp->target;
		update_light_state();
		k_timer_stop(&ctl->transition->timer);
	}
}

static void light_lightness_actual_work_handler(struct k_work *work)
{
	if (ctl->transition->just_started) {
		ctl->transition->just_started = false;

		if (ctl->transition->counter == 0U) {
			update_light_state();
			k_timer_stop(&ctl->transition->timer);
		} else {
			ctl->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	ctl->transition->counter--;
	if (ctl->transition->counter) {
		ctl->light->current -= ctl->light->delta;
		update_light_state();
	} else {
		ctl->light->current = ctl->light->target;
		update_light_state();
		k_timer_stop(&ctl->transition->timer);
	}
}

static void light_lightness_linear_work_handler(struct k_work *work)
{
	if (ctl->transition->just_started) {
		ctl->transition->just_started = false;

		if (ctl->transition->counter == 0U) {
			update_light_state();
			k_timer_stop(&ctl->transition->timer);
		} else {
			ctl->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	ctl->transition->counter--;
	if (ctl->transition->counter) {
		ctl->light->current -= ctl->light->delta;
		update_light_state();
	} else {
		ctl->light->current = ctl->light->target;
		update_light_state();
		k_timer_stop(&ctl->transition->timer);
	}
}

static void light_ctl_work_handler(struct k_work *work)
{
	if (ctl->transition->just_started) {
		ctl->transition->just_started = false;

		if (ctl->transition->counter == 0U) {
			update_light_state();
			k_timer_stop(&ctl->transition->timer);
		} else {
			ctl->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	ctl->transition->counter--;
	if (ctl->transition->counter) {
		/* Lightness */
		ctl->light->current -= ctl->light->delta;
		/* Temperature */
		ctl->temp->current -= ctl->temp->delta;
		/* Delta_UV */
		ctl->duv->current -= ctl->duv->delta;
		update_light_state();
	} else {
		ctl->light->current = ctl->light->target;
		ctl->temp->current = ctl->temp->target;
		ctl->duv->current = ctl->duv->target;
		update_light_state();
		k_timer_stop(&ctl->transition->timer);
	}
}

static void light_ctl_temp_work_handler(struct k_work *work)
{
	if (ctl->transition->just_started) {
		ctl->transition->just_started = false;

		if (ctl->transition->counter == 0U) {
			update_light_state();
			k_timer_stop(&ctl->transition->timer);
		} else {
			ctl->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	ctl->transition->counter--;
	if (ctl->transition->counter) {
		/* Temperature */
		ctl->temp->current -= ctl->temp->delta;
		/* Delta_UV */
		ctl->duv->current -= ctl->duv->delta;
		update_light_state();
	} else {
		ctl->temp->current = ctl->temp->target;
		ctl->duv->current = ctl->duv->target;
		update_light_state();
		k_timer_stop(&ctl->transition->timer);
	}
}


K_WORK_DEFINE(onoff_work, onoff_work_handler);
K_WORK_DEFINE(level_lightness_work, level_lightness_work_handler);
K_WORK_DEFINE(level_temp_work, level_temp_work_handler);
K_WORK_DEFINE(light_lightness_actual_work, light_lightness_actual_work_handler);
K_WORK_DEFINE(light_lightness_linear_work, light_lightness_linear_work_handler);
K_WORK_DEFINE(light_ctl_work, light_ctl_work_handler);
K_WORK_DEFINE(light_ctl_temp_work, light_ctl_temp_work_handler);

static void onoff_tt_handler(struct k_timer *dummy)
{
	k_work_submit(&onoff_work);
}

static void level_lightness_tt_handler(struct k_timer *dummy)
{
	k_work_submit(&level_lightness_work);
}

static void level_temp_tt_handler(struct k_timer *dummy)
{
	k_work_submit(&level_temp_work);
}

static void light_lightness_actual_tt_handler(struct k_timer *dummy)
{
	k_work_submit(&light_lightness_actual_work);
}

static void light_lightness_linear_tt_handler(struct k_timer *dummy)
{
	k_work_submit(&light_lightness_linear_work);
}

static void light_ctl_tt_handler(struct k_timer *dummy)
{
	k_work_submit(&light_ctl_work);
}

static void light_ctl_temp_tt_handler(struct k_timer *dummy)
{
	k_work_submit(&light_ctl_temp_work);
}
/* Timers related handlers & threads (End) */

K_TIMER_DEFINE(dummy_timer, NULL, NULL);

/* Messages handlers (Start) */
void onoff_handler(void)
{
	if (ctl->transition->counter == 0U && ctl->transition->delay == 0) {
		update_light_state();
		return;
	}

	k_timer_init(&ctl->transition->timer, onoff_tt_handler, NULL);

	k_timer_start(&ctl->transition->timer,
		      K_MSEC(ctl->transition->delay * 5U),
		      K_MSEC(ctl->transition->quo_tt));
}

void level_lightness_handler(void)
{
	if (ctl->transition->counter == 0U && ctl->transition->delay == 0) {
		update_light_state();
		return;
	}

	k_timer_init(&ctl->transition->timer,
		     level_lightness_tt_handler, NULL);

	k_timer_start(&ctl->transition->timer,
		      K_MSEC(ctl->transition->delay * 5U),
		      K_MSEC(ctl->transition->quo_tt));
}

void level_temp_handler(void)
{
	if (ctl->transition->counter == 0U && ctl->transition->delay == 0) {
		update_light_state();
		return;
	}

	k_timer_init(&ctl->transition->timer, level_temp_tt_handler, NULL);

	k_timer_start(&ctl->transition->timer,
		      K_MSEC(ctl->transition->delay * 5U),
		      K_MSEC(ctl->transition->quo_tt));
}

void light_lightness_actual_handler(void)
{
	if (ctl->transition->counter == 0U && ctl->transition->delay == 0) {
		update_light_state();
		return;
	}

	k_timer_init(&ctl->transition->timer,
		     light_lightness_actual_tt_handler, NULL);

	k_timer_start(&ctl->transition->timer,
		      K_MSEC(ctl->transition->delay * 5U),
		      K_MSEC(ctl->transition->quo_tt));
}

void light_lightness_linear_handler(void)
{
	if (ctl->transition->counter == 0U && ctl->transition->delay == 0) {
		update_light_state();
		return;
	}

	k_timer_init(&ctl->transition->timer,
		     light_lightness_linear_tt_handler, NULL);

	k_timer_start(&ctl->transition->timer,
		      K_MSEC(ctl->transition->delay * 5U),
		      K_MSEC(ctl->transition->quo_tt));
}

void light_ctl_handler(void)
{
	if (ctl->transition->counter == 0U && ctl->transition->delay == 0) {
		update_light_state();
		return;
	}

	k_timer_init(&ctl->transition->timer, light_ctl_tt_handler, NULL);

	k_timer_start(&ctl->transition->timer,
		      K_MSEC(ctl->transition->delay * 5U),
		      K_MSEC(ctl->transition->quo_tt));
}

void light_ctl_temp_handler(void)
{
	if (ctl->transition->counter == 0U && ctl->transition->delay == 0) {
		update_light_state();
		return;
	}

	k_timer_init(&ctl->transition->timer,
		     light_ctl_temp_tt_handler, NULL);

	k_timer_start(&ctl->transition->timer,
		      K_MSEC(ctl->transition->delay * 5U),
		      K_MSEC(ctl->transition->quo_tt));
}
/* Messages handlers (End) */
