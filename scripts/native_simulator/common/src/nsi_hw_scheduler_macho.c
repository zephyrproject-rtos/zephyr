/*
 * Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef __APPLE__

#include "nsi_hw_scheduler_backend.h"

unsigned int nsi_hws_backend_event_count;

void nsi_hws_backend_init(void)
{
	nsi_hws_backend_event_count =
		(unsigned int)(__nsi_hw_events_end - __nsi_hw_events_start);
}

#endif /* __APPLE__ */
