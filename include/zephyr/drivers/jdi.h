/*
 * Copyright (c) 2026 Qingsong Gou <v-gouqingsong@xiaomi.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup jdi_interface
 * @brief Main header file for JDI (Japan Display Inc.) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_JDI_H_
#define ZEPHYR_INCLUDE_DRIVERS_JDI_H_

/**
 * @brief Interfaces for JDI display.
 * @defgroup jdi_interface JDI Display
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 */

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief JDI display configuration parameters.
 *
 * This structure holds the configuration for a JDI (Japan Display Inc.)
 * display panel, including display dimensions, signal timing, and polarity
 * settings for control signals.
 */
struct jdi_config {
	/** Column bank head padding in pixels (left side). */
	uint16_t bank_col_head;
	/** Number of valid/visible columns in pixels. */
	uint16_t valid_columns;
	/** Column bank tail padding in pixels (right side). */
	uint16_t bank_col_tail;

	/** Row bank head padding in pixels (top side). */
	uint16_t bank_row_head;
	/** Number of valid/visible rows in pixels. */
	uint16_t valid_rows;
	/** Row bank tail padding in pixels (bottom side). */
	uint16_t bank_row_tail;

	/** ENB (Enable) active start column number. */
	uint16_t enb_start_col;
	/** ENB (Enable) active end column number. */
	uint16_t enb_end_col;

	/** Enable signal polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t enb_pol_invert : 1;
	/** HCK (Horizontal Clock) polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t hck_pol_invert : 1;
	/** HST (Horizontal Start) polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t hst_pol_invert : 1;
	/** VCK (Vertical Clock) polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t vck_pol_invert : 1;
	/** VST (Vertical Start) polarity invert. 1 = low active, 0 = high active (default). */
	uint8_t vst_pol_invert : 1;
	/** Reserved bits for future use. */
	uint8_t reserved : 3;
};

/**
 * @brief JDI display bus API.
 *
 * This structure contains function pointers for the JDI display bus
 * interface. Implementations should provide concrete implementations
 * of these functions.
 */
struct jdi_bus_api {
	/** Configure the JDI display bus with display parameters. */
	int (*config)(const struct device *dev, const struct jdi_config *config);
	/** Write a command byte to the JDI display. */
	int (*write_cmd)(const struct device *dev, uint8_t cmd);
	/** Write data buffer to the JDI display. */
	int (*write_data)(const struct device *dev, uint16_t x, uint16_t y, uint16_t width,
			  uint16_t height, const uint8_t *data, size_t len, const struct jdi_config *jdi_cfg);
	/** Read data from the JDI display. */
	int (*read_data)(const struct device *dev, uint8_t *data, size_t len);
};

/**
 * @brief Configure JDI display.
 *
 * @param dev JDI display device instance.
 * @param config Display configuration parameters.
 * @return 0 on success, negative errno on failure.
 */
static inline int jdi_config(const struct device *dev, const struct jdi_config *config)
{
	const struct jdi_bus_api *api = (const struct jdi_bus_api *)dev->api;

	return api->config(dev, config);
}

/**
 * @brief Write command to JDI display.
 *
 * @param dev JDI display device instance.
 * @param cmd Command byte to write.
 * @return 0 on success, negative errno on failure.
 */
static inline int jdi_write_cmd(const struct device *dev, uint8_t cmd)
{
	const struct jdi_bus_api *api = (const struct jdi_bus_api *)dev->api;

	return api->write_cmd(dev, cmd);
}

/**
 * @brief Write data to JDI display.
 *
 * @param dev JDI display device instance.
 * @param x X coordinate of the starting pixel.
 * @param y Y coordinate of the starting pixel.
 * @param width Width of the region in pixels.
 * @param height Height of the region in pixels.
 * @param data Pointer to data buffer to write.
 * @param len Length of data in bytes.
 * @return 0 on success, negative errno on failure.
 */
static inline int jdi_write_data(const struct device *dev, uint16_t x, uint16_t y,
				 uint16_t width, uint16_t height, const uint8_t *data, size_t len, const struct jdi_config *jdi_cfg)
{
	const struct jdi_bus_api *api = (const struct jdi_bus_api *)dev->api;

	return api->write_data(dev, x, y, width, height, data, len, jdi_cfg);
}

/**
 * @brief Read data from JDI display.
 *
 * @param dev JDI display device instance.
 * @param data Pointer to buffer to store read data.
 * @param len Length of data to read in bytes.
 * @return 0 on success, negative errno on failure.
 */
static inline int jdi_read_data(const struct device *dev, uint8_t *data, size_t len)
{
	const struct jdi_bus_api *api = (const struct jdi_bus_api *)dev->api;

	return api->read_data(dev, data, len);
}

#ifdef __cplusplus
}
#endif

#endif  /* ZEPHYR_INCLUDE_DRIVERS_JDI_H_ */
