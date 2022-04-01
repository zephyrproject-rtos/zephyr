/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEASUREMENT_EVENT_H_
#define _MEASUREMENT_EVENT_H_

/**
 * @brief Measurement Event
 * @defgroup measurement_event Measurement Event
 * @{
 */

#include "event_manager/event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct measurement_event {
	struct event_header header;

	int8_t value1;
	int16_t value2;
	int32_t value3;
};

EVENT_TYPE_DECLARE(measurement_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MEASUREMENT_EVENT_H_ */
