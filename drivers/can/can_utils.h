/*
 * Copyright (c) 2018 Karsten Koenig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Header where utility code can be found for CAN drivers
 */

#ifndef ZEPHYR_DRIVERS_CAN_CAN_UTILS_H_
#define ZEPHYR_DRIVERS_CAN_CAN_UTILS_H_

static inline uint8_t can_utils_filter_match(const struct zcan_frame *msg,
					     struct zcan_filter *filter)
{
	if (msg->id_type != filter->id_type) {
		return 0;
	}

	if ((msg->rtr ^ filter->rtr) & filter->rtr_mask) {
		return 0;
	}

	if ((msg->id ^ filter->id) & filter->id_mask) {
		return 0;
	}

	return 1;
}

#endif /* ZEPHYR_DRIVERS_CAN_CAN_UTILS_H_ */
