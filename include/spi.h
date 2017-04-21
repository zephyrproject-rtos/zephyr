/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SPI drivers and applications
 */

#if defined(CONFIG_SPI_LEGACY_API)

/*
 * This is the default, and will be the way until the new API below
 * will be enforced everywhere.
 */
#include <spi_legacy.h>

#else

#ifndef __SPI_H__
#define __SPI_H__

/**
 * @brief SPI Interface
 * @defgroup spi_interface SPI Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**x
 * @brief SPI operational mode
 */
#define SPI_OP_MODE_MASTER	0
#define SPI_OP_MODE_SLAVE	1

/**
 * @brief SPI Polarity & Phase Modes
 */

/**
 * Clock Polarity: if set, clock idle state will be 1
 * and active state will be 0. If untouched, the inverse will be true
 * which is the default.
 */
#define SPI_MODE_CPOL		BIT(1)

/**
 * Clock Phase: this dictates when is the data captured, and depends
 * clock's polarity. When SPI_MODE_CPOL is set and this bit as well,
 * capture will occure on low to high transition and high to low if
 * this bit is not set (default). This is fully reversed if CPOL is
 * not set.
 */
#define SPI_MODE_CPHA		BIT(2)

/**
 * Whatever data is transmitted is looped-back to the receiving buffer of
 * the controller. This is fully controller dependent as some may not
 * support this, and can be used for testing purposes only.
 */
#define SPI_MODE_LOOP		BIT(3)

#define SPI_MODE_MASK		(0xE)
#define SPI_MODE_GET(_mode_)			\
	((_mode_) & SPI_MODE_MASK)

/**
 * @brief SPI Transfer modes (host controller dependent)
 */
#define SPI_TRANSFER_MSB	(0)
#define SPI_TRANSFER_LSB	BIT(4)

/**
 * @brief SPI word size
 */
#define SPI_WORD_SIZE_SHIFT	(5)
#define SPI_WORD_SIZE_MASK	(0x3F << SPI_WORD_SIZE_SHIFT)
#define SPI_WORD_SIZE_GET(_operation_)					\
	(((_operation_) & SPI_WORD_SIZE_MASK) >> SPI_WORD_SIZE_SHIFT)

#define SPI_WORD_SET(_word_size_)		\
	((_word_size_) << SPI_WORD_SIZE_SHIFT)

/**
 * @brief SPI MISO lines
 *
 * Some controllers support dual or quad MISO lines connected to slaves.
 * Default is single, which is the case most of the time.
 */
#define SPI_LINES_SINGLE	(0)
#define SPI_LINES_DUAL		BIT(11)
#define SPI_LINES_QUAD		BIT(12)

#define SPI_LINES_MASK		(0x3 << 11)

/**
 * @brief SPI Chip Select control structure
 *
 * This can be used to control a CS line via a GPIO line, instead of
 * using the controller inner CS logic.
 *
 * gpio_dev is a valid pointer to an actual GPIO device
 * gpio_pin is a number representing the gpio PIN that will be used
 *    to act as a CS line
 * delay is a delay in microseconds to wait before starting the
 *    transmission and before releasing the CS line
 */
struct spi_cs_control {
	struct device	*gpio_dev;
	u32_t		gpio_pin;
	u32_t		delay;
};

/**
 * @brief SPI controller configuration structure
 *
 * frequency is the bus frequency in Hertz
 * operation is a bit field with the following parts:
 *    operational mode    [ 0 ]       - master or slave.
 *    mode                [ 1 : 3 ]   - Polarity, phase and loop mode.
 *    transfer            [ 4 ]       - LSB or MSB first.
 *    word_size           [ 5 : 10 ]  - Size of a data frame in bits.
 *    lines               [ 11 : 12 ] - MISO lines: Single/Dual/Quad.
 *    RESERVED            [ 13 : 15 ] - Unused yet
 *
 * slave is the slave number from 0 to host constoller slave limit.
 *
 * cs is a valid pointer on a struct spi_cs_control is CS line is
 *    emulated through a gpio line, or NULL otherwise.
 */
struct spi_config {
	u32_t		frequency;
	u16_t		operation;
	u16_t		slave;
	struct spi_cs_control *cs;
};

/**
 * @brief SPI buffer structure
 *
 * buf is a valid pointer on a data buffer, or NULL otherwise.
 * len is the length of the buffer or, if buf is NULL, will be the
 *     length which as to be sent as dummy bytes (as TX buffer) or
 *     the length of bytes that should be skipped (as RX buffer).
 */
struct spi_buf {
	void *buf;
	size_t len;
};

/**
 * @typedef spi_api_io
 * @brief Callback API for I/O
 * See spi_transceive() for argument descriptions
 */
typedef int (*spi_api_io)(struct device	*dev,
			  struct spi_config *config,
			  const struct spi_buf **tx_bufs,
			  struct spi_buf **rx_bufs);

/**
 * @brief SPI driver API
 * This is the mandatory API any SPI driver needs to expose.
 */
struct spi_driver_api {
	spi_api_io transceive;
};

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param dev is a valid pointer to an actual SPI device
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs NULL terminated buffer array where data to be sent
 *        originates from, or NULL if none.
 * @param rx_bufs NULL terminated buffer array where data to be read
 *        will be written to, or NULL if none.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int spi_transceive(struct device *dev,
				 struct spi_config *config,
				 const struct spi_buf **tx_bufs,
				 struct spi_buf **rx_bufs)
{
	const struct spi_driver_api *api = dev->driver_api;

	return api->transceive(dev, config, tx_bufs, rx_bufs);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param dev is a valid pointer to an actual SPI device
 * @param config Pointer to a valid spi_config structure instance.
 * @param rx_bufs NULL terminated buffer array where data to be read
 *        will be written to.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int spi_read(struct device *dev,
			   struct spi_config *config,
			   struct spi_buf **rx_bufs)
{
	const struct spi_driver_api *api = dev->driver_api;

	return api->transceive(dev, config, NULL, rx_bufs);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param dev is a valid pointer to an actual SPI device
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs NULL terminated buffer array where data to be sent
 *        originates from.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int spi_write(struct device *dev,
			    struct spi_config *config,
			    const struct spi_buf **tx_bufs)
{
	const struct spi_driver_api *api = dev->driver_api;

	return api->transceive(dev, config, tx_bufs, NULL);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __SPI_H__ */

#endif /* CONFIG_SPI_LEGACY_API */
