/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SPI drivers and applications
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPI_H_

/**
 * @brief SPI Interface
 * @defgroup spi_interface SPI Interface
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <device.h>
#include <sys/qop_mngr.h>

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
 * Some controllers support dual, quad or octal MISO lines connected to slaves.
 * Default is single, which is the case most of the time.
 */
#define SPI_LINES_SINGLE	(0 << 11)
#define SPI_LINES_DUAL		(1 << 11)
#define SPI_LINES_QUAD		(2 << 11)
#define SPI_LINES_OCTAL		(3 << 11)

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

/* Active high logic on CS - Usually, and by default, CS logic is active
 * low. However, some devices may require the reverse logic: active high.
 * This bit will request the controller to use that logic. Note that not
 * all controllers are able to handle that natively. In this case deferring
 * the CS control to a gpio line through struct spi_cs_control would be
 * the solution.
 */
#define SPI_CS_ACTIVE_HIGH	BIT(15)

/**@defgroup SPI_MSG_FLAGS SPI message flags
 *
 * @brief Used for message configuration.
 * @{ */

/** @brief Message for reading the data. */
#define SPI_MSG_READ		(0)

/** @brief Message for writing the data. */
#define SPI_MSG_WRITE		BIT(0)
#define SPI_MSG_DIR_MASK	SPI_MSG_WRITE

/** @brief Indicate that transfer should be started after this message.
 *
 * Duplex transfer (RXTX) is described by two messages. First comes read message
 * without commit flag and it is followed by write message with commit flag set.
 */
#define SPI_MSG_COMMIT		BIT(1)

/** @brief Activate CS_0 on message start. */
#define SPI_MSG_CS0_START_SET	BIT(2)
#define SPI_MSG_CS0_START_MASK	SPI_MSG_CS0_START_SET

/** @brief No CS_0 action on message start. */
#define SPI_MSG_CS0_START_NOP	(0)

/** @brief Deactivate CS_0 on message end. */
#define SPI_MSG_CS0_END_CLR	BIT(3)
#define SPI_MSG_CS0_END_MASK	SPI_MSG_CS0_END_CLR

/** @brief No CS_0 action on message end. */
#define SPI_MSG_CS0_END_NOP	(0))

/** @brief Activate CS_1 on message start. */
#define SPI_MSG_CS1_START_SET	BIT(4)
#define SPI_MSG_CS1_START_MASK	SPI_MSG_CS1_START_SET

/** @brief No CS_1 action on message start. */
#define SPI_MSG_CS1_START_NOP	(0)

/** @brief Deactivate CS_1
 *  on message end. */
#define SPI_MSG_CS1_END_CLR	BIT(5)
#define SPI_MSG_CS1_END_MASK	SPI_MSG_CS1_END_CLR

/** @brief No CS_1 action on message end. */
#define SPI_MSG_CS1_END_NOP	(0))

#define SPI_MSG_DELAY_BITS		3
#define SPI_MSG_DELAY_OFFSET		5
#define SPI_MSG_DELAY_MASK \
	((BIT(SPI_MSG_DELAY_BITS) - 1) << SPI_MSG_DELAY_OFFSET)
/** @brief Add delay before starting operation. */
#define SPI_MSG_DELAY(ms) \
	((ms /*& SPI_MSG_DELAY_MASK*/) << SPI_MSG_DELAY_OFFSET)

/** @brief Pause transaction after that message. Use spi_resume() to resume. */
#define SPI_MSG_PAUSE_AFTER	BIT(9)
/**@} */

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
 * @param frequency is the bus frequency in Hertz
 * @param operation is a bit field with the following parts:
 *
 *     operational mode    [ 0 ]       - master or slave.
 *     mode                [ 1 : 3 ]   - Polarity, phase and loop mode.
 *     transfer            [ 4 ]       - LSB or MSB first.
 *     word_size           [ 5 : 10 ]  - Size of a data frame in bits.
 *     lines               [ 11 : 12 ] - MISO lines: Single/Dual/Quad/Octal.
 *     cs_hold             [ 13 ]      - Hold on the CS line if possible.
 *     lock_on             [ 14 ]      - Keep resource locked for the caller.
 *     cs_active_high      [ 15 ]      - Active high CS logic.
 * @param slave is the slave number from 0 to host controller slave limit.
 * @param cs is a pointer to an array of chip select objects. CS line is
 *    emulated through a gpio line, or NULL for no CS control.
 * @param num_cs Number of chip select objects. For backward compatibity, if
 *		pointer is not NULL and num_cs is 0, it is still assumed that
 *		there is one chip select object.
 *
 * @note Only cs_hold and lock_on can be changed between consecutive
 * transceive call. Rest of the attributes are not meant to be tweaked.
 */
struct spi_config {
	u32_t		frequency;
	u16_t		operation;
	u16_t		slave;

	const struct spi_cs_control *cs;
	u8_t num_cs;
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
 * @brief SPI buffer array structure
 *
 * @param buffers is a valid pointer on an array of spi_buf, or NULL.
 * @param count is the length of the array pointed by buffers.
 */
struct spi_buf_set {
	const struct spi_buf *buffers;
	size_t count;
};

/** @brief Callback called on completion of single message transfer.
 *
 * Callback can be called from two contexts:
 * - SPI transfer completion interrupt if message had SPI_MSG_COMMIT flag set
 * - spi_single_transfer() call context if message did not have flag set.
 */
typedef void (*spi_transfer_callback_t)(struct device *dev, int res,
					void *user_data);

/** @brief SPI simplex message.
 *
 * Message contains single data (read or write). Flags contains message details.
 */
struct spi_msg {
	u8_t *buf;
	u16_t len;
	u16_t flags; /* see SPI_MSG_FLAGS */
};

/* Forward declaration. */
struct spi_transaction;

typedef void (*spi_paused_callback_t)(struct device *dev,
				struct spi_transaction *transaction,
				u8_t msg_idx);

/** @brief SPI transaction.
 *
 * Transaction contains single SPI configuration and 1 or many messages. User
 * is notified once transaction is completed (all messages proceeded or error
 * occured.)
 */
struct spi_transaction {
	struct qop_op op;
	const struct spi_config *config;
	spi_paused_callback_t paused_callback;
	const struct spi_msg *msgs;
	u8_t num_msgs;
	bool paused;
};


/** @brief Create set of messages needed for TXRX transfer with CS. */
#define SPI_MSG_TXRX(txbuf, txlen, rxbuf, rxlen, _flags) \
	{ \
		.buf = (u8_t *)txbuf, \
		.len = txlen, \
		.flags = SPI_MSG_WRITE | SPI_MSG_COMMIT | \
			SPI_MSG_CS0_START_SET | _flags\
	}, \
	{ \
		.buf = (u8_t *)rxbuf, \
		.len = rxlen, \
		.flags = SPI_MSG_READ | SPI_MSG_COMMIT | SPI_MSG_CS0_END_CLR \
	}

/** @brief Create set of messages needed for TXTX transfer with CS. */
#define SPI_MSG_TXTX(txbuf0, txlen0, txbuf1, txlen1, _flags) \
	{ \
		.buf = (u8_t *)txbuf0, \
		.len = txlen0, \
		.flags = SPI_MSG_WRITE | SPI_MSG_COMMIT | \
			SPI_MSG_CS0_START_SET | _flags \
	}, \
	{ \
		.buf = (u8_t *)txbuf1, \
		.len = txlen1, \
		.flags = SPI_MSG_WRITE | SPI_MSG_COMMIT | SPI_MSG_CS0_END_CLR \
	}

/** @brief Create message for for TX transfer with CS. */
#define SPI_MSG_TX(txbuf, txlen, _flags) \
	{ \
		.buf = (u8_t *)txbuf, \
		.len = txlen, \
		.flags = SPI_MSG_WRITE | SPI_MSG_COMMIT | SPI_MSG_CS0_END_CLR |\
			SPI_MSG_CS0_START_SET | _flags \
	}

/** @brief Create message for for RX transfer with CS. */
#define SPI_MSG_RX(rxbuf, rxlen, _flags) \
	{ \
		.buf = (u8_t *)rxbuf, \
		.len = rxlen, \
		.flags = SPI_MSG_READ | SPI_MSG_COMMIT | SPI_MSG_CS0_END_CLR |\
			SPI_MSG_CS0_START_SET | _flags \
	}


/**
 * @typedef spi_api_io
 * @brief Callback API for I/O
 * See spi_transceive() for argument descriptions
 */
typedef int (*spi_api_io)(struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs);

/**
 * @typedef spi_api_io
 * @brief Callback API for asynchronous I/O
 * See spi_transceive_async() for argument descriptions
 */
typedef int (*spi_api_io_async)(struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				struct k_poll_signal *async);


typedef int (*spi_api_single_async)(struct device *dev,
				const struct spi_msg *msg,
				spi_transfer_callback_t callback,
				void *user_data);

typedef int (*spi_api_configure)(struct device *dev,
				const struct spi_config *config);
/**
 * @typedef spi_api_release
 * @brief Callback API for unlocking SPI device.
 * See spi_release() for argument descriptions
 */
typedef int (*spi_api_release)(struct device *dev,
			       const struct spi_config *config);


/**
 * @brief SPI driver API
 * This is the mandatory API any SPI driver needs to expose.
 */
struct spi_driver_api {
	spi_api_io transceive;
	spi_api_single_async single_transfer;
	spi_api_configure configure;
#ifdef CONFIG_SPI_ASYNC
	spi_api_io_async transceive_async;
#endif /* CONFIG_SPI_ASYNC */
	spi_api_release release;
};

/** @brief Asynchronously process single message.
 *
 * SPI must be configured prior issueing the message.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param msg Message.
 * @param callback Callback called on message completion. Callback is called
 * 			from interrupt context or caller context.
 * @param user_data User data passed to the callback.
 *
 * @retval 0 on success.
 * @retval -EBUSY if driver is busy.
 * @retval -ENOTSUP if not supported.
 * @retval -EINVAL if write message without commit flag.
 * @retval -EIO on internal driver error.
 */
static inline int spi_single_transfer(struct device *dev,
				const struct spi_msg *msg,
				spi_transfer_callback_t callback,
				void *user_data)
{
	__ASSERT_NO_MSG(msg);
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->driver_api;

	if (api->single_transfer == NULL) {
		return -ENOTSUP;
	}

	return api->single_transfer(dev, msg, callback, user_data);
}

/** @brief Set asynchrnous client for transaction.
 *
 * @param transaction	Transaction.
 * @param client	Asynchronous client to be used for transaction.
 */
static inline void spi_set_async_client(struct spi_transaction *transaction,
					struct async_client *client)
{
	__ASSERT_NO_MSG(transaction);
	__ASSERT_NO_MSG(client);
	transaction->op.async_cli = client;
}

/** @brief Configure SPI.
 *
 * Manually configure SPI. Must be used if spi_single_transfer() is used
 * explicitly.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 */
static inline int spi_configure(struct device *dev,
				const struct spi_config *config)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->driver_api;

	return api->configure(dev, config);
}

/** @brief Asynchronously schedule SPI transaction.
 *
 * User is notified once transaction is completed or error occured.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param transaction Pointer to the transaction.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if not supported.
 * @retval other negative value if fails to start.
 */
int spi_schedule(struct device *dev, const struct spi_transaction *transaction);

/** @brief Cancel transaction.
 *
 * Transaction can be cancelled if it is not yet started.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param transaction Pointer to the transaction.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if not supported.
 * @retval -EALREADY if already in progress.
 * @retval -EINVAL if transaction not active.
 */
int spi_cancel(struct device *dev, const struct spi_transaction *transaction);

/** @brief Resume transaction.
 *
 * If transaction contains message with SPI_MSG_PAUSE_AFTER flag set then
 * next transfer is holded until it is resumed.
 *
 * @param dev Pointer to the device structure for the driver instance
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if not supported.
 * @retval -EFAULT if transaction not in paused state.
 */
int spi_resume(struct device *dev);

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 *
 * @retval 0 If successful, negative errno code otherwise. In case of slave
 *         transaction: if successful it will return the amount of frames
 *         received, negative errno code otherwise.
 */
__syscall int spi_transceive(struct device *dev,
			     const struct spi_config *config,
			     const struct spi_buf_set *tx_bufs,
			     const struct spi_buf_set *rx_bufs);

static inline int z_impl_spi_transceive(struct device *dev,
				       const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->driver_api;

	return api->transceive(dev, config, tx_bufs, rx_bufs);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 * @param rx_bufs Buffer array where data to be read will be written to.
 *
 * @retval 0 If successful, negative errno code otherwise.
 *
 * @note This function is an helper function calling spi_transceive.
 */
static inline int spi_read(struct device *dev,
			   const struct spi_config *config,
			   const struct spi_buf_set *rx_bufs)
{
	return spi_transceive(dev, config, NULL, rx_bufs);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from.
 *
 * @retval 0 If successful, negative errno code otherwise.
 *
 * @note This function is an helper function calling spi_transceive.
 */
static inline int spi_write(struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs)
{
	return spi_transceive(dev, config, tx_bufs, NULL);
}

#ifdef CONFIG_SPI_ASYNC
/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is asynchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval 0 If successful, negative errno code otherwise. In case of slave
 *         transaction: if successful it will return the amount of frames
 *         received, negative errno code otherwise.
 */
static inline int spi_transceive_async(struct device *dev,
				       const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs,
				       struct k_poll_signal *async)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->driver_api;

	return api->transceive_async(dev, config, tx_bufs, rx_bufs, async);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * Note: This function is asynchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 * @param rx_bufs Buffer array where data to be read will be written to.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval 0 If successful, negative errno code otherwise.
 *
 * @note This function is an helper function calling spi_transceive_async.
 */
static inline int spi_read_async(struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *rx_bufs,
				 struct k_poll_signal *async)
{
	return spi_transceive_async(dev, config, NULL, rx_bufs, async);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * Note: This function is asynchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 * @param tx_bufs Buffer array where data to be sent originates from.
 * @param async A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval 0 If successful, negative errno code otherwise.
 *
 * @note This function is an helper function calling spi_transceive_async.
 */
static inline int spi_write_async(struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  struct k_poll_signal *async)
{
	return spi_transceive_async(dev, config, tx_bufs, NULL, async);
}
#endif /* CONFIG_SPI_ASYNC */

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
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 */
__syscall int spi_release(struct device *dev,
			  const struct spi_config *config);

static inline int z_impl_spi_release(struct device *dev,
				    const struct spi_config *config)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->driver_api;

	return api->release(dev, config);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/spi.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_H_ */
