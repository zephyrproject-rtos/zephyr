/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "gnss_timepulse_backend.h"

TYPE_SECTION_START_EXTERN(struct gnss_timepulse_source, gnss_timepulse_source);
TYPE_SECTION_END_EXTERN(struct gnss_timepulse_source, gnss_timepulse_source);

int gnss_timepulse_attach(const struct device *dev, struct gnss_timepulse *tp)
{
	const struct gnss_timepulse_source *match = NULL;
	const struct gnss_timepulse_source *src_end = TYPE_SECTION_END(gnss_timepulse_source);

	tp->last = 0;
	tp->available = false;
	tp->valid = false;

	__ASSERT(TYPE_SECTION_START(gnss_timepulse_source) <= src_end,
		 "unexpected list end location");

	for (const struct gnss_timepulse_source *src = TYPE_SECTION_START(gnss_timepulse_source);
	     src < src_end; src++) {
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
