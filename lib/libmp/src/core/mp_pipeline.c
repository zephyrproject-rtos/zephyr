/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include <zephyr/logging/log.h>

#include "mp_bus.h"
#include "mp_element.h"
#include "mp_element_factory.h"
#include "mp_pipeline.h"

LOG_MODULE_REGISTER(mp_pipeline, CONFIG_LIBMP_LOG_LEVEL);

static int mp_pipeline_set_property(MpObject *obj, uint32_t id, const void *val)
{
	return 0;
}

static int mp_pipeline_get_property(MpObject *obj, uint32_t id, void *val)
{
	return 0;
}

static MpStateChangeReturn mp_pipeline_change_state(MpElement *element, MpStateChange transition)
{
	return mp_bin_change_state_func(element, transition);
}

void mp_pipeline_init(MpElement *self)
{
	/* Init base class */
	mp_bin_init(self);

	/* Init bus */
	mp_bus_init(&MP_PIPELINE(self)->bus);

	self->object.set_property = mp_pipeline_set_property;
	self->object.get_property = mp_pipeline_get_property;
	self->change_state = mp_pipeline_change_state;

	self->bus = &MP_PIPELINE(self)->bus;
}

MpElement *mp_pipeline_new(const char *name)
{
	return mp_element_factory_create("pipeline", name);
}
