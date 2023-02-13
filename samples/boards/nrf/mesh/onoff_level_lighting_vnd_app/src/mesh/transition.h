/* Bluetooth: Mesh Generic OnOff, Generic Level, Lighting & Vendor Models
 *
 * Copyright (c) 2018 Vikrant More
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TRANSITION_H
#define _TRANSITION_H

#include "device_composition.h"

#define UNKNOWN_VALUE 0x3F
#define DEVICE_SPECIFIC_RESOLUTION 10

enum transition_types {
	MOVE = 0x01,
	NON_MOVE
};

struct transition {
	bool just_started;
	uint8_t type;
	uint8_t tt;
	uint8_t rt;
	uint8_t delay;
	uint32_t quo_tt;
	uint32_t counter;
	uint32_t total_duration;
	int64_t start_timestamp;

	struct k_timer timer;
};

extern struct transition transition;

extern struct k_timer dummy_timer;

void calculate_rt(struct transition *transition);
void set_transition_values(uint8_t type);

void onoff_handler(void);
void level_lightness_handler(void);
void level_temp_handler(void);
void light_lightness_actual_handler(void);
void light_lightness_linear_handler(void);
void light_ctl_handler(void);
void light_ctl_temp_handler(void);

#endif
