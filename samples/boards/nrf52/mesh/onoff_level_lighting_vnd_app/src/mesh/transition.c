/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/gpio.h>

#include "ble_mesh.h"
#include "common.h"
#include "device_composition.h"
#include "state_binding.h"
#include "transition.h"

u8_t default_tt;
u32_t *ptr_counter;
struct k_timer *ptr_timer = &dummy_timer;

struct transition transition;

/* Function to calculate Remaining Time (Start) */

void calculate_rt(struct transition *transition)
{
	u8_t steps, resolution;
	s32_t duration_remainder;
	s64_t now;

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

static bool tt_values_calculator(struct transition *transition)
{
	u8_t steps_multiplier, resolution;

	resolution = (transition->tt >> 6);
	steps_multiplier = (transition->tt & 0x3F);
	if (steps_multiplier == 0U) {
		return false;
	}

	switch (resolution) {
	case 0:	/* 100ms */
		transition->total_duration = steps_multiplier * 100U;
		break;
	case 1:	/* 1 second */
		transition->total_duration = steps_multiplier * 1000U;
		break;
	case 2:	/* 10 seconds */
		transition->total_duration = steps_multiplier * 10000U;
		break;
	case 3:	/* 10 minutes */
		transition->total_duration = steps_multiplier * 600000U;
		break;
	}

	transition->counter = ((float) transition->total_duration / 100);

	if (transition->counter > DEVICE_SPECIFIC_RESOLUTION) {
		transition->counter = DEVICE_SPECIFIC_RESOLUTION;
	}

	ptr_counter = &transition->counter;
	return true;
}

void onoff_tt_values(struct generic_onoff_state *state)
{
	calculate_lightness_target_values(ONOFF);

	if (!tt_values_calculator(state->transition)) {
		return;
	}

	state->transition->quo_tt = state->transition->total_duration /
					state->transition->counter;

	state->tt_delta = ((float) (lightness - target_lightness) /
			   state->transition->counter);
}

void level_tt_values(struct generic_level_state *state)
{
	if (state == &gen_level_srv_root_user_data) {
		calculate_lightness_target_values(LEVEL);
	} else if (state == &gen_level_srv_s0_user_data) {
		calculate_temp_target_values(LEVEL_TEMP);
	}

	if (!tt_values_calculator(state->transition)) {
		return;
	}

	state->transition->quo_tt = state->transition->total_duration /
					state->transition->counter;

	state->tt_delta = ((float) (state->level - state->target_level) /
			   state->transition->counter);
}

void level_move_tt_values(struct generic_level_state *state)
{
	if (state == &gen_level_srv_root_user_data) {
		calculate_lightness_target_values(LEVEL);
	} else if (state == &gen_level_srv_s0_user_data) {
		calculate_temp_target_values(LEVEL_TEMP);
	}

	if (!tt_values_calculator(state->transition)) {
		return;
	}

	state->transition->quo_tt = state->transition->total_duration;

	state->tt_delta = state->last_delta;
}

void light_lightness_actual_tt_values(struct light_lightness_state *state)
{
	calculate_lightness_target_values(ACTUAL);

	if (!tt_values_calculator(state->transition)) {
		return;
	}

	state->transition->quo_tt = state->transition->total_duration /
					state->transition->counter;

	state->tt_delta_actual =
		((float) (state->actual - state->target_actual) /
		 state->transition->counter);
}

void light_lightness_linear_tt_values(struct light_lightness_state *state)
{
	calculate_lightness_target_values(LINEAR);

	if (!tt_values_calculator(state->transition)) {
		return;
	}

	state->transition->quo_tt = state->transition->total_duration /
					state->transition->counter;

	state->tt_delta_linear =
		((float) (state->linear - state->target_linear) /
		 state->transition->counter);
}

void light_ctl_tt_values(struct light_ctl_state *state)
{
	calculate_lightness_target_values(CTL);

	if (!tt_values_calculator(state->transition)) {
		return;
	}

	state->transition->quo_tt = state->transition->total_duration /
					state->transition->counter;

	state->tt_delta_lightness =
		((float) (state->lightness - state->target_lightness) /
		 state->transition->counter);

	state->tt_delta_temp =
		((float) (state->temp - state->target_temp) /
		 state->transition->counter);

	state->tt_delta_duv =
		((float) (state->delta_uv - state->target_delta_uv) /
		 state->transition->counter);
}

void light_ctl_temp_tt_values(struct light_ctl_state *state)
{
	calculate_temp_target_values(CTL_TEMP);

	if (!tt_values_calculator(state->transition)) {
		return;
	}

	state->transition->quo_tt = state->transition->total_duration /
					state->transition->counter;

	state->tt_delta_temp = ((float) (state->temp - state->target_temp) /
				state->transition->counter);

	state->tt_delta_duv =
		((float) (state->delta_uv - state->target_delta_uv) /
		 state->transition->counter);
}

/* Timers related handlers & threads (Start) */
static void onoff_work_handler(struct k_work *work)
{
	struct generic_onoff_state *state = &gen_onoff_srv_root_user_data;

	if (state->transition->just_started) {
		state->transition->just_started = false;

		if (state->transition->counter == 0U) {
			state_binding(ONOFF, IGNORE_TEMP);
			update_light_state();

			k_timer_stop(ptr_timer);
		} else {
			state->transition->start_timestamp = k_uptime_get();

			if (state->target_onoff == STATE_ON) {
				state->onoff = STATE_ON;
			}
		}

		return;
	}

	if (state->transition->counter != 0U) {
		state->transition->counter--;

		lightness -= state->tt_delta;

		state_binding(IGNORE, IGNORE_TEMP);
		update_light_state();
	}

	if (state->transition->counter == 0U) {
		state->onoff = state->target_onoff;
		lightness = target_lightness;

		state_binding(IGNORE, IGNORE_TEMP);
		update_light_state();

		k_timer_stop(ptr_timer);
	}
}

static void level_move_lightness_work_handler(void)
{
	s32_t level;
	struct generic_level_state *state = &gen_level_srv_root_user_data;

	level = state->level + state->tt_delta;
	if (level <= INT16_MIN || level >= INT16_MAX) {
		state->transition->counter = 0U;
		k_timer_stop(ptr_timer);

		if (level > 0) {
			level = INT16_MAX;
		} else if (level < 0) {
			level = INT16_MIN;
		}
	}

	state->level = level;

	state_binding(LEVEL, IGNORE_TEMP);
	update_light_state();
}

static void level_lightness_work_handler(struct k_work *work)
{
	u8_t level;
	struct generic_level_state *state = &gen_level_srv_root_user_data;

	switch (state->transition->type) {
	case LEVEL_TT:
		level = LEVEL;
		break;
	case LEVEL_TT_DELTA:
		level = DELTA_LEVEL;
		break;
	case LEVEL_TT_MOVE:
		if (state->transition->just_started) {
			state->transition->just_started = false;
			return;
		}

		level_move_lightness_work_handler();
		return;
	default:
		return;
	}

	if (state->transition->just_started) {
		state->transition->just_started = false;

		if (state->transition->counter == 0U) {
			state_binding(level, IGNORE_TEMP);
			update_light_state();

			k_timer_stop(ptr_timer);
		} else {
			state->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->transition->counter != 0U) {
		state->transition->counter--;

		state->level -= state->tt_delta;

		state_binding(level, IGNORE_TEMP);
		update_light_state();
	}

	if (state->transition->counter == 0U) {
		state->level = state->target_level;

		state_binding(level, IGNORE_TEMP);
		update_light_state();

		k_timer_stop(ptr_timer);
	}
}

static void level_move_temp_work_handler(void)
{
	s32_t level;
	struct generic_level_state *state = &gen_level_srv_s0_user_data;

	level = state->level + state->tt_delta;
	if (level <= INT16_MIN || level >= INT16_MAX) {
		state->transition->counter = 0U;
		k_timer_stop(ptr_timer);

		if (level > 0) {
			level = INT16_MAX;
		} else if (level < 0) {
			level = INT16_MIN;
		}
	}

	state->level = level;

	state_binding(IGNORE, LEVEL_TEMP);
	update_light_state();
}

static void level_temp_work_handler(struct k_work *work)
{
	struct generic_level_state *state = &gen_level_srv_s0_user_data;

	switch (state->transition->type) {
	case LEVEL_TEMP_TT:
		break;
	case LEVEL_TEMP_TT_DELTA:
		break;
	case LEVEL_TEMP_TT_MOVE:
		if (state->transition->just_started) {
			state->transition->just_started = false;
			return;
		}

		level_move_temp_work_handler();
		return;
	default:
		return;
	}

	if (state->transition->just_started) {
		state->transition->just_started = false;

		if (state->transition->counter == 0U) {
			state_binding(IGNORE, LEVEL_TEMP);
			update_light_state();

			k_timer_stop(ptr_timer);
		} else {
			state->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->transition->counter != 0U) {
		state->transition->counter--;

		state->level -= state->tt_delta;

		state_binding(IGNORE, LEVEL_TEMP);
		update_light_state();
	}

	if (state->transition->counter == 0U) {
		state->level = state->target_level;

		state_binding(IGNORE, LEVEL_TEMP);
		update_light_state();

		k_timer_stop(ptr_timer);
	}
}

static void light_lightness_actual_work_handler(struct k_work *work)
{
	struct light_lightness_state *state = &light_lightness_srv_user_data;

	if (state->transition->just_started) {
		state->transition->just_started = false;

		if (state->transition->counter == 0U) {
			state_binding(ACTUAL, IGNORE_TEMP);
			update_light_state();

			k_timer_stop(ptr_timer);
		} else {
			state->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->transition->counter != 0U) {
		state->transition->counter--;

		state->actual -= state->tt_delta_actual;

		state_binding(ACTUAL, IGNORE_TEMP);
		update_light_state();
	}

	if (state->transition->counter == 0U) {
		state->actual = state->target_actual;

		state_binding(ACTUAL, IGNORE_TEMP);
		update_light_state();

		k_timer_stop(ptr_timer);
	}
}

static void light_lightness_linear_work_handler(struct k_work *work)
{
	struct light_lightness_state *state = &light_lightness_srv_user_data;

	if (state->transition->just_started) {
		state->transition->just_started = false;

		if (state->transition->counter == 0U) {
			state_binding(LINEAR, IGNORE_TEMP);
			update_light_state();

			k_timer_stop(ptr_timer);
		} else {
			state->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->transition->counter != 0U) {
		state->transition->counter--;

		state->linear -= state->tt_delta_linear;

		state_binding(LINEAR, IGNORE_TEMP);
		update_light_state();
	}

	if (state->transition->counter == 0U) {
		state->linear = state->target_linear;

		state_binding(LINEAR, IGNORE_TEMP);
		update_light_state();

		k_timer_stop(ptr_timer);
	}
}

static void light_ctl_work_handler(struct k_work *work)
{
	struct light_ctl_state *state = &light_ctl_srv_user_data;

	if (state->transition->just_started) {
		state->transition->just_started = false;

		if (state->transition->counter == 0U) {
			state_binding(CTL, CTL_TEMP);
			update_light_state();

			k_timer_stop(ptr_timer);
		} else {
			state->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->transition->counter != 0U) {
		state->transition->counter--;

		/* Lightness */
		state->lightness -= state->tt_delta_lightness;

		/* Temperature */
		state->temp -= state->tt_delta_temp;

		/* Delta_UV */
		state->delta_uv -= state->tt_delta_duv;

		state_binding(CTL, CTL_TEMP);
		update_light_state();
	}

	if (state->transition->counter == 0U) {
		state->lightness = state->target_lightness;
		state->temp = state->target_temp;
		state->delta_uv = state->target_delta_uv;

		state_binding(CTL, CTL_TEMP);
		update_light_state();

		k_timer_stop(ptr_timer);
	}
}

static void light_ctl_temp_work_handler(struct k_work *work)
{
	struct light_ctl_state *state = &light_ctl_srv_user_data;

	if (state->transition->just_started) {
		state->transition->just_started = false;

		if (state->transition->counter == 0U) {
			state_binding(IGNORE, CTL_TEMP);
			update_light_state();

			k_timer_stop(ptr_timer);
		} else {
			state->transition->start_timestamp = k_uptime_get();
		}

		return;
	}

	if (state->transition->counter != 0U) {
		state->transition->counter--;

		/* Temperature */
		state->temp -= state->tt_delta_temp;

		/* Delta UV */
		state->delta_uv -= state->tt_delta_duv;

		state_binding(IGNORE, CTL_TEMP);
		update_light_state();
	}

	if (state->transition->counter == 0U) {
		state->temp = state->target_temp;
		state->delta_uv = state->target_delta_uv;

		state_binding(IGNORE, CTL_TEMP);
		update_light_state();

		k_timer_stop(ptr_timer);
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
void onoff_handler(struct generic_onoff_state *state)
{
	ptr_timer = &state->transition->timer;

	k_timer_init(ptr_timer, onoff_tt_handler, NULL);

	k_timer_start(ptr_timer,
		      K_MSEC(state->transition->delay * 5U),
		      K_MSEC(state->transition->quo_tt));
}

void level_lightness_handler(struct generic_level_state *state)
{
	ptr_timer = &state->transition->timer;

	k_timer_init(ptr_timer, level_lightness_tt_handler, NULL);

	k_timer_start(ptr_timer,
		      K_MSEC(state->transition->delay * 5U),
		      K_MSEC(state->transition->quo_tt));
}

void level_temp_handler(struct generic_level_state *state)
{
	ptr_timer = &state->transition->timer;

	k_timer_init(ptr_timer, level_temp_tt_handler, NULL);

	k_timer_start(ptr_timer,
		      K_MSEC(state->transition->delay * 5U),
		      K_MSEC(state->transition->quo_tt));
}

void light_lightness_actual_handler(struct light_lightness_state *state)
{
	ptr_timer = &state->transition->timer;

	k_timer_init(ptr_timer, light_lightness_actual_tt_handler, NULL);

	k_timer_start(ptr_timer,
		      K_MSEC(state->transition->delay * 5U),
		      K_MSEC(state->transition->quo_tt));
}

void light_lightness_linear_handler(struct light_lightness_state *state)
{
	ptr_timer = &state->transition->timer;

	k_timer_init(ptr_timer, light_lightness_linear_tt_handler, NULL);

	k_timer_start(ptr_timer,
		      K_MSEC(state->transition->delay * 5U),
		      K_MSEC(state->transition->quo_tt));
}

void light_ctl_handler(struct light_ctl_state *state)
{
	ptr_timer = &state->transition->timer;

	k_timer_init(ptr_timer, light_ctl_tt_handler, NULL);

	k_timer_start(ptr_timer,
		      K_MSEC(state->transition->delay * 5U),
		      K_MSEC(state->transition->quo_tt));
}

void light_ctl_temp_handler(struct light_ctl_state *state)
{
	ptr_timer = &state->transition->timer;

	k_timer_init(ptr_timer, light_ctl_temp_tt_handler, NULL);

	k_timer_start(ptr_timer,
		      K_MSEC(state->transition->delay * 5U),
		      K_MSEC(state->transition->quo_tt));
}
/* Messages handlers (End) */



