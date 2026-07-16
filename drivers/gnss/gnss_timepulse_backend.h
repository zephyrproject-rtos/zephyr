/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_TIMEPULSE_BACKEND_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_TIMEPULSE_BACKEND_H_

#include <zephyr/device.h>
#include <zephyr/sys/iterable_sections.h>

#include "gnss_timepulse.h"

struct gnss_timepulse_source {
	const struct device *dev;
	int (*attach)(const struct gnss_timepulse_source *src, struct gnss_timepulse *tp);
	void *data;
};

#define GNSS_TIMEPULSE_SOURCE_DEFINE(name, _dev, _attach, _data)                      \
	static const STRUCT_SECTION_ITERABLE(gnss_timepulse_source, name) = {         \
		.dev = _dev,                                                           \
		.attach = _attach,                                                     \
		.data = _data,                                                         \
	}

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_TIMEPULSE_BACKEND_H_ */
