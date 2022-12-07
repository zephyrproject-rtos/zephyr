/*
 * Copyright (c) 2022 Andreas Sandberg
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DISPLAY_SSD16XX_H_
#define ZEPHYR_INCLUDE_DISPLAY_SSD16XX_H_

#include <zephyr/drivers/display.h>

/**
 * SSD16xx RAM type for direct RAM access
 */
enum ssd16xx_ram {
	/** The black RAM buffer. This is typically the buffer used to
	 * compose the contents that will be displayed after the next
	 * refresh.
	 */
	SSD16XX_RAM_BLACK = 0,
	/* The red RAM buffer. This is typically the old frame buffer
	 * when performing partial refreshes or an additional color
	 * channel.
	 */
	SSD16XX_RAM_RED,
};

/**
 * @brief Read data directly from the display controller's internal
 * RAM.
 *
 * @param dev Pointer to device structure
 * @param ram_type Type of RAM to read from
 * @param x Coordinate relative to the upper left corner
 * @param y Coordinate relative to the upper left corner
 * @param desc Structure describing the buffer layout
 * @param buf Output buffer
 *
 * @retval 0 on success, negative errno on failure.
 */
int ssd16xx_read_ram(const struct device *dev, enum ssd16xx_ram ram_type,
		     const uint16_t x, const uint16_t y,
		     const struct display_buffer_descriptor *desc,
		     void *buf);

#endif /* ZEPHYR_INCLUDE_DISPLAY_SSD16XX_H_ */
