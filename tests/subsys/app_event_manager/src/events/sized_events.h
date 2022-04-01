/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SIZED_EVENTS_H_
#define _SIZED_EVENTS_H_

/**
 * @brief Events with different sizes
 * @defgroup sized_events Events used to test @ref app_event_manager_event_size
 * @{
 */

#include <app_event_manager/app_event_manager.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct test_size1_event {
	struct app_event_header header;

	uint8_t val1;
};

APP_EVENT_TYPE_DECLARE(test_size1_event);


struct test_size2_event {
	struct app_event_header header;

	uint8_t val1;
	uint8_t val2;
};

APP_EVENT_TYPE_DECLARE(test_size2_event);


struct test_size3_event {
	struct app_event_header header;

	uint8_t val1;
	uint8_t val2;
	uint8_t val3;
};

APP_EVENT_TYPE_DECLARE(test_size3_event);


struct test_size_big_event {
	struct app_event_header header;

	uint32_t array[64];
};

APP_EVENT_TYPE_DECLARE(test_size_big_event);


struct test_dynamic_event {
	struct app_event_header header;

	struct event_dyndata dyndata;
};

APP_EVENT_TYPE_DYNDATA_DECLARE(test_dynamic_event);


struct test_dynamic_with_data_event {
	struct app_event_header header;

	uint32_t val1;
	uint32_t val2;
	struct event_dyndata dyndata;
};

APP_EVENT_TYPE_DYNDATA_DECLARE(test_dynamic_with_data_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _SIZED_EVENTS_H_ */
