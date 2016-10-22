/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Public API for SPI drivers
 */

#ifndef __SPI_H__
#define __SPI_H__

/**
 * @brief SPI Interface
 * @defgroup spi_interface SPI Interface
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SPI Polarity & Phase Modes
 */
#define SPI_MODE_CPOL		0x1
#define SPI_MODE_CPHA		0x2
#define SPI_MODE_LOOP		0x4

#define SPI_MODE_MASK		(0x7)
#define SPI_MODE(_in_)		((_in_) & SPI_MODE_MASK)

/**
 * @brief SPI Transfer modes (host controller dependent)
 */
#define SPI_TRANSFER_MSB	(0 << 3)
#define SPI_TRANSFER_LSB	(1 << 3)

#define SPI_TRANSFER_MASK	(0x8)

#define SPI_WORD_SIZE_MASK	(0xFF << 4)
#define SPI_WORD_SIZE_GET(_in_) (((_in_) & SPI_WORD_SIZE_MASK) >> 4)
#define SPI_WORD(_in_) ((_in_) << 4)

/**
 * @brief SPI configuration structure.
 *
 * config is a bit field with the following parts:
 *    mode           [ 0 : 2 ]   - Polarity, phase and loop mode.
 *    transfer_mode  [ 3 ]       - LSB or MSB first transfer mode.
 *    word_size      [ 4 : 11 ]  - Size of a data frame in bits.
 *    RESERVED       [ 12 : 31 ] - Undefined or device-specific usage.
 *
 * max_sys_freq is the clock divider supported by the the host
 * spi controller.
 */
struct spi_config {
	uint32_t	config;
	uint32_t	max_sys_freq;
};

/**
 * @typedef spi_api_configure
 * @brief Callback API upon configuring the const controller
 * See spi_configure() for argument description
 */
typedef int (*spi_api_configure)(struct device *dev,
				 struct spi_config *config);
/**
 * @typedef spi_api_slave_select
 * @brief Callback API upon selecting a slave
 * See spi_slave_select() for argument description
 */
typedef int (*spi_api_slave_select)(struct device *dev, uint32_t slave);
/**
 * @typedef spi_api_io
 * @brief Callback API for I/O
 * See spi_read() and spi_write() for argument descriptions
 */
typedef int (*spi_api_io)(struct device *dev,
			  const void *tx_buf, uint32_t tx_buf_len,
			  void *rx_buf, uint32_t rx_buf_len);

struct spi_driver_api {
	spi_api_configure configure;
	spi_api_slave_select slave_select;
	spi_api_io transceive;
};

/**
 * @brief Configure a host controller for operating against slaves.
 * @param dev Pointer to the device structure for the driver instance.
 * @param config Pointer to the configuration provided by the application.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int spi_configure(struct device *dev,
				struct spi_config *config)
{
	const struct spi_driver_api *api = dev->driver_api;

	return api->configure(dev, config);
}

/**
 * @brief Select a slave to deal with.
 *
 * This routine is meaningful only if the controller supports per-slave
 * addressing: One SS line per-slave. If not, this routine has no effect
 * and daisy-chaining should be considered to deal with multiple slaves
 * on the same line.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param slave An integer identifying the slave. It starts from 1 which
 *		corresponds to cs0.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int spi_slave_select(struct device *dev, uint32_t slave)
{
	const struct spi_driver_api *api = dev->driver_api;

	if (!api->slave_select) {
		return 0;
	}

	return api->slave_select(dev, slave);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Memory buffer where data will be transferred.
 * @param len Size of the memory buffer available for writing.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int spi_read(struct device *dev, void *buf, uint32_t len)
{
	const struct spi_driver_api *api = dev->driver_api;

	return api->transceive(dev, NULL, 0, buf, len);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 * @param dev Pointer to the device structure for the driver instance.
 * @param buf Memory buffer from where data is transferred.
 * @param len Size of the memory buffer available for reading.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int spi_write(struct device *dev, const void *buf, uint32_t len)
{
	const struct spi_driver_api *api = dev->driver_api;

	return api->transceive(dev, buf, len, NULL, 0);
}

/**
 * @brief Read and write the specified amount of data from the SPI driver.
 *
 * This routine is meant for full-duplex transmission.
 * Only equal length is supported(tx_buf_len must be equal to rx_buf_len).
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param tx_buf Memory buffer where data originates
 * @param tx_buf_len Size of the memory buffer available for reading.
 * @param rx_buf Memory buffer where data is transferred.
 * @param rx_buf_len Size of the memory buffer available for writing.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int spi_transceive(struct device *dev,
			  const void *tx_buf, uint32_t tx_buf_len,
			  void *rx_buf, uint32_t rx_buf_len)
{
	const struct spi_driver_api *api = dev->driver_api;

	return api->transceive(dev, tx_buf, tx_buf_len, rx_buf, rx_buf_len);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __SPI_H__ */
