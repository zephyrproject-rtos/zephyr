/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/rc/rc.h>
#include <zephyr/sys/iterable_sections.h>

#include "rc_internal.h"

void rc_callback_call(const struct device *dev, const struct rc_event *evt)
{
	STRUCT_SECTION_FOREACH(rc_callback, cb) {
		if ((cb->dev == NULL) || (cb->dev == dev)) {
			cb->callback(dev, evt, cb->user_data);
		}
	}
}
