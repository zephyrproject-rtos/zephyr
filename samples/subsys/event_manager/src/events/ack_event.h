/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ACK_EVENT_H_
#define _ACK_EVENT_H_

/**
 * @brief ACK Event
 * @defgroup ack_event ACK Event
 * @{
 */

#include "event_manager/event_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

struct ack_event {
	struct event_header header;
};

EVENT_TYPE_DECLARE(ack_event);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _ACK_EVENT_H_ */
