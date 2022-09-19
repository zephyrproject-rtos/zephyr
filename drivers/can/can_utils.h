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

/**
 * @brief Check if a CAN filter matches a CAN frame
 *
 * @param frame  CAN frame.
 * @param filter CAN filter.
 * @return true if the CAN filter matches the CAN frame, false otherwise
 */
static inline bool can_utils_filter_match(const struct can_frame *frame,
					  struct can_filter *filter)
{
	if (frame->id_type != filter->id_type) {
		return false;
	}

	if ((frame->rtr ^ filter->rtr) & filter->rtr_mask) {
		return false;
	}

	if ((frame->id ^ filter->id) & filter->id_mask) {
		return false;
	}

	return true;
}

#endif /* ZEPHYR_DRIVERS_CAN_CAN_UTILS_H_ */
