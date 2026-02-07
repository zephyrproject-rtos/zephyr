/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/logging/log.h>

#include "mp_bus.h"
#include "mp_element.h"
#include "mp_pipeline.h"

LOG_MODULE_REGISTER(mp_pipeline, CONFIG_LIBMP_LOG_LEVEL);

static int mp_pipeline_set_property(struct mp_object *obj, uint32_t id, const void *val)
{
	return 0;
}

static int mp_pipeline_get_property(struct mp_object *obj, uint32_t id, void *val)
{
	return 0;
}

static enum mp_state_change_return mp_pipeline_change_state(struct mp_element *element,
							    enum mp_state_change transition)
{
	return mp_bin_change_state_func(element, transition);
}

void mp_pipeline_init(struct mp_element *self)
{
	/* Init base class */
	mp_bin_init(self);

	self->object.set_property = mp_pipeline_set_property;
	self->object.get_property = mp_pipeline_get_property;
	self->change_state = mp_pipeline_change_state;
}
