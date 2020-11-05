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
#include <drivers/gpio.h>

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
 * @param gpio_dt_flags is the devicetree flags corresponding to how the CS
 *    line should be driven. GPIO_ACTIVE_LOW/GPIO_ACTIVE_HIGH should be
 *    equivalent to SPI_CS_ACTIVE_HIGH/SPI_CS_ACTIVE_LOW options in struct
 *    spi_config.
 */
struct spi_cs_control {
	const struct device	*gpio_dev;
	uint32_t		delay;
	gpio_pin_t		gpio_pin;
	gpio_dt_flags_t		gpio_dt_flags;
};

#ifndef __cplusplus
/**
 * @brief Initialize and get a pointer to a @p spi_cs_control from a
 *        devicetree node identifier
 *
 * This helper is useful for initializing a device on a SPI bus. It
 * initializes a struct spi_cs_control and returns a pointer to it.
 * Here, @p node_id is a node identifier for a SPI device, not a SPI
 * controller.
 *
 * Example devicetree fragment:
 *
 *     spi@... {
 *             cs-gpios = <&gpio0 1 GPIO_ACTIVE_LOW>;
 *             spidev: spi-device@0 { ... };
 *     };
 *
 * Assume that @p gpio0 follows the standard convention for specifying
 * GPIOs, i.e. it has the following in its binding:
 *
 *     gpio-cells:
 *     - pin
 *     - flags
 *
 * Example usage:
 *
 *     struct spi_cs_control *ctrl =
 *             SPI_CS_CONTROL_PTR_DT(DT_NODELABEL(spidev), 2);
 *
 * This example is equivalent to:
 *
 *     struct spi_cs_control *ctrl =
 *             &(struct spi_cs_control) {
 *                     .gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0)),
 *                     .delay = 2,
 *                     .gpio_pin = 1,
 *                     .gpio_dt_flags = GPIO_ACTIVE_LOW
 *             };
 *
 * This macro is not available in C++.
 *
 * @param node_id Devicetree node identifier for a device on a SPI bus
 * @param delay_ The @p delay field to set in the @p spi_cs_control
 * @return a pointer to the @p spi_cs_control structure
 */
#define SPI_CS_CONTROL_PTR_DT(node_id, delay_)				\
	(&(struct spi_cs_control) {						\
		.gpio_dev = DEVICE_DT_GET(				\
			DT_SPI_DEV_CS_GPIOS_CTLR(node_id)),		\
		.delay = (delay_),					\
		.gpio_pin = DT_SPI_DEV_CS_GPIOS_PIN(node_id),		\
		.gpio_dt_flags = DT_SPI_DEV_CS_GPIOS_FLAGS(node_id),	\
	})

/**
 * @brief Get a pointer to a @p spi_cs_control from a devicetree node
 *
 * This is equivalent to
 * <tt>SPI_CS_CONTROL_PTR_DT(DT_DRV_INST(inst), delay)</tt>.
 *
 * Therefore, @p DT_DRV_COMPAT must already be defined before using
 * this macro.
 *
 * This macro is not available in C++.
 *
 * @param inst Devicetree node instance number
 * @param delay_ The @p delay field to set in the @p spi_cs_control
 * @return a pointer to the @p spi_cs_control structure
 */
#define SPI_CS_CONTROL_PTR_DT_INST(inst, delay_)		\
	SPI_CS_CONTROL_PTR_DT(DT_DRV_INST(inst), delay_)
#endif

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
 * @param cs is a valid pointer on a struct spi_cs_control is CS line is
 *    emulated through a gpio line, or NULL otherwise.
 *
 * @note Only cs_hold and lock_on can be changed between consecutive
 * transceive call. Rest of the attributes are not meant to be tweaked.
 *
 * @warning Most drivers use pointer comparison to determine whether a
 * passed configuration is different from one used in a previous
 * transaction.  Changes to fields in the structure may not be
 * detected.
 */
struct spi_config {
	uint32_t		frequency;
	uint16_t		operation;
	uint16_t		slave;

	const struct spi_cs_control *cs;
};

#ifndef __cplusplus
/**
 * @brief Structure initializer for spi_config from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * spi_config</tt> by reading the relevant @p frequency, @p slave, and
 * @p cs data from the devicetree.
 *
 * Important: the @p cs field is initialized using
 * SPI_CS_CONTROL_PTR_DT(). The @p gpio_dev value pointed to by this
 * structure must be checked using device_is_ready() before use.
 *
 * This macro is not available in C++.
 *
 * @param node_id Devicetree node identifier for the SPI device whose
 *                struct spi_config to create an initializer for
 * @param operation_ the desired @p operation field in the struct spi_config
 * @param delay_ the desired @p delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define SPI_CONFIG_DT(node_id, operation_, delay_)			\
	{								\
		.frequency = DT_PROP(node_id, spi_max_frequency),	\
		.operation = (operation_),				\
		.slave = DT_REG_ADDR(node_id),				\
		.cs = COND_CODE_1(					\
			DT_SPI_DEV_HAS_CS_GPIOS(node_id),		\
			(SPI_CS_CONTROL_PTR_DT(node_id, delay_)),	\
			(NULL)),					\
	}

/**
 * @brief Structure initializer for spi_config from devicetree instance
 *
 * This is equivalent to
 * <tt>SPI_CONFIG_DT(DT_DRV_INST(inst), operation_, delay_)</tt>.
 *
 * This macro is not available in C++.
 *
 * @param inst Devicetree instance number
 * @param operation_ the desired @p operation field in the struct spi_config
 * @param delay_ the desired @p delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define SPI_CONFIG_DT_INST(inst, operation_, delay_)	\
	SPI_CONFIG_DT(DT_DRV_INST(inst), operation_, delay_)
#endif

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

/**
 * @typedef spi_api_io
 * @brief Callback API for I/O
 * See spi_transceive() for argument descriptions
 */
typedef int (*spi_api_io)(const struct device *dev,
			  const struct spi_config *config,
			  const struct spi_buf_set *tx_bufs,
			  const struct spi_buf_set *rx_bufs);

/**
 * @typedef spi_api_io
 * @brief Callback API for asynchronous I/O
 * See spi_transceive_async() for argument descriptions
 */
typedef int (*spi_api_io_async)(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				struct k_poll_signal *async);

/**
 * @typedef spi_api_release
 * @brief Callback API for unlocking SPI device.
 * See spi_release() for argument descriptions
 */
typedef int (*spi_api_release)(const struct device *dev,
			       const struct spi_config *config);


/**
 * @brief SPI driver API
 * This is the mandatory API any SPI driver needs to expose.
 */
__subsystem struct spi_driver_api {
	spi_api_io transceive;
#ifdef CONFIG_SPI_ASYNC
	spi_api_io_async transceive_async;
#endif /* CONFIG_SPI_ASYNC */
	spi_api_release release;
};

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 *
 * @retval 0 If successful, negative errno code otherwise. In case of slave
 *         transaction: if successful it will return the amount of frames
 *         received, negative errno code otherwise.
 */
__syscall int spi_transceive(const struct device *dev,
			     const struct spi_config *config,
			     const struct spi_buf_set *tx_bufs,
			     const struct spi_buf_set *rx_bufs);

static inline int z_impl_spi_transceive(const struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->api;

	return api->transceive(dev, config, tx_bufs, rx_bufs);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * Note: This function is synchronous.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param rx_bufs Buffer array where data to be read will be written to.
 *
 * @retval 0 If successful, negative errno code otherwise.
 *
 * @note This function is an helper function calling spi_transceive.
 */
static inline int spi_read(const struct device *dev,
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
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from.
 *
 * @retval 0 If successful, negative errno code otherwise.
 *
 * @note This function is an helper function calling spi_transceive.
 */
static inline int spi_write(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs)
{
	return spi_transceive(dev, config, tx_bufs, NULL);
}

/* Doxygen defines this so documentation is generated. */
#ifdef CONFIG_SPI_ASYNC

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is available only if @option{CONFIG_SPI_ASYNC}
 * is selected.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
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
static inline int spi_transceive_async(const struct device *dev,
				       const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs,
				       struct k_poll_signal *async)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->api;

	return api->transceive_async(dev, config, tx_bufs, rx_bufs, async);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is available only if @option{CONFIG_SPI_ASYNC}
 * is selected.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
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
static inline int spi_read_async(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *rx_bufs,
				 struct k_poll_signal *async)
{
	return spi_transceive_async(dev, config, NULL, rx_bufs, async);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is available only if @option{CONFIG_SPI_ASYNC}
 * is selected.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
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
static inline int spi_write_async(const struct device *dev,
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
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 */
__syscall int spi_release(const struct device *dev,
			  const struct spi_config *config);

static inline int z_impl_spi_release(const struct device *dev,
				     const struct spi_config *config)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->api;

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
