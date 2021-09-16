/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "ack_event.h"


EVENT_TYPE_DEFINE(ack_event,
		  true,
		  NULL,
		  NULL);
