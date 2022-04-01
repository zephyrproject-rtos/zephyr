/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "sized_events.h"

APP_EVENT_TYPE_DEFINE(test_size1_event, NULL, NULL, APP_EVENT_FLAGS_CREATE());
APP_EVENT_TYPE_DEFINE(test_size2_event, NULL, NULL, APP_EVENT_FLAGS_CREATE());
APP_EVENT_TYPE_DEFINE(test_size3_event, NULL, NULL, APP_EVENT_FLAGS_CREATE());
APP_EVENT_TYPE_DEFINE(test_size_big_event, NULL, NULL, APP_EVENT_FLAGS_CREATE());

APP_EVENT_TYPE_DEFINE(test_dynamic_event, NULL, NULL, APP_EVENT_FLAGS_CREATE());
APP_EVENT_TYPE_DEFINE(test_dynamic_with_data_event,
			      NULL,
			      NULL,
			      APP_EVENT_FLAGS_CREATE());
