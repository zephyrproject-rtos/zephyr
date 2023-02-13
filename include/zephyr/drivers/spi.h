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
#include <zephyr/device.h>
#include <zephyr/dt-bindings/spi/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name SPI operational mode
 * @{
 */
#define SPI_OP_MODE_MASTER	0U
#define SPI_OP_MODE_SLAVE	BIT(0)
#define SPI_OP_MODE_MASK	0x1U
#define SPI_OP_MODE_GET(_operation_) ((_operation_) & SPI_OP_MODE_MASK)
/** @} */

/**
 * @name SPI Polarity & Phase Modes
 * @{
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

#define SPI_MODE_MASK		(0xEU)
#define SPI_MODE_GET(_mode_)			\
	((_mode_) & SPI_MODE_MASK)

/** @} */

/**
 * @name SPI Transfer modes (host controller dependent)
 * @{
 */
#define SPI_TRANSFER_MSB	(0U)
#define SPI_TRANSFER_LSB	BIT(4)
/** @} */

/**
 * @name SPI word size
 * @{
 */
#define SPI_WORD_SIZE_SHIFT	(5U)
#define SPI_WORD_SIZE_MASK	(0x3FU << SPI_WORD_SIZE_SHIFT)
#define SPI_WORD_SIZE_GET(_operation_)					\
	(((_operation_) & SPI_WORD_SIZE_MASK) >> SPI_WORD_SIZE_SHIFT)

#define SPI_WORD_SET(_word_size_)		\
	((_word_size_) << SPI_WORD_SIZE_SHIFT)
/** @} */

/**
 * @name Specific SPI devices control bits
 * @{
 */
/* Requests - if possible - to keep CS asserted after the transaction */
#define SPI_HOLD_ON_CS		BIT(12)
/* Keep the device locked after the transaction for the current config.
 * Use this with extreme caution (see spi_release() below) as it will
 * prevent other callers to access the SPI device until spi_release() is
 * properly called.
 */
#define SPI_LOCK_ON		BIT(13)

/* Active high logic on CS - Usually, and by default, CS logic is active
 * low. However, some devices may require the reverse logic: active high.
 * This bit will request the controller to use that logic. Note that not
 * all controllers are able to handle that natively. In this case deferring
 * the CS control to a gpio line through struct spi_cs_control would be
 * the solution.
 */
#define SPI_CS_ACTIVE_HIGH	BIT(14)
/** @} */

/**
 * @name SPI MISO lines (if @kconfig{CONFIG_SPI_EXTENDED_MODES} is enabled)
 * @{
 *
 * Some controllers support dual, quad or octal MISO lines connected to slaves.
 * Default is single, which is the case most of the time.
 * Without @kconfig{CONFIG_SPI_EXTENDED_MODES} being enabled, single is the
 * only supported one.
 */
#define SPI_LINES_SINGLE	(0U << 16)
#define SPI_LINES_DUAL		(1U << 16)
#define SPI_LINES_QUAD		(2U << 16)
#define SPI_LINES_OCTAL		(3U << 16)

#define SPI_LINES_MASK		(0x3U << 16)
/** @} */

/**
 * @brief SPI Chip Select control structure
 *
 * This can be used to control a CS line via a GPIO line, instead of
 * using the controller inner CS logic.
 *
 */
struct spi_cs_control {
	/**
	 * GPIO devicetree specification of CS GPIO.
	 * The device pointer can be set to NULL to fully inhibit CS control if
	 * necessary. The GPIO flags GPIO_ACTIVE_LOW/GPIO_ACTIVE_HIGH should be
	 * equivalent to SPI_CS_ACTIVE_HIGH/SPI_CS_ACTIVE_LOW options in struct
	 * spi_config.
	 */
	struct gpio_dt_spec gpio;
	/**
	 * Delay in microseconds to wait before starting the
	 * transmission and before releasing the CS line.
	 */
	uint32_t delay;
};

/**
 * @brief Get a <tt>struct gpio_dt_spec</tt> for a SPI device's chip select pin
 *
 * Example devicetree fragment:
 *
 * @code{.devicetree}
 *     gpio1: gpio@abcd0001 { ... };
 *
 *     gpio2: gpio@abcd0002 { ... };
 *
 *     spi@abcd0003 {
 *             compatible = "vnd,spi";
 *             cs-gpios = <&gpio1 10 GPIO_ACTIVE_LOW>,
 *                        <&gpio2 20 GPIO_ACTIVE_LOW>;
 *
 *             a: spi-dev-a@0 {
 *                     reg = <0>;
 *             };
 *
 *             b: spi-dev-b@1 {
 *                     reg = <1>;
 *             };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(a)) \
 *           // { DEVICE_DT_GET(DT_NODELABEL(gpio1)), 10, GPIO_ACTIVE_LOW }
 *     SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(b)) \
 *           // { DEVICE_DT_GET(DT_NODELABEL(gpio2)), 20, GPIO_ACTIVE_LOW }
 * @endcode
 *
 * @param spi_dev a SPI device node identifier
 * @return #gpio_dt_spec struct corresponding with spi_dev's chip select
 */
#define SPI_CS_GPIOS_DT_SPEC_GET(spi_dev) \
	GPIO_DT_SPEC_GET_BY_IDX(DT_BUS(spi_dev), cs_gpios, DT_REG_ADDR(spi_dev))

/**
 * @brief Get a <tt>struct gpio_dt_spec</tt> for a SPI device's chip select pin
 *
 * This is equivalent to
 * <tt>SPI_CS_GPIOS_DT_SPEC_GET(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 * @return #gpio_dt_spec struct corresponding with spi_dev's chip select
 */
#define SPI_CS_GPIOS_DT_SPEC_INST_GET(inst) \
	SPI_CS_GPIOS_DT_SPEC_GET(DT_DRV_INST(inst))

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
#define SPI_CS_CONTROL_PTR_DT(node_id, delay_)			  \
	(&(struct spi_cs_control) {				  \
		.gpio = SPI_CS_GPIOS_DT_SPEC_GET(node_id),	  \
		.delay = (delay_),				  \
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
 *     duplex              [ 11 ]      - full/half duplex.
 *     cs_hold             [ 12 ]      - Hold on the CS line if possible.
 *     lock_on             [ 13 ]      - Keep resource locked for the caller.
 *     cs_active_high      [ 14 ]      - Active high CS logic.
 *     format              [ 15 ]      - Motorola or TI frame format (optional).
 * if @kconfig{CONFIG_SPI_EXTENDED_MODES} is defined:
 *     lines               [ 16 : 17 ] - MISO lines: Single/Dual/Quad/Octal.
 *     reserved            [ 18 : 31 ] - reserved for future use.
 * @param slave is the slave number from 0 to host controller slave limit.
 * @param cs is a valid pointer on a struct spi_cs_control is CS line is
 *    emulated through a gpio line, or NULL otherwise.
 * @warning Most drivers use pointer comparison to determine whether a
 * passed configuration is different from one used in a previous
 * transaction.  Changes to fields in the structure may not be
 * detected.
 */
struct spi_config {
	uint32_t		frequency;
#if defined(CONFIG_SPI_EXTENDED_MODES)
	uint32_t		operation;
	uint16_t		slave;
	uint16_t		_unused;
#else
	uint16_t		operation;
	uint16_t		slave;
#endif /* CONFIG_SPI_EXTENDED_MODES */

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
		.operation = (operation_) |				\
			DT_PROP(node_id, duplex) |			\
			DT_PROP(node_id, frame_format),			\
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
 * @brief Complete SPI DT information
 *
 * @param bus is the SPI bus
 * @param config is the slave specific configuration
 */
struct spi_dt_spec {
	const struct device *bus;
	struct spi_config config;
};

#ifndef __cplusplus
/**
 * @brief Structure initializer for spi_dt_spec from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * spi_dt_spec</tt> by reading the relevant bus, frequency, slave, and cs
 * data from the devicetree.
 *
 * Important: multiple fields are automatically constructed by this macro
 * which must be checked before use. @ref spi_is_ready performs the required
 * @ref device_is_ready checks.
 * @deprecated Use @ref spi_is_ready_dt instead.
 *
 * This macro is not available in C++.
 *
 * @param node_id Devicetree node identifier for the SPI device whose
 *                struct spi_dt_spec to create an initializer for
 * @param operation_ the desired @p operation field in the struct spi_config
 * @param delay_ the desired @p delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define SPI_DT_SPEC_GET(node_id, operation_, delay_)		     \
	{							     \
		.bus = DEVICE_DT_GET(DT_BUS(node_id)),		     \
		.config = SPI_CONFIG_DT(node_id, operation_, delay_) \
	}

/**
 * @brief Structure initializer for spi_dt_spec from devicetree instance
 *
 * This is equivalent to
 * <tt>SPI_DT_SPEC_GET(DT_DRV_INST(inst), operation_, delay_)</tt>.
 *
 * This macro is not available in C++.
 *
 * @param inst Devicetree instance number
 * @param operation_ the desired @p operation field in the struct spi_config
 * @param delay_ the desired @p delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define SPI_DT_SPEC_INST_GET(inst, operation_, delay_) \
	SPI_DT_SPEC_GET(DT_DRV_INST(inst), operation_, delay_)
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
 * @brief SPI callback for asynchronous transfer requests
 *
 * @param dev SPI device which is notifying of transfer completion or error
 * @param result Result code of the transfer request. 0 is success, -errno for failure.
 * @param data Transfer requester supplied data which is passed along to the callback.
 */
typedef void (*spi_callback_t)(const struct device *dev, int result, void *data);

/**
 * @typedef spi_api_io
 * @brief Callback API for asynchronous I/O
 * See spi_transceive_async() for argument descriptions
 */
typedef int (*spi_api_io_async)(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t cb,
				void *userdata);

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
 * @brief Validate that SPI bus is ready.
 *
 * @param spec SPI specification from devicetree
 *
 * @retval true if the SPI bus is ready for use.
 * @retval false if the SPI bus is not ready for use.
 */
__deprecated
static inline bool spi_is_ready(const struct spi_dt_spec *spec)
{
	/* Validate bus is ready */
	if (!device_is_ready(spec->bus)) {
		return false;
	}
	/* Validate CS gpio port is ready, if it is used */
	if (spec->config.cs &&
	    !device_is_ready(spec->config.cs->gpio.port)) {
		return false;
	}
	return true;
}

/**
 * @brief Validate that SPI bus (and CS gpio if defined) is ready.
 *
 * @param spec SPI specification from devicetree
 *
 * @retval true if the SPI bus is ready for use.
 * @retval false if the SPI bus (or the CS gpio defined) is not ready for use.
 */
static inline bool spi_is_ready_dt(const struct spi_dt_spec *spec)
{
	/* Validate bus is ready */
	if (!device_is_ready(spec->bus)) {
		return false;
	}
	/* Validate CS gpio port is ready, if it is used */
	if (spec->config.cs &&
	    !device_is_ready(spec->config.cs->gpio.port)) {
		return false;
	}
	return true;
}
/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
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
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful in master mode.
 * @retval -errno Negative errno code on failure.
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
 * @brief Read/write data from an SPI bus specified in @p spi_dt_spec.
 *
 * This is equivalent to:
 *
 *     spi_transceive(spec->bus, &spec->config, tx_bufs, rx_bufs);
 *
 * @param spec SPI specification from devicetree
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 *
 * @return a value from spi_transceive().
 */
static inline int spi_transceive_dt(const struct spi_dt_spec *spec,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs)
{
	return spi_transceive(spec->bus, &spec->config, tx_bufs, rx_bufs);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @note This function is an helper function calling spi_transceive.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param rx_bufs Buffer array where data to be read will be written to.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_read(const struct device *dev,
			   const struct spi_config *config,
			   const struct spi_buf_set *rx_bufs)
{
	return spi_transceive(dev, config, NULL, rx_bufs);
}

/**
 * @brief Read data from a SPI bus specified in @p spi_dt_spec.
 *
 * This is equivalent to:
 *
 *     spi_read(spec->bus, &spec->config, rx_bufs);
 *
 * @param spec SPI specification from devicetree
 * @param rx_bufs Buffer array where data to be read will be written to.
 *
 * @return a value from spi_read().
 */
static inline int spi_read_dt(const struct spi_dt_spec *spec,
			      const struct spi_buf_set *rx_bufs)
{
	return spi_read(spec->bus, &spec->config, rx_bufs);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @note This function is an helper function calling spi_transceive.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_write(const struct device *dev,
			    const struct spi_config *config,
			    const struct spi_buf_set *tx_bufs)
{
	return spi_transceive(dev, config, tx_bufs, NULL);
}

/**
 * @brief Write data to a SPI bus specified in @p spi_dt_spec.
 *
 * This is equivalent to:
 *
 *     spi_write(spec->bus, &spec->config, tx_bufs);
 *
 * @param spec SPI specification from devicetree
 * @param tx_bufs Buffer array where data to be sent originates from.
 *
 * @return a value from spi_write().
 */
static inline int spi_write_dt(const struct spi_dt_spec *spec,
			       const struct spi_buf_set *tx_bufs)
{
	return spi_write(spec->bus, &spec->config, tx_bufs);
}

/* Doxygen defines this so documentation is generated. */
#ifdef CONFIG_SPI_ASYNC

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is available only if @kconfig{CONFIG_SPI_ASYNC}
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
 * @param callback Function pointer to completion callback.
 *	  (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 * @param userdata Userdata passed to callback
 *
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful in master mode.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_transceive_cb(const struct device *dev,
				    const struct spi_config *config,
				    const struct spi_buf_set *tx_bufs,
				    const struct spi_buf_set *rx_bufs,
				    spi_callback_t callback,
				    void *userdata)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->api;

	return api->transceive_async(dev, config, tx_bufs, rx_bufs, callback, userdata);
}

#ifdef CONFIG_POLL

/** @cond INTERNAL_HIDDEN */
void z_spi_transfer_signal_cb(const struct device *dev, int result, void *userdata);
/** @endcond */

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is available only if @kconfig{CONFIG_SPI_ASYNC}
 * and @kconfig{CONFIG_POLL} are selected.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from,
 *        or NULL if none.
 * @param rx_bufs Buffer array where data to be read will be written to,
 *        or NULL if none.
 * @param sig A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful in master mode.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_transceive_signal(const struct device *dev,
				       const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs,
				       struct k_poll_signal *sig)
{
	const struct spi_driver_api *api =
		(const struct spi_driver_api *)dev->api;
	spi_callback_t cb = (sig == NULL) ? NULL : z_spi_transfer_signal_cb;

	return api->transceive_async(dev, config, tx_bufs, rx_bufs, cb, sig);
}

/**
 * @brief Alias for spi_transceive_signal for backwards compatibility
 *
 * @deprecated
 * @see spi_transceive_signal
 */
__deprecated static inline int spi_transceive_async(const struct device *dev,
				       const struct spi_config *config,
				       const struct spi_buf_set *tx_bufs,
				       const struct spi_buf_set *rx_bufs,
				       struct k_poll_signal *sig)
{
	return spi_transceive_signal(dev, config, tx_bufs, rx_bufs, sig);
}

/**
 * @brief Read the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is an helper function calling spi_transceive_signal.
 *
 * @note This function is available only if @kconfig{CONFIG_SPI_ASYNC}
 * and @kconfig{CONFIG_POLL} are selected.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param rx_bufs Buffer array where data to be read will be written to.
 * @param sig A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval 0 If successful
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_read_signal(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *rx_bufs,
				 struct k_poll_signal *sig)
{
	return spi_transceive_signal(dev, config, NULL, rx_bufs, sig);
}

/**
 * @brief Alias for spi_read_signal for backwards compatibility
 *
 * @deprecated
 * @see spi_read_signal
 */
__deprecated static inline int spi_read_async(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *rx_bufs,
				 struct k_poll_signal *sig)
{
	return spi_read_signal(dev, config, rx_bufs, sig);
}

/**
 * @brief Write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is an helper function calling spi_transceive_async.
 *
 * @note This function is available only if @kconfig{CONFIG_SPI_ASYNC}
 * and @kconfig{CONFIG_POLL} are selected.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from.
 * @param sig A pointer to a valid and ready to be signaled
 *        struct k_poll_signal. (Note: if NULL this function will not
 *        notify the end of the transaction, and whether it went
 *        successfully or not).
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_write_signal(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  struct k_poll_signal *sig)
{
	return spi_transceive_signal(dev, config, tx_bufs, NULL, sig);
}

/**
 * @brief Alias for spi_read_signal for backwards compatibility
 *
 * @deprecated
 * @see spi_read_signal
 */
__deprecated static inline int spi_write_async(const struct device *dev,
				 const struct spi_config *config,
				 const struct spi_buf_set *tx_bufs,
				 struct k_poll_signal *sig)
{
	return spi_write_signal(dev, config, tx_bufs, sig);
}

#endif /* CONFIG_POLL */

#endif /* CONFIG_SPI_ASYNC */

/**
 * @brief Release the SPI device locked on and/or the CS by the current config
 *
 * Note: This synchronous function is used to release either the lock on the
 *       SPI device and/or the CS line that was kept if, and if only,
 *       given config parameter was the last one to be used (in any of the
 *       above functions) and if it has the SPI_LOCK_ON bit set and/or the
 *       SPI_HOLD_ON_CS bit set into its operation bits field.
 *       This can be used if the caller needs to keep its hand on the SPI
 *       device for consecutive transactions and/or if it needs the device to
 *       stay selected. Usually both bits will be used along each other, so the
 *       the device is locked and stays on until another operation is necessary
 *       or until it gets released with the present function.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code on failure.
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

/**
 * @brief Release the SPI device specified in @p spi_dt_spec.
 *
 * This is equivalent to:
 *
 *     spi_release(spec->bus, &spec->config);
 *
 * @param spec SPI specification from devicetree
 *
 * @return a value from spi_release().
 */
static inline int spi_release_dt(const struct spi_dt_spec *spec)
{
	return spi_release(spec->bus, &spec->config);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#include <syscalls/spi.h>

#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_H_ */
