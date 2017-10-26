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

/**
 * @brief SPI operational mode
 */
#define SPI_OP_MODE_MASTER	0
#define SPI_OP_MODE_SLAVE	BIT(0)
#define SPI_OP_MODE_MASK	0x1
#define SPI_OP_MODE_GET(_operation_) ((_operation_) & SPI_OP_MODE_MASK)

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
 * capture will occur on low to high transition and high to low if
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
 * @brief Specific SPI devices control bits
 */
/* Requests - if possible - to keep CS asserted after the transaction */
#define SPI_HOLD_ON_CS		BIT(13)
/* Keep the device locked after the transaction for the current config.
 * Use this with extreme caution (see spi_release() below) as it will
 * prevent other callers to access the SPI device until spi_release() is
 * properly called.
 */
#define SPI_LOCK_ON		BIT(14)
/* Select EEPROM read mode on master controller.
 * If not supported by the controller, the driver will have to emulate
 * the mode and thus should never return -EINVAL on that configuration)
 */
#define SPI_EEPROM_MODE		BIT(15)

/**
 * @brief SPI Chip Select control structure
 *
 * This can be used to control a CS line via a GPIO line, instead of
 * using the controller inner CS logic.
 *
 * @param gpio_dev is a valid pointer to an actual GPIO device. A NULL pointer
 *        can be provided to full inhibit CS control if necessary.
 * @param gpio_pin is a number representing the gpio PIN that will be used
 *    to act as a CS line
 * @param delay is a delay in microseconds to wait before starting the
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
 * @param dev is a valid pointer to an actual SPI device
 * @param frequency is the bus frequency in Hertz
 * @param operation is a bit field with the following parts:
 *
 *     operational mode    [ 0 ]       - master or slave.
 *     mode                [ 1 : 3 ]   - Polarity, phase and loop mode.
 *     transfer            [ 4 ]       - LSB or MSB first.
 *     word_size           [ 5 : 10 ]  - Size of a data frame in bits.
 *     lines               [ 11 : 12 ] - MISO lines: Single/Dual/Quad.
 *     cs_hold             [ 13 ]      - Hold on the CS line if possible.
 *     lock_on             [ 14 ]      - Keep resource locked for the caller.
 *     eeprom              [ 15 ]      - EEPROM mode.
 * @param slave is the slave number from 0 to host controller slave limit.
 * @param cs is a valid pointer on a struct spi_cs_control is CS line is
 *    emulated through a gpio line, or NULL otherwise.
 *
 * @note cs_hold, lock_on and eeprom_rx can be changed between consecutive
 * transceive call.
 */
struct spi_config {
	struct device	*dev;

	u32_t		frequency;
	u16_t		operation;
	u16_t		slave;

	struct spi_cs_control *cs;
};

/**
 * @brief SPI buffer structure
 *
 * @param buf is a valid pointer on a data buffer, or NULL otherwise.
 * @param len is the length of the buffer or, if buf is NULL, will be the
 *    length which as to be sent as dummy bytes (as TX buffer) or
 *    the length of bytes that should be skipped (as RX buffer).
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
typedef int (*spi_api_io)(struct spi_config *config,
			  const struct spi_buf *tx_bufs,
			  size_t tx_count,
			  struct spi_buf *rx_bufs,
			  size_t rx_count);

/**
 * @typedef spi_api_io
 * @brief Callback API for asynchronous I/O
 * See spi_transceive_async() for argument descriptions
 */
typedef int (*spi_api_io_async)(struct spi_config *config,
				const struct spi_buf *tx_bufs,
				size_t tx_count,
				struct spi_buf *rx_bufs,
				size_t rx_count,
				struct k_poll_signal *async);

/**
 * @typedef spi_api_release
 * @brief Callback API for unlocking SPI device.
 * See spi_release() for argument descriptions
 */
typedef int (*spi_api_release)(struct spi_config *config);


/**
 * @brief SPI driver API
 * This is the mandatory API any SPI driver needs to expose.
 */
struct spi_driver_api {
	spi_api_io transceive;
#ifdef CONFIG_POLL
	spi_api_io_async transceive_async;
#endif
	spi_api_release release;
};

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param tx_count Number of element in the tx_bufs array.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 * @param rx_count Number of element in the rx_bufs array.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
__syscall int spi_transceive(struct spi_config *config,
			     const struct spi_buf *tx_bufs,
			     size_t tx_count,
			     struct spi_buf *rx_bufs,
			     size_t rx_count);

static inline int _impl_spi_transceive(struct spi_config *config,
				       const struct spi_buf *tx_bufs,
				       size_t tx_count,
				       struct spi_buf *rx_bufs,
				       size_t rx_count)
{
	const struct spi_driver_api *api = config->dev->driver_api;

	return api->transceive(config, tx_bufs, tx_count, rx_bufs, rx_count);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param config Pointer to a valid spi_config structure instance.
 * @param rx_bufs Buffer array where data to be read will be written to.
 * @param rx_count Number of element in the rx_bufs array.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int spi_read(struct spi_config *config,
			   struct spi_buf *rx_bufs,
			   size_t rx_count)
{
	return spi_transceive(config, NULL, 0, rx_bufs, rx_count);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from.
 * @param tx_count Number of element in the tx_bufs array.
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int spi_write(struct spi_config *config,
			    const struct spi_buf *tx_bufs,
			    size_t tx_count)
{
	return spi_transceive(config, tx_bufs, tx_count, NULL, 0);
}

#ifdef CONFIG_POLL
/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is asynchronous.
 *
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param tx_count Number of element in the tx_bufs array.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 * @param rx_count Number of element in the rx_bufs array.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int spi_transceive_async(struct spi_config *config,
				       const struct spi_buf *tx_bufs,
				       size_t tx_count,
				       struct spi_buf *rx_bufs,
				       size_t rx_count,
				       struct k_poll_signal *async)
{
	const struct spi_driver_api *api = config->dev->driver_api;

	return api->transceive_async(config, tx_bufs, tx_count,
				     rx_bufs, rx_count, async);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * Note: This function is asynchronous.
 *
 * @param config Pointer to a valid spi_config structure instance.
 * @param rx_bufs Buffer array where data to be read will be written to.
 * @param rx_count Number of element in the rx_bufs array.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int spi_read_async(struct spi_config *config,
				 struct spi_buf *rx_bufs,
				 size_t rx_count,
				 struct k_poll_signal *async)
{
	const struct spi_driver_api *api = config->dev->driver_api;

	return api->transceive_async(config, NULL, 0,
				     rx_bufs, rx_count, async);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * Note: This function is asynchronous.
 *
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from.
 * @param tx_count Number of element in the tx_bufs array.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval 0 If successful, negative errno code otherwise.
 */
static inline int spi_write_async(struct spi_config *config,
				  const struct spi_buf *tx_bufs,
				  size_t tx_count,
				  struct k_poll_signal *async)
{
	const struct spi_driver_api *api = config->dev->driver_api;

	return api->transceive_async(config, tx_bufs, tx_count,
				     NULL, 0, async);
}
#endif /* CONFIG_POLL */

/**
 * @brief Release the SPI device locked on by the current config
 *
 * Note: This synchronous function is used to release the lock on the SPI
 *       device that was kept if, and if only, given config parameter was
 *       the last one to be used (in any of the above functions) and if
 *       it has the SPI_LOCK_ON bit set into its operation bits field.
 *       This can be used if the caller needs to keep its hand on the SPI
 *       device for consecutive transactions.
 *
 * @param config Pointer to a valid spi_config structure instance.
 */
__syscall int spi_release(struct spi_config *config);

static inline int _impl_spi_release(struct spi_config *config)
{
	const struct spi_driver_api *api = config->dev->driver_api;

	return api->release(config);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/spi.h>

#endif /* __SPI_H__ */
#endif /* CONFIG_SPI_LEGACY_API */
