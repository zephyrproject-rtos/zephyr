/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <board.h>
#include <gpio.h>

#include "common.h"
#include "ble_mesh.h"
#include "device_composition.h"
#include "state_binding.h"
#include "transition.h"

u8_t enable_transition, default_tt;
u32_t *ptr_tt_counter;

/* Functions to calculate Remaining Time (Start) */
u8_t calculate_rt(s32_t duration_remainder)
{
	u8_t steps, resolution;

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

	return ((resolution << 6) | steps);
}

void onoff_calculate_rt(struct generic_onoff_state *state)
{
	s32_t duration_remainder;
	s64_t now;

	if (state->is_new_transition_start) {
		state->rt = state->tt;
	} else {
		now = k_uptime_get();
		duration_remainder =
			state->total_transition_duration -
				(now -	state->transition_start_timestamp);

		state->rt = calculate_rt(duration_remainder);
	}
}

void level_calculate_rt(struct generic_level_state *state)
{
	s32_t duration_remainder;
	s64_t now;

	if (state->is_new_transition_start) {
		state->rt = state->tt;
	} else {
		now = k_uptime_get();
		duration_remainder =
			state->total_transition_duration -
				(now -	state->transition_start_timestamp);

		state->rt = calculate_rt(duration_remainder);
	}
}

void light_lightness_actual_calculate_rt(struct light_lightness_state *state)
{
	s32_t duration_remainder;
	s64_t now;

	if (state->is_new_transition_start) {
		state->rt = state->tt;
	} else {
		now = k_uptime_get();
		duration_remainder =
			state->total_transition_duration -
				(now -	state->transition_start_timestamp);

		state->rt = calculate_rt(duration_remainder);
	}
}

void light_lightness_linear_calculate_rt(struct light_lightness_state *state)
{
	s32_t duration_remainder;
	s64_t now;

	if (state->is_new_transition_start) {
		state->rt = state->tt;
	} else {
		now = k_uptime_get();
		duration_remainder =
			state->total_transition_duration -
				(now -	state->transition_start_timestamp);

		state->rt = calculate_rt(duration_remainder);
	}
}

void light_ctl_calculate_rt(struct light_ctl_state *state)
{
	s32_t duration_remainder;
	s64_t now;

	if (state->is_new_transition_start) {
		state->rt = state->tt;
	} else {
		now = k_uptime_get();
		duration_remainder =
			state->total_transition_duration -
				(now -	state->transition_start_timestamp);

		state->rt = calculate_rt(duration_remainder);
	}
}

void light_ctl_temp_calculate_rt(struct light_ctl_state *state)
{
	s32_t duration_remainder;
	s64_t now;

	if (state->is_new_transition_start) {
		state->rt = state->tt;
	} else {
		now = k_uptime_get();
		duration_remainder =
			state->total_transition_duration -
				(now -	state->transition_start_timestamp);

		state->rt = calculate_rt(duration_remainder);
	}
}
/* Functions to calculate Remaining Time (End) */

static u32_t tt_values_calculator(u8_t *tt, u32_t *total_transition_duration)
{
	u8_t steps_multiplier, resolution;
	u32_t tt_counter;

	resolution = ((*tt) >> 6);
	steps_multiplier = (*tt) & 0x3F;

	switch (resolution) {
	case 0:	/* 100ms */
		*total_transition_duration = steps_multiplier * 100;
		break;
	case 1:	/* 1 second */
		*total_transition_duration = steps_multiplier * 1000;
		break;
	case 2:	/* 10 seconds */
		*total_transition_duration = steps_multiplier * 10000;
		break;
	case 3:	/* 10 minutes */
		*total_transition_duration = steps_multiplier * 600000;
		break;
	}

	tt_counter = ((float) *total_transition_duration / 100);

	if (tt_counter > DEVICE_SPECIFIC_RESOLUTION) {
		tt_counter = DEVICE_SPECIFIC_RESOLUTION;
	}

	return tt_counter;
}

void onoff_tt_values(struct generic_onoff_state *state)
{
	state->tt_counter =
		tt_values_calculator(&state->tt,
				     &state->total_transition_duration);
	ptr_tt_counter = &state->tt_counter;

	if (state->tt_counter == 0) {
		state->quo_tt = 0;
	} else {
		state->quo_tt = state->total_transition_duration /
					state->tt_counter;
	}
}

void level_tt_values(struct generic_level_state *state)
{
	u32_t tt_counter;

	tt_counter = tt_values_calculator(&state->tt,
					  &state->total_transition_duration);
	state->tt_counter = tt_counter;
	ptr_tt_counter = &state->tt_counter;

	if (tt_counter == 0) {
		state->quo_tt = 0;
		tt_counter = 1;
	} else {
		state->quo_tt = state->total_transition_duration / tt_counter;
	}

	state->tt_delta = ((float) (state->level - state->target_level) /
			   tt_counter);
}

void light_lightness_actual_tt_values(struct light_lightness_state *state)
{
	u32_t tt_counter;

	tt_counter = tt_values_calculator(&state->tt,
					  &state->total_transition_duration);
	state->tt_counter_actual = tt_counter;
	ptr_tt_counter = &state->tt_counter_actual;

	if (tt_counter == 0) {
		state->quo_tt = 0;
		tt_counter = 1;
	} else {
		state->quo_tt = state->total_transition_duration / tt_counter;
	}

	state->tt_delta_actual =
		((float) (state->actual - state->target_actual) /
		 tt_counter);
}

void light_lightness_linear_tt_values(struct light_lightness_state *state)
{
	u32_t tt_counter;

	tt_counter = tt_values_calculator(&state->tt,
					  &state->total_transition_duration);
	state->tt_counter_linear = tt_counter;
	ptr_tt_counter = &state->tt_counter_linear;

	if (tt_counter == 0) {
		state->quo_tt = 0;
		tt_counter = 1;
	} else {
		state->quo_tt = state->total_transition_duration / tt_counter;
	}

	state->tt_delta_linear =
		((float) (state->linear - state->target_linear) /
		 tt_counter);
}

void light_ctl_tt_values(struct light_ctl_state *state)
{
	u32_t tt_counter;

	tt_counter = tt_values_calculator(&state->tt,
					  &state->total_transition_duration);
	state->tt_counter = tt_counter;
	ptr_tt_counter = &state->tt_counter;

	if (tt_counter == 0) {
		state->quo_tt = 0;
		tt_counter = 1;
	} else {
		state->quo_tt = state->total_transition_duration / tt_counter;
	}

	state->tt_lightness_delta =
		((float) (state->lightness - state->target_lightness) /
		 tt_counter);

	state->tt_temp_delta =
		((float) (state->temp - state->target_temp) /
		 tt_counter);

	state->tt_duv_delta =
		((float) (state->delta_uv - state->target_delta_uv) /
		 tt_counter);
}

void light_ctl_temp_tt_values(struct light_ctl_state *state)
{
	u32_t tt_counter;

	tt_counter = tt_values_calculator(&state->tt,
					  &state->total_transition_duration);
	state->tt_counter_temp = tt_counter;
	ptr_tt_counter = &state->tt_counter_temp;

	if (tt_counter == 0) {
		state->quo_tt = 0;
		tt_counter = 1;
	} else {
		state->quo_tt = state->total_transition_duration / tt_counter;
	}

	state->tt_temp_delta = ((float) (state->temp - state->target_temp) /
				tt_counter);

	state->tt_duv_delta =
		((float) (state->delta_uv - state->target_delta_uv) /
		 tt_counter);
}

/* Timers related handlers & threads (Start) */
static void onoff_work_handler(struct k_work *work)
{
	struct generic_onoff_state *state = &gen_onoff_srv_root_user_data;

	if (enable_transition != ONOFF_TT) {
		k_timer_stop(&onoff_transition_timer);
		return;
	}

	if (state->is_new_transition_start) {
		state->is_new_transition_start = false;

		if (state->tt_counter == 0) {
			state_binding(ONOFF, IGNORE_TEMP);
			update_light_state();

			k_timer_stop(&onoff_transition_timer);
		} else {
			state->transition_start_timestamp = k_uptime_get();

			if (state->target_onoff == STATE_ON) {
				state->onoff = STATE_ON;

				state_binding(ONOFF, IGNORE_TEMP);
				update_light_state();
			}
		}

		return;
	}

	if (state->tt_counter != 0) {
		state->tt_counter--;
	}

	if (state->tt_counter == 0) {
		state->onoff = state->target_onoff;

		state_binding(ONOFF, IGNORE_TEMP);
		update_light_state();

		k_timer_stop(&onoff_transition_timer);
	}
}

static void level_lightness_work_handler(struct k_work *work)
{
	u8_t level;
	struct generic_level_state *state = &gen_level_srv_root_user_data;

	switch (enable_transition) {
	case LEVEL_TT:
		level = LEVEL;
		break;
	case LEVEL_TT_DELTA:
		level = DELTA_LEVEL;
		break;
	case LEVEL_TT_MOVE:
		level = LEVEL;
		break;
	default:
		k_timer_stop(&level_lightness_transition_timer);
		return;
	}

	if (state->is_new_transition_start) {
		state->is_new_transition_start = false;

		if (state->tt_counter == 0) {
			state_binding(level, IGNORE_TEMP);
			update_light_state();

			k_timer_stop(&level_lightness_transition_timer);
		} else {
			state->transition_start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->tt_counter != 0) {
		state->tt_counter--;

		state->level -= state->tt_delta;

		state_binding(level, IGNORE_TEMP);
		update_light_state();
	}

	if (state->tt_counter == 0) {
		state->level = state->target_level;

		state_binding(level, IGNORE_TEMP);
		update_light_state();

		k_timer_stop(&level_lightness_transition_timer);
	}
}

static void level_temp_work_handler(struct k_work *work)
{
	struct generic_level_state *state = &gen_level_srv_s0_user_data;

	switch (enable_transition) {
	case LEVEL_TEMP_TT:
		break;
	case LEVEL_TEMP_TT_DELTA:
		break;
	case LEVEL_TEMP_TT_MOVE:
		break;
	default:
		k_timer_stop(&level_lightness_transition_timer);
		return;
	}

	if (state->is_new_transition_start) {
		state->is_new_transition_start = false;

		if (state->tt_counter == 0) {
			state_binding(IGNORE, LEVEL_TEMP);
			update_light_state();

			k_timer_stop(&level_temp_transition_timer);
		} else {
			state->transition_start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->tt_counter != 0) {
		state->tt_counter--;

		state->level -= state->tt_delta;

		state_binding(IGNORE, LEVEL_TEMP);
		update_light_state();
	}

	if (state->tt_counter == 0) {
		state->level = state->target_level;

		state_binding(IGNORE, LEVEL_TEMP);
		update_light_state();

		k_timer_stop(&level_temp_transition_timer);
	}
}

static void light_lightness_actual_work_handler(struct k_work *work)
{
	struct light_lightness_state *state = &light_lightness_srv_user_data;

	if (enable_transition != LIGHT_LIGHTNESS_ACTUAL_TT) {
		k_timer_stop(&light_lightness_actual_transition_timer);
		return;
	}

	if (state->is_new_transition_start) {
		state->is_new_transition_start = false;

		if (state->tt_counter_actual == 0) {
			state_binding(ACTUAL, IGNORE_TEMP);
			update_light_state();

			k_timer_stop(&light_lightness_actual_transition_timer);
		} else {
			state->transition_start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->tt_counter_actual != 0) {
		state->tt_counter_actual--;

		state->actual -= state->tt_delta_actual;

		state_binding(ACTUAL, IGNORE_TEMP);
		update_light_state();
	}

	if (state->tt_counter_actual == 0) {
		state->actual = state->target_actual;

		state_binding(ACTUAL, IGNORE_TEMP);
		update_light_state();

		k_timer_stop(&light_lightness_actual_transition_timer);
	}
}

static void light_lightness_linear_work_handler(struct k_work *work)
{
	struct light_lightness_state *state = &light_lightness_srv_user_data;

	if (enable_transition != LIGHT_LIGHTNESS_LINEAR_TT) {
		k_timer_stop(&light_lightness_linear_transition_timer);
		return;
	}

	if (state->is_new_transition_start) {
		state->is_new_transition_start = false;

		if (state->tt_counter_linear == 0) {
			state_binding(LINEAR, IGNORE_TEMP);
			update_light_state();

			k_timer_stop(&light_lightness_linear_transition_timer);
		} else {
			state->transition_start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->tt_counter_linear != 0) {
		state->tt_counter_linear--;

		state->linear -= state->tt_delta_linear;

		state_binding(LINEAR, IGNORE_TEMP);
		update_light_state();
	}

	if (state->tt_counter_linear == 0) {
		state->linear = state->target_linear;

		state_binding(LINEAR, IGNORE_TEMP);
		update_light_state();

		k_timer_stop(&light_lightness_linear_transition_timer);
	}
}

static void light_ctl_work_handler(struct k_work *work)
{
	struct light_ctl_state *state = &light_ctl_srv_user_data;

	if (enable_transition != LIGHT_CTL_TT) {
		k_timer_stop(&light_ctl_transition_timer);
		return;
	}

	if (state->is_new_transition_start) {
		state->is_new_transition_start = false;

		if (state->tt_counter == 0) {
			state_binding(CTL, CTL_TEMP);
			update_light_state();

			k_timer_stop(&light_ctl_transition_timer);
		} else {
			state->transition_start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->tt_counter != 0) {
		state->tt_counter--;

		/* Lightness */
		state->lightness -= state->tt_lightness_delta;

		/* Temperature */
		state->temp -= state->tt_temp_delta;

		/* Delta_UV */
		state->delta_uv -= state->tt_duv_delta;

		state_binding(CTL, CTL_TEMP);
		update_light_state();
	}

	if (state->tt_counter == 0) {
		state->lightness = state->target_lightness;
		state->temp = state->target_temp;
		state->delta_uv = state->target_delta_uv;

		state_binding(CTL, CTL_TEMP);
		update_light_state();

		k_timer_stop(&light_ctl_transition_timer);
	}
}

static void light_ctl_temp_work_handler(struct k_work *work)
{
	struct light_ctl_state *state = &light_ctl_srv_user_data;

	if (enable_transition != LIGHT_CTL_TEMP_TT) {
		k_timer_stop(&light_ctl_temp_transition_timer);
		return;
	}

	if (state->is_new_transition_start) {
		state->is_new_transition_start = false;

		if (state->tt_counter_temp == 0) {
			state_binding(IGNORE, CTL_TEMP);
			update_light_state();

			k_timer_stop(&light_ctl_temp_transition_timer);
		} else {
			state->transition_start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->tt_counter_temp != 0) {
		state->tt_counter_temp--;

		/* Temperature */
		state->temp -= state->tt_temp_delta;

		/* Delta UV */
		state->delta_uv -= state->tt_duv_delta;

		state_binding(IGNORE, CTL_TEMP);
		update_light_state();
	}

	if (state->tt_counter_temp == 0) {
		state->temp = state->target_temp;
		state->delta_uv = state->target_delta_uv;

		state_binding(IGNORE, CTL_TEMP);
		update_light_state();

		k_timer_stop(&light_ctl_temp_transition_timer);
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

/* Messages handlers (Start) */
void onoff_handler(struct generic_onoff_state *state)
{
	enable_transition = ONOFF_TT;
	state->is_new_transition_start = true;

	k_timer_start(&onoff_transition_timer, K_MSEC(5 * state->delay),
		      K_MSEC(state->quo_tt));
}

void level_lightness_handler(struct generic_level_state *state)
{
	state->is_new_transition_start = true;

	k_timer_start(&level_lightness_transition_timer,
		      K_MSEC(5 * state->delay),
		      K_MSEC(state->quo_tt));
}

void level_temp_handler(struct generic_level_state *state)
{
	state->is_new_transition_start = true;

	k_timer_start(&level_temp_transition_timer, K_MSEC(5 * state->delay),
		      K_MSEC(state->quo_tt));
}

void light_lightness_actual_handler(struct light_lightness_state *state)
{
	enable_transition = LIGHT_LIGHTNESS_ACTUAL_TT;
	state->is_new_transition_start = true;

	k_timer_start(&light_lightness_actual_transition_timer,
		      K_MSEC(5 * state->delay),
		      K_MSEC(state->quo_tt));
}

void light_lightness_linear_handler(struct light_lightness_state *state)
{
	enable_transition = LIGHT_LIGHTNESS_LINEAR_TT;
	state->is_new_transition_start = true;

	k_timer_start(&light_lightness_linear_transition_timer,
		      K_MSEC(5 * state->delay),
		      K_MSEC(state->quo_tt));
}

void light_ctl_handler(struct light_ctl_state *state)
{
	enable_transition = LIGHT_CTL_TT;
	state->is_new_transition_start = true;

	k_timer_start(&light_ctl_transition_timer, K_MSEC(5 * state->delay),
		      K_MSEC(state->quo_tt));
}

void light_ctl_temp_handler(struct light_ctl_state *state)
{
	enable_transition = LIGHT_CTL_TEMP_TT;
	state->is_new_transition_start = true;

	k_timer_start(&light_ctl_temp_transition_timer,
		      K_MSEC(5 * state->delay),
		      K_MSEC(state->quo_tt));
}
/* Messages handlers (End) */

K_TIMER_DEFINE(onoff_transition_timer, onoff_tt_handler, NULL);

K_TIMER_DEFINE(level_lightness_transition_timer,
	       level_lightness_tt_handler, NULL);
K_TIMER_DEFINE(level_temp_transition_timer,
	       level_temp_tt_handler, NULL);

K_TIMER_DEFINE(light_lightness_actual_transition_timer,
	       light_lightness_actual_tt_handler, NULL);
K_TIMER_DEFINE(light_lightness_linear_transition_timer,
	       light_lightness_linear_tt_handler, NULL);

K_TIMER_DEFINE(light_ctl_transition_timer,
	       light_ctl_tt_handler, NULL);
K_TIMER_DEFINE(light_ctl_temp_transition_timer,
	       light_ctl_temp_tt_handler, NULL);

