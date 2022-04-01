/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "order_event.h"

APP_EVENT_TYPE_DEFINE(order_event,
		  NULL,
		  NULL,
		  APP_EVENT_FLAGS_CREATE());
