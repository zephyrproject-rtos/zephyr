/*
 * Copyright (c) 2018 Miras Absar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LS013B7DH05_H
#define LS013B7DH05_H

#define LS013B7DH05_UPDATE_MODE 0b10000000
#define LS013B7DH05_INVERT_MODE 0b01000000
#define LS013B7DH05_CLEAR_MODE 0b00100000

#include <stdint.h>
#include <device.h>
#include <spi.h>
#include <display/segdl.h>

/**
 * @brief A function to clear an LS013B7DH05.
 *
 * @param dev is a valid pointer to a device that implements the
 *        SEGDL & LS013B7DH05 APIs.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*ls013b7dh05_clear_t)(struct device *dev);

/**
 * @brief A function to write a buffer to an LS013B7DH05.
 *
 * @param dev is a valid pointer to a device that implements the
 *        SEGDL & LS013B7DH05 APIs.
 *
 * @param buf is a valid pointer to a 1D array of @ref uint8_t.
 * @param buf_len is the length of @p buf.
 *
 * @returns 0 on success or a negative error number on error.
 */
typedef int (*ls013b7dh05_write_buf_t)(struct device *dev, uint8_t *buf,
				       size_t buf_len);

/**
 * @brief A struct to represent the LS013B7DH05 API implementation.
 *
 * @param clear the implementation of @ref ls013b7dh05_clear_t.
 * @param write_buf the implementation of @ref ls013b7dh05_write_buf_t.
 */
struct ls013b7dh05_extra_api {
	ls013b7dh05_clear_t clear;
	ls013b7dh05_write_buf_t write_buf;
};

/**
 * @see ls013b7dh05_clear_t
 */
static inline int ls013b7dh05_clear(struct device *dev)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	struct ls013b7dh05_extra_api *extra_api =
		(struct ls013b7dh05_extra_api *)api->extra_api;
	return extra_api->clear(dev);
}

/**
 * @see ls013b7dh05_write_buf_t
 */
static inline int ls013b7dh05_write_buf(struct device *dev, uint8_t *buf,
					size_t buf_len)
{
	struct segdl_api *api = (struct segdl_api *)dev->driver_api;
	struct ls013b7dh05_extra_api *extra_api =
		(struct ls013b7dh05_extra_api *)api->extra_api;
	return extra_api->write_buf(dev, buf, buf_len);
}

/**
 * @brief A struct to represent an LS013B7DH05 driver data.
 *
 * @param spi_dev is a valid pointer to the SPI device where an LS013B7DH05 is
 *        connected.
 *
 * @param scs_dev is a valid pointer to the GPIO device where an LS013B7DH05's
 *        chip select is connected.
 *
 * @param spi_conf is @p spi_dev's configuration.
 */
struct ls013b7dh05_data {
	struct device *spi_dev;
	struct device *scs_dev;
	struct spi_config spi_conf;
};

/**
 * @brief A function to initialize the LS013B7DH05 display driver.
 *
 * @param dev is a valid pointer to a device where the LS013B7DH05 display
 *        driver will be initialized to.

 * @returns 0 on success or a negative error number on error.
 */
static int ls013b7dh05_init(struct device *dev);

#endif
