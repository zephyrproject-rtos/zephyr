/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MULTICONTEXT_EVENT_H_
#define _MULTICONTEXT_EVENT_H_

/**
 * @brief Multicontext Event
 * @defgroup multicontext_event Multicontext Event
 * @{
 */

#include <app_event_manager/app_event_manager.h>

#ifdef __cplusplus
extern "C" {
#endif

struct multicontext_event {
	struct app_event_header header;

	int val1;
	int val2;
};

APP_EVENT_TYPE_DECLARE(multicontext_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _MULTICONTEXT_EVENT_H_ */
