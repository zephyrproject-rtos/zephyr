/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRANSITION_H
#define _TRANSITION_H

#define DEVICE_SPECIFIC_RESOLUTION 10

enum transition_time {
	DISABLE_TRANSITION = 0x00,
	ONOFF_TT = 0x01,
	LEVEL_TT,
	LEVEL_TT_DELTA,
	LEVEL_TT_MOVE,
	LIGHT_LIGHTNESS_ACTUAL_TT,
	LIGHT_LIGHTNESS_LINEAR_TT,
	LIGHT_CTL_TT,
	LEVEL_TEMP_TT,
	LEVEL_TEMP_TT_DELTA,
	LEVEL_TEMP_TT_MOVE,
	LIGHT_CTL_TEMP_TT
};

extern u8_t enable_transition, default_tt;
extern u32_t *ptr_tt_counter;
extern s64_t transition_start_timestamp;

struct k_timer onoff_transition_timer;
struct k_timer level_lightness_transition_timer;
struct k_timer level_temp_transition_timer;
struct k_timer light_lightness_actual_transition_timer;
struct k_timer light_lightness_linear_transition_timer;
struct k_timer light_ctl_transition_timer;
struct k_timer light_ctl_temp_transition_timer;

u8_t calculate_rt(s32_t duration_remainder);

void onoff_calculate_rt(struct generic_onoff_state *state);
void level_calculate_rt(struct generic_level_state *state);
void light_lightness_actual_calculate_rt(struct light_lightness_state *state);
void light_lightness_linear_calculate_rt(struct light_lightness_state *state);
void light_ctl_calculate_rt(struct light_ctl_state *state);
void light_ctl_temp_calculate_rt(struct light_ctl_state *state);

void onoff_tt_values(struct generic_onoff_state *state);
void level_tt_values(struct generic_level_state *state);
void light_lightness_actual_tt_values(struct light_lightness_state *state);
void light_lightness_linear_tt_values(struct light_lightness_state *state);
void light_ctl_tt_values(struct light_ctl_state *state);
void light_ctl_temp_tt_values(struct light_ctl_state *state);

void onoff_handler(struct generic_onoff_state *state);
void level_lightness_handler(struct generic_level_state *state);
void level_temp_handler(struct generic_level_state *state);
void light_lightness_actual_handler(struct light_lightness_state *state);
void light_lightness_linear_handler(struct light_lightness_state *state);
void light_ctl_handler(struct light_ctl_state *state);
void light_ctl_temp_handler(struct light_ctl_state *state);

#endif
