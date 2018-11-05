/* si1133_trigger.c - Driver trigger for SI1133 UV index and ambient light sensor */

/*
 * Copyright (c) 2018 Thomas Li Fredriksen
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "si1133.h"

int si1133_trigger_set(struct device *dev, const struct sensor_trigger *trig, sensor_trigger_handler_t handler)
{
	/* TODO */

	return 0;
}

int si1133_init_interrupt(struct device *dev)
{
	/* TODO */
	return 0;
}
