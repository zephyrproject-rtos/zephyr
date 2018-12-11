/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRANSITION_H
#define _TRANSITION_H

#define UNKNOWN_VALUE 0x3F
#define DEVICE_SPECIFIC_RESOLUTION 10

enum level_transition_types {
	LEVEL_TT,
	LEVEL_TT_DELTA,
	LEVEL_TT_MOVE,

	LEVEL_TEMP_TT,
	LEVEL_TEMP_TT_DELTA,
	LEVEL_TEMP_TT_MOVE,
};

struct transition {
	bool just_started;
	u8_t tt;
	u8_t rt;
	u8_t delay;
	u32_t quo_tt;
	u32_t counter;
	u32_t total_duration;
	s64_t start_timestamp;

	struct k_timer timer;
};

extern u8_t transition_type, default_tt;
extern u32_t *ptr_counter;
extern struct k_timer *ptr_timer;

extern struct transition lightness_transition, temp_transition;

extern struct k_timer dummy_timer;

void reassign_last_target_values(void);
void calculate_rt(struct transition *transition);


void onoff_tt_values(struct generic_onoff_state *state, u8_t tt, u8_t delay);
void level_tt_values(struct generic_level_state *state, u8_t tt, u8_t delay);
void light_lightness_actual_tt_values(struct light_lightness_state *state,
				      u8_t tt, u8_t delay);
void light_lightness_linear_tt_values(struct light_lightness_state *state,
				      u8_t tt, u8_t delay);
void light_ctl_tt_values(struct light_ctl_state *state, u8_t tt, u8_t delay);
void light_ctl_temp_tt_values(struct light_ctl_state *state,
			      u8_t tt, u8_t delay);

void onoff_handler(struct generic_onoff_state *state);
void level_lightness_handler(struct generic_level_state *state);
void level_temp_handler(struct generic_level_state *state);
void light_lightness_actual_handler(struct light_lightness_state *state);
void light_lightness_linear_handler(struct light_lightness_state *state);
void light_ctl_handler(struct light_ctl_state *state);
void light_ctl_temp_handler(struct light_ctl_state *state);

#endif
