/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_FRAMEBUFFER_H_
#define ZEPHYR_DRIVERS_DISPLAY_FRAMEBUFFER_H_

#include <zephyr/drivers/display.h>

/* Display drivers that are using framebuffer helper uses 4 bytes per pixel. */
#define DISPLAY_FB_BYTES_PER_PIXEL 4U

/**
 * @brief Framebuffer configuration, shared across framebuffer-backed display drivers.
 */
struct display_fb_common_config {
	/** Firmware/mailbox device used for framebuffer allocation queries. */
	const struct device *fw_dev;
	/** Data buffer row width, in pixels. */
	uint16_t width;
	/** Data buffer column height, in pixels. */
	uint16_t height;
};

/**
 * @brief Framebuffer data, shared across framebuffer-backed display drivers.
 */
struct display_fb_common_data {
	/** Framebuffer base address (CPU bus address). */
	mem_addr_t fb_addr;
	/** Pixel stride: distance between consecutive rows, in pixels. */
	uint32_t pitch;
};

/**
 * @brief Initializer for display_fb_common_config from devicetree.
 *
 * @param inst Devicetree instance number.
 */
#define DISPLAY_FB_COMMON_CFG_FROM_DT_INST(inst)                                                   \
	{                                                                                          \
		.fw_dev = COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, firmware),                       \
					(DEVICE_DT_GET(DT_INST_PHANDLE(inst, firmware))), (NULL)), \
		.width = DT_INST_PROP(inst, width),                                                \
		.height = DT_INST_PROP(inst, height),                                              \
	}

/**
 * @brief Read data from framebuffer device
 *
 * @param dev Pointer to device structure
 * @param x x Coordinate of the upper left corner where to read from the buffer
 * @param y y Coordinate of the upper left corner where to read from the buffer
 * @param desc Pointer to a structure describing the buffer layout
 * @param buf Pointer to buffer array
 *
 * @return 0 on success else negative errno code.
 */
int display_fb_read(const struct device *dev, uint16_t x, uint16_t y,
		    const struct display_buffer_descriptor *desc, void *buf);

/**
 * @brief Write data to framebuffer device
 *
 * @param dev Pointer to device structure
 * @param x x Coordinate of the upper left corner where to write the buffer
 * @param y y Coordinate of the upper left corner where to write the buffer
 * @param desc Pointer to a structure describing the buffer layout
 * @param buf Pointer to buffer array
 *
 * @return 0 on success else negative errno code.
 */
int display_fb_write(const struct device *dev, uint16_t x, uint16_t y,
		     const struct display_buffer_descriptor *desc, const void *buf);

#endif /* ZEPHYR_DRIVERS_DISPLAY_FRAMEBUFFER_H_ */
