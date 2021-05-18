/*
 * Copyright (c) 2021 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENV_DISPLAY_H_
#define ENV_DISPLAY_H_

void init_display(void);
void reset_display(void);
void update_display(struct sensor_value temp,
		struct sensor_value hum,
		struct sensor_value press,
		struct sensor_value gas);

#endif
