/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _TEST_EVENTS_H_
#define _TEST_EVENTS_H_

/**
 * @brief Test Start and End Events
 * @defgroup test_events Test Start and End Events
 * @{
 */

#include <event_manager/event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

enum test_id {
	TEST_IDLE,
	TEST_BASIC,
	TEST_DATA,
	TEST_EVENT_ORDER,
	TEST_SUBSCRIBER_ORDER,
	TEST_OOM_RESET,
	TEST_MULTICONTEXT,

	TEST_CNT
};

struct test_start_event {
	struct event_header header;

	enum test_id test_id;
};

EVENT_TYPE_DECLARE(test_start_event);

struct test_end_event {
	struct event_header header;

	enum test_id test_id;
};

EVENT_TYPE_DECLARE(test_end_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _TEST_EVENTS_H_ */
