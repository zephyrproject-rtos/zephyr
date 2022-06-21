/*
 * Copyright (c) 2022 Bjarki Arge Andreasen <bjarkix123@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file data/datetime.h
 * @brief Date and time structures
 */

#ifndef ZEPHYR_INCLUDE_DATETIME_H_
#define ZEPHYR_INCLUDE_DATETIME_H_

#include <zephyr/types.h>

/**
 * @brief Date and time
 */
struct datetime {
	uint16_t millisecond;
	uint8_t second;
	uint8_t minute;
	uint8_t hour;
	uint8_t day;
	uint16_t year;
	uint8_t month;
};

/**
 * @brief Mask for date and time
 */
union datetime_mask {
	/** The mask bitmap */
	struct {
		uint8_t millisecond : 1;
		uint8_t second      : 1;
		uint8_t minute      : 1;
		uint8_t hour        : 1;
		uint8_t day         : 1;
		uint8_t month       : 1;
		uint8_t year        : 1;
	};
	/** The mask value */
	uint8_t mask;
};

#endif /* ZEPHYR_INCLUDE_DATETIME_H_ */
