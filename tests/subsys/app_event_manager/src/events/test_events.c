/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_events.h"


EVENT_TYPE_DEFINE(test_start_event,
		  false,
		  NULL,
		  NULL);

EVENT_TYPE_DEFINE(test_end_event,
		  false,
		  NULL,
		  NULL);
