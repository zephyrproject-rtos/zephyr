/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gnss_timepulse_backend.h"

int gnss_timepulse_attach(const struct device *dev, struct gnss_timepulse *tp)
{
	const struct gnss_timepulse_source *match = NULL;

	tp->last = 0;
	tp->available = false;
	tp->valid = false;

	STRUCT_SECTION_FOREACH(gnss_timepulse_source, src) {
		if (src->dev != dev) {
			continue;
		}

		if (match != NULL) {
			return -EINVAL;
		}

		match = src;
	}

	if (match == NULL) {
		return 0;
	}

	return match->attach(match, tp);
}
