/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZIO_FT5336_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZIO_FT5336_H_

#include <zio.h>

/* channel type(s) */
#define FT5336_COORD_TYPE	(ZIO_CHAN_TYPES + 1)
#define FT5336_EVENT_TYPE	(ZIO_CHAN_TYPES + 2)
#define FT5336_ID_TYPE		(ZIO_CHAN_TYPES + 3)

/* device attributes */

/* channel attribute type(s) */

/* datum type */
struct ft5336_datum {
	u16_t x;
	u16_t y;
	enum {
		FT5336_DOWN    = 0, /*!< The state changed to touched */
		FT5336_UP      = 1, /*!< The state changed to not touched */
		FT5336_CONTACT = 2, /*!< Continuous touch detected */
	} event;
	u8_t id;
};

#endif
