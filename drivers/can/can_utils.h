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
	if (((frame->flags & CAN_FRAME_IDE) != 0) && ((filter->flags & CAN_FILTER_IDE) == 0)) {
		return false;
	}

	if (((frame->flags & CAN_FRAME_RTR) == 0) && (filter->flags & CAN_FILTER_DATA) == 0) {
		return false;
	}

	if (((frame->flags & CAN_FRAME_RTR) != 0) && (filter->flags & CAN_FILTER_RTR) == 0) {
		return false;
	}

	if (((frame->flags & CAN_FRAME_FDF) != 0) && (filter->flags & CAN_FILTER_FDF) == 0) {
		return false;
	}

	if ((frame->id ^ filter->id) & filter->mask) {
		return false;
	}

	return true;
}

#endif /* ZEPHYR_DRIVERS_CAN_CAN_UTILS_H_ */
