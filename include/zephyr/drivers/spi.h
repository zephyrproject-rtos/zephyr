/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup spi_interface
 * @brief Main header file for SPI (Serial Peripheral Interface) driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SPI_H_
#define ZEPHYR_INCLUDE_DRIVERS_SPI_H_

/**
 * @brief Interfaces for Serial Peripheral Interface (SPI)
 *        controllers.
 * @defgroup spi_interface SPI
 * @since 1.0
 * @version 1.0.0
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/spi/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/rtio/rtio.h>
#include <zephyr/stats/stats.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name SPI operational mode
 * @{
 */

/**
 * @brief Master (controller) mode.
 *
 * In this case the device used with the API will function as a controller,
 * meaning it will control the CLK line on the SPI bus and the chip select,
 * and therefore have full control over the timing of the transaction.
 */
#define SPI_OP_MODE_MASTER	0U

/**
 * @brief Slave (peripheral) mode.
 *
 * With this mode, the device will function as a peripheral,
 * meaning it will need to wait for it's select line to be asserted,
 * and will be need to be subject to pacing by a controller's clock in order to
 * send and receive data during a transaction.
 */
#define SPI_OP_MODE_SLAVE	BIT(0)  /**< Slave mode. */

/** @cond INTERNAL_HIDDEN */
#define SPI_OP_MODE_MASK	0x1U
/** @endcond */

/**
 * @brief Get SPI Operational mode bitmask from a @ref spi_operation_t
 */
#define SPI_OP_MODE_GET(_operation_) ((_operation_) & SPI_OP_MODE_MASK)
/** @} */


/**
 * @name SPI Clock Modes
 * @{
 */

/**
 * @brief Clock Polarity (Clock Idle State)
 *
 * @details
 * Used in @ref spi_operation_t definition.
 * If set, clock idle state will be 1 and active state will be 0.
 * If unset, clock idle state will be 0 and active state will be 1.
 * Unset is the default.
 */
#define SPI_MODE_CPOL		BIT(1)

/**
 * @brief Clock Phase (Clock data capture edge)
 *
 * @details
 * Used in @ref spi_operation_t definition.
 * If set, data is captured on transition from active to idle CLK state.
 * If unset, data is captured on transition from idle to active state.
 * Unset is the default.
 */
#define SPI_MODE_CPHA		BIT(2)

/**
 * @brief Controller loopback mode
 *
 * @details
 * For testing purposes, enable hardware loopback,
 * which means that transmit data is fed back to the receiver of the same controller.
 *
 * Not all controllers support this feature.
 */
#define SPI_MODE_LOOP		BIT(3)

/** @cond INTERNAL_HIDDEN */
#define SPI_MODE_MASK		(0xEU)
/** @endcond */

/**
 * @brief Get SPI clock polarity and phase mode bitmask from a @ref spi_operation_t
 */
#define SPI_MODE_GET(_mode_)			\
	((_mode_) & SPI_MODE_MASK)

/** @} */


/**
 * @name SPI Data Word Configurations
 *
 * A SPI Data word is a value that is shifted in/out of the controller's hardware FIFO
 * and is the atomic unit of communication on the spi bus.
 * A word is also called a "data frame" in this API.
 * A transfer is made up of an arbitrary number of words.
 * The following options specify configurations of the SPI word for the operation.
 *
 * @{
 */

/** Words are most significant bit first, used for @ref spi_operation_t */
#define SPI_TRANSFER_MSB	(0U)
/** Words are least significant bit first, used for @ref spi_operation_t */
#define SPI_TRANSFER_LSB	BIT(4)

/** @cond INTERNAL_HIDDEN */
#define SPI_WORD_SIZE_SHIFT	(5U)
#define SPI_WORD_SIZE_MASK	(0x3FU << SPI_WORD_SIZE_SHIFT)
/** @endcond */

/**
 * @brief Get SPI word size in bits from a @ref spi_operation_t
 *
 * @param operation A @ref spi_operation_t from which to get the configured word size.
 * @retval The size (in bits) of a spi word for the operation.
 */
#define SPI_WORD_SIZE_GET(operation)					\
	(((operation) & SPI_WORD_SIZE_MASK) >> SPI_WORD_SIZE_SHIFT)

/**
 * @brief Get a bitmask to set the word size in a @ref spi_operation_t
 *
 * @param word_size The size of a SPI data frame in bits.
 * @retval A bitmask to apply to a @ref spi_operation_t
 */
#define SPI_WORD_SET(word_size)			\
	((word_size) << SPI_WORD_SIZE_SHIFT)

/** @} */


/**
 * @name SPI Transfer control flags
 * @{
 */

/**
 * @brief Keep chip select active after transaction
 *
 * After one of the spi transceive calls described in this API,
 * if this flag is set in the spi config operation, then
 * attempt to keep the CS active after the call, if supported and possible.
 */
#define SPI_HOLD_ON_CS		BIT(12)

/**
 * @brief Retain ownership of the spi device
 *
 * This is a software control parameter that will prevent the spi device
 * from being accessed by other API callers after the transaction,
 * and therefore should be used with caution.
 *
 * The identifying piece of information for who "locks" the device
 * is the spi_config pointer given to the transaction API, so this same
 * config should be re-used to do another transaction or release the lock.
 *
 * See @ref spi_release for how to release the  lock.
 */
#define SPI_LOCK_ON		BIT(13)

/**
 * @brief Chip select active state configuration
 *
 * If this flag is set, the CS will be active high.
 * If this flag is unset, the CS will be active low.
 *
 * Default is active low (unset) as that is most common for spi peripherals.
 *
 * Not all controllers are able to handle this natively, in which case a
 * gpio can still be used to control the CS through software with a @ref spi_cs_control
 */
#define SPI_CS_ACTIVE_HIGH	BIT(14)

/** @} */


/**
 * @name SPI MISO lines
 * @{
 *
 * Some controllers support dual, quad or octal MISO lines connected to slaves.
 * Default is single, which is the case most of the time.
 * Without @kconfig{CONFIG_SPI_EXTENDED_MODES} being enabled, single is the
 * only supported one.
 */
#define SPI_LINES_SINGLE	(0U << 16)     /**< Single line */
#define SPI_LINES_DUAL		(1U << 16)     /**< Dual lines */
#define SPI_LINES_QUAD		(2U << 16)     /**< Quad lines */
#define SPI_LINES_OCTAL		(3U << 16)     /**< Octal lines */

#define SPI_LINES_MASK		(0x3U << 16)   /**< Mask for MISO lines in spi_operation_t */

/** @} */

/**
 * @name SPI GPIO Chip Select control
 * @{
 */

/**
 * @brief SPI Chip Select control structure
 *
 * This can be used to control a CS line via a GPIO line, instead of
 * using the controller internal CS logic.
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
#define SPI_CS_GPIOS_DT_SPEC_GET(spi_dev)			\
	GPIO_DT_SPEC_GET_BY_IDX_OR(DT_BUS(spi_dev), cs_gpios,	\
				   DT_REG_ADDR_RAW(spi_dev), {})

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
 * @code{.devicetree}
 *     spi@abcd0001 {
 *             cs-gpios = <&gpio0 1 GPIO_ACTIVE_LOW>;
 *             spidev: spi-device@0 { ... };
 *     };
 * @endcode
 *
 * Example usage:
 *
 * @code{.c}
 *     struct spi_cs_control ctrl =
 *             SPI_CS_CONTROL_INIT(DT_NODELABEL(spidev), 2);
 * @endcode
 *
 * This example is equivalent to:
 *
 * @code{.c}
 *     struct spi_cs_control ctrl = {
 *             .gpio = SPI_CS_GPIOS_DT_SPEC_GET(DT_NODELABEL(spidev)),
 *             .delay = 2,
 *     };
 * @endcode
 *
 * @param node_id Devicetree node identifier for a device on a SPI bus
 * @param delay_ The @p delay field to set in the @p spi_cs_control
 * @return a pointer to the @p spi_cs_control structure
 */
#define SPI_CS_CONTROL_INIT(node_id, delay_)			  \
	{							  \
		.gpio = SPI_CS_GPIOS_DT_SPEC_GET(node_id),	  \
		.delay = (delay_),				  \
	}

/**
 * @brief Get a pointer to a @p spi_cs_control from a devicetree node
 *
 * This is equivalent to
 * <tt>SPI_CS_CONTROL_INIT(DT_DRV_INST(inst), delay)</tt>.
 *
 * Therefore, @p DT_DRV_COMPAT must already be defined before using
 * this macro.
 *
 * @param inst Devicetree node instance number
 * @param delay_ The @p delay field to set in the @p spi_cs_control
 * @return a pointer to the @p spi_cs_control structure
 */
#define SPI_CS_CONTROL_INIT_INST(inst, delay_)		\
	SPI_CS_CONTROL_INIT(DT_DRV_INST(inst), delay_)

/** @} */

/**
 * @typedef spi_operation_t
 * Opaque type to hold the SPI operation flags.
 */
#if defined(CONFIG_SPI_EXTENDED_MODES)
typedef uint32_t spi_operation_t;
#else
typedef uint16_t spi_operation_t;
#endif

/**
 * @brief SPI controller configuration structure
 *
 * @warning Most drivers use pointer comparison to determine whether a passed
 * configuration is different from one used in a previous transaction.
 * Changes to fields in the structure may not be detected.
 */
struct spi_config {
	/** @brief Bus frequency in Hertz. */
	uint32_t frequency;
	/**
	 * @brief Operation flags.
	 *
	 * It is a bit field with the following parts:
	 *
	 * - 0:      Master or slave.
	 * - 1..3:   Clock polarity, phase and loop mode.
	 * - 4:      LSB or MSB first.
	 * - 5..10:  Size of a data frame (word) in bits.
	 * - 11:     Full/half duplex.
	 * - 12:     Hold on the CS line if possible.
	 * - 13:     Keep resource locked for the caller.
	 * - 14:     Active high CS logic.
	 * - 15:     Motorola or TI frame format (optional).
	 *
	 * If @kconfig{CONFIG_SPI_EXTENDED_MODES} is enabled:
	 *
	 * - 16..17: MISO lines (Single/Dual/Quad/Octal).
	 * - 18..31: Reserved for future use.
	 */
	spi_operation_t operation;
	/** @brief Slave number from 0 to host controller slave limit. */
	uint16_t slave;
	/**
	 * @brief GPIO chip-select line (optional, must be initialized to zero
	 * if not used).
	 */
	struct spi_cs_control cs;
};

/**
 * @brief Structure initializer for spi_config from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * spi_config</tt> by reading the relevant @p frequency, @p slave, and
 * @p cs data from the devicetree.
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
			DT_PROP(node_id, frame_format) |			\
			COND_CODE_1(DT_PROP(node_id, spi_cpol), SPI_MODE_CPOL, (0)) |	\
			COND_CODE_1(DT_PROP(node_id, spi_cpha), SPI_MODE_CPHA, (0)) |	\
			COND_CODE_1(DT_PROP(node_id, spi_hold_cs), SPI_HOLD_ON_CS, (0)),	\
		.slave = DT_REG_ADDR(node_id),				\
		.cs = SPI_CS_CONTROL_INIT(node_id, delay_),		\
	}

/**
 * @brief Structure initializer for spi_config from devicetree instance
 *
 * This is equivalent to
 * <tt>SPI_CONFIG_DT(DT_DRV_INST(inst), operation_, delay_)</tt>.
 *
 * @param inst Devicetree instance number
 * @param operation_ the desired @p operation field in the struct spi_config
 * @param delay_ the desired @p delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define SPI_CONFIG_DT_INST(inst, operation_, delay_)	\
	SPI_CONFIG_DT(DT_DRV_INST(inst), operation_, delay_)

/**
 * @brief Complete SPI DT information
 */
struct spi_dt_spec {
	/** SPI bus */
	const struct device *bus;
	/** Slave specific configuration */
	struct spi_config config;
};

/**
 * @brief Structure initializer for spi_dt_spec from devicetree
 *
 * This helper macro expands to a static initializer for a <tt>struct
 * spi_dt_spec</tt> by reading the relevant bus, frequency, slave, and cs
 * data from the devicetree.
 *
 * Important: multiple fields are automatically constructed by this macro
 * which must be checked before use. @ref spi_is_ready_dt performs the required
 * @ref device_is_ready checks.
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
 * @param inst Devicetree instance number
 * @param operation_ the desired @p operation field in the struct spi_config
 * @param delay_ the desired @p delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define SPI_DT_SPEC_INST_GET(inst, operation_, delay_) \
	SPI_DT_SPEC_GET(DT_DRV_INST(inst), operation_, delay_)

/**
 * @brief Value that will never compare true with any valid overrun character
 */
#define SPI_MOSI_OVERRUN_UNKNOWN 0x100

/**
 * @brief The value sent on MOSI when all TX bytes are sent, but RX continues
 *
 * For drivers where the MOSI line state when receiving is important, this value
 * can be queried at compile-time to determine whether allocating a constant
 * array is necessary.
 *
 * @param node_id Devicetree node identifier for the SPI device to query
 *
 * @retval SPI_MOSI_OVERRUN_UNKNOWN if controller does not export the value
 * @retval byte default MOSI value otherwise
 */
#define SPI_MOSI_OVERRUN_DT(node_id) \
	DT_PROP_OR(node_id, overrun_character, SPI_MOSI_OVERRUN_UNKNOWN)

/**
 * @brief The value sent on MOSI when all TX bytes are sent, but RX continues
 *
 * This is equivalent to
 * <tt>SPI_MOSI_OVERRUN_DT(DT_DRV_INST(inst))</tt>.
 *
 * @param inst Devicetree instance number
 *
 * @retval SPI_MOSI_OVERRUN_UNKNOWN if controller does not export the value
 * @retval byte default MOSI value otherwise
 */
#define SPI_MOSI_OVERRUN_DT_INST(inst) \
	DT_INST_PROP_OR(inst, overrun_character, SPI_MOSI_OVERRUN_UNKNOWN)

/**
 * @brief SPI buffer structure
 *
 * A SPI buffer describes either a real data buffer or an indication of NOP
 * For a NOP indicator:
 *   If buffer is used for TX, only 0's will be sent for the length on the bus
 *   If buffer is used for RX, that length of data received by bus will be ignored/skipped
 */
struct spi_buf {
	/** Valid pointer to a data buffer, or NULL for NOP indication */
	void *buf;
	/** Length of the buffer @a buf in bytes, or length of NOP */
	size_t len;
};

/**
 * @brief SPI scatter-gather buffer array structure
 *
 * A spi_buf_set is a flexible description of a whole single SPI bus transfer.
 *
 * Since the set is an array of pointers to buffers, it means that pieces of a spi transfer
 * definition can be re-used across different transfers, without having to redefine or allocate
 * new memory for them each time.
 * This accomplishes what is called "scatter-gather" buffer management at the driver level with
 * user-provided buffers.
 */
struct spi_buf_set {
	/** Pointer to an array of spi_buf, or NULL */
	const struct spi_buf *buffers;
	/** Number of buffers in the array pointed to: by @a buffers */
	size_t count;
};

/**
 * @name SPI Stats
 * @{
 */
#if defined(CONFIG_SPI_STATS)
STATS_SECT_START(spi)
STATS_SECT_ENTRY32(rx_bytes)
STATS_SECT_ENTRY32(tx_bytes)
STATS_SECT_ENTRY32(transfer_error)
STATS_SECT_END;

STATS_NAME_START(spi)
STATS_NAME(spi, rx_bytes)
STATS_NAME(spi, tx_bytes)
STATS_NAME(spi, transfer_error)
STATS_NAME_END(spi);

/**
 * @brief SPI specific device state which allows for SPI device class specific additions
 */
struct spi_device_state {
	struct device_state devstate;
	struct stats_spi stats;
};

/**
 * @brief Get pointer to SPI statistics structure
 */
#define Z_SPI_GET_STATS(dev_)				\
	CONTAINER_OF(dev_->state, struct spi_device_state, devstate)->stats

/**
 * @brief Increment the rx bytes for a SPI device
 *
 * @param dev_ Pointer to the device structure for the driver instance.
 */
#define SPI_STATS_RX_BYTES_INCN(dev_, n)			\
	STATS_INCN(Z_SPI_GET_STATS(dev_), rx_bytes, n)

/**
 * @brief Increment the tx bytes for a SPI device
 *
 * @param dev_ Pointer to the device structure for the driver instance.
 */
#define SPI_STATS_TX_BYTES_INCN(dev_, n)			\
	STATS_INCN(Z_SPI_GET_STATS(dev_), tx_bytes, n)

/**
 * @brief Increment the transfer error counter for a SPI device
 *
 * The transfer error count is incremented when there occurred a transfer error
 *
 * @param dev_ Pointer to the device structure for the driver instance.
 */
#define SPI_STATS_TRANSFER_ERROR_INC(dev_)			\
	STATS_INC(Z_SPI_GET_STATS(dev_), transfer_error)

/** @cond INTERNAL_HIDDEN */
/**
 * @brief Define a statically allocated and section assigned SPI device state
 */
#define Z_SPI_DEVICE_STATE_DEFINE(dev_id)	\
	static struct spi_device_state Z_DEVICE_STATE_NAME(dev_id)	\
	__attribute__((__section__(".z_devstate")));

/**
 * @brief Define an SPI device init wrapper function
 *
 * This does device instance specific initialization of common data (such as stats)
 * and calls the given init_fn
 */
#define Z_SPI_INIT_FN(dev_id, init_fn)					\
	static inline int UTIL_CAT(dev_id, _init)(const struct device *dev) \
	{								\
		struct spi_device_state *state =			\
			CONTAINER_OF(dev->state, struct spi_device_state, devstate); \
		stats_init(&state->stats.s_hdr, STATS_SIZE_32, 3,	\
			   STATS_NAME_INIT_PARMS(spi));			\
		stats_register(dev->name, &(state->stats.s_hdr));	\
		return init_fn(dev);					\
	}
/** @endcond */

#define SPI_DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, deinit_fn,	\
				    pm_device, data_ptr, cfg_ptr,	\
				    level, prio, api_ptr, ...)		\
	Z_SPI_DEVICE_STATE_DEFINE(Z_DEVICE_DT_DEV_ID(node_id));		\
	Z_SPI_INIT_FN(Z_DEVICE_DT_DEV_ID(node_id), init_fn)		\
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id),		\
			DEVICE_DT_NAME(node_id),			\
			&UTIL_CAT(Z_DEVICE_DT_DEV_ID(node_id), _init),	\
			deinit_fn, Z_DEVICE_DT_FLAGS(node_id),		\
			pm_device, data_ptr, cfg_ptr, level, prio,	\
			api_ptr,					\
			&(Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_ID(node_id)).devstate), \
			__VA_ARGS__)

static inline void spi_transceive_stats(const struct device *dev, int error,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	uint32_t tx_bytes;
	uint32_t rx_bytes;

	if (error) {
		SPI_STATS_TRANSFER_ERROR_INC(dev);
	}

	if (tx_bufs) {
		tx_bytes = tx_bufs->count ? tx_bufs->buffers->len : 0;
		SPI_STATS_TX_BYTES_INCN(dev, tx_bytes);
	}

	if (rx_bufs) {
		rx_bytes = rx_bufs->count ? rx_bufs->buffers->len : 0;
		SPI_STATS_RX_BYTES_INCN(dev, rx_bytes);
	}
}
/** @} */

#else /*CONFIG_SPI_STATS*/

/**
 * @name SPI DT Device Macros
 * @{
 */

/**
 * @brief Like DEVICE_DT_DEINIT_DEFINE() with SPI specifics.
 *
 * @details Defines a device which implements the SPI API. May
 * generate a custom device_state container struct and init_fn
 * wrapper when needed depending on SPI @kconfig{CONFIG_SPI_STATS}.
 *
 * @param node_id The devicetree node identifier.
 * @param init_fn Name of the init function of the driver.
 * @param deinit_fn Name of the deinit function of the driver.
 * @param pm PM device resources reference (NULL if device does not use PM).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the configuration
 *                information for this instance of the driver.
 * @param level The initialization level. See SYS_INIT() for details.
 * @param prio Priority within the selected initialization level. See SYS_INIT()
 *             for details.
 * @param api Provides an initial pointer to the API function struct used by
 *                the driver. Can be NULL.
 */
#define SPI_DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, deinit_fn, pm, data,	\
				    config, level, prio, api, ...)		\
	Z_DEVICE_STATE_DEFINE(Z_DEVICE_DT_DEV_ID(node_id));			\
	Z_DEVICE_DEFINE(node_id, Z_DEVICE_DT_DEV_ID(node_id),			\
			DEVICE_DT_NAME(node_id), init_fn, deinit_fn,		\
			Z_DEVICE_DT_FLAGS(node_id), pm, data, config,		\
			level, prio, api,					\
			&Z_DEVICE_STATE_NAME(Z_DEVICE_DT_DEV_ID(node_id)),	\
			__VA_ARGS__)

/** @} */

#define SPI_STATS_RX_BYTES_INC(dev_)
#define SPI_STATS_TX_BYTES_INC(dev_)
#define SPI_STATS_TRANSFER_ERROR_INC(dev_)

#define spi_transceive_stats(dev, error, tx_bufs, rx_bufs)

#endif /*CONFIG_SPI_STATS*/

/**
 * @brief Like DEVICE_DT_DEINIT_DEFINE() without deinit function.
 *
 * @details Defines a device which implements the SPI API. May
 * generate a custom device_state container struct and init_fn
 * wrapper when needed depending on SPI @kconfig{CONFIG_SPI_STATS}.
 *
 * @param node_id The devicetree node identifier.
 * @param init_fn Name of the init function of the driver.
 * @param pm PM device resources reference (NULL if device does not use PM).
 * @param data Pointer to the device's private data.
 * @param config The address to the structure containing the configuration
 *                information for this instance of the driver.
 * @param level The initialization level. See SYS_INIT() for details.
 * @param prio Priority within the selected initialization level. See SYS_INIT()
 *             for details.
 * @param api Provides an initial pointer to the API function struct used by
 *                the driver. Can be NULL.
 */
#define SPI_DEVICE_DT_DEFINE(node_id, init_fn, pm, data, config, level, prio,	\
			     api, ...)						\
	SPI_DEVICE_DT_DEINIT_DEFINE(node_id, init_fn, NULL, pm, data, config,	\
				    level, prio, api, __VA_ARGS__)

/**
 * @brief Like SPI_DEVICE_DT_DEINIT_DEFINE(), but uses an instance of a `DT_DRV_COMPAT`
 * compatible instead of a node identifier.
 *
 * @param inst Instance number. The `node_id` argument to SPI_DEVICE_DT_DEINIT_DEFINE() is
 * set to `DT_DRV_INST(inst)`.
 * @param ... Other parameters as expected by SPI_DEVICE_DT_DEFINE().
 */
#define SPI_DEVICE_DT_INST_DEINIT_DEFINE(inst, ...) \
	SPI_DEVICE_DT_DEINIT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

/**
 * @brief Like SPI_DEVICE_DT_DEFINE(), but uses an instance of a `DT_DRV_COMPAT`
 * compatible instead of a node identifier.
 *
 * @param inst Instance number. The `node_id` argument to SPI_DEVICE_DT_DEFINE() is
 * set to `DT_DRV_INST(inst)`.
 * @param ... Other parameters as expected by SPI_DEVICE_DT_DEFINE().
 */
#define SPI_DEVICE_DT_INST_DEFINE(inst, ...)                                       \
	SPI_DEVICE_DT_DEFINE(DT_DRV_INST(inst), __VA_ARGS__)

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
 * See spi_transceive_signal() for argument descriptions
 */
typedef int (*spi_api_io_async)(const struct device *dev,
				const struct spi_config *config,
				const struct spi_buf_set *tx_bufs,
				const struct spi_buf_set *rx_bufs,
				spi_callback_t cb,
				void *userdata);

#if defined(CONFIG_SPI_RTIO) || defined(DOXYGEN)

/**
 * @typedef spi_api_iodev_submit
 * @brief Callback API for submitting work to a SPI device with RTIO
 */
typedef void (*spi_api_iodev_submit)(const struct device *dev,
				     struct rtio_iodev_sqe *iodev_sqe);
#endif /* CONFIG_SPI_RTIO */

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
#ifdef CONFIG_SPI_RTIO
	spi_api_iodev_submit iodev_submit;
#endif /* CONFIG_SPI_RTIO */
	spi_api_release release;
};

/**
 * @brief Check if SPI CS is controlled using a GPIO.
 *
 * @param config SPI configuration.
 * @return true If CS is controlled using a GPIO.
 * @return false If CS is controlled by hardware or any other means.
 */
static inline bool spi_cs_is_gpio(const struct spi_config *config)
{
	return config->cs.gpio.port != NULL;
}

/**
 * @brief Check if SPI CS in @ref spi_dt_spec is controlled using a GPIO.
 *
 * @param spec SPI specification from devicetree.
 * @return true If CS is controlled using a GPIO.
 * @return false If CS is controlled by hardware or any other means.
 */
static inline bool spi_cs_is_gpio_dt(const struct spi_dt_spec *spec)
{
	return spi_cs_is_gpio(&spec->config);
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
	if (spi_cs_is_gpio_dt(spec) &&
	    !gpio_is_ready_dt(&spec->config.cs.gpio)) {
		return false;
	}
	return true;
}

/**
 * @name SPI Synchronous Transfer Functions
 *
 * These functions will not return until transfer is complete
 *
 * @{
 */

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is synchronous.
 *
 * @note In master mode, the chip select line will remain asserted (active) for
 *       the entire duration of the transfer of all buffers in the provided buf sets.
 *       Only after all buffers have been transferred will CS be deasserted.
 *
 * @note In peripheral mode, data transfer happens when the master asserts CS and
 *       provides the clock. The function will wait for the master to complete
 *       the transfer before returning. The CS is controlled by master
 *       and therefore may not be continuously asserted for the whole transfer.
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
 * @retval -ENOTSUP means some part of the spi config is not supported either by the
 *	   device hardware or the driver software.
 * @retval -EINVAL means that some parameter of the spi_config is invalid.
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
	int ret;

	ret = api->transceive(dev, config, tx_bufs, rx_bufs);
	spi_transceive_stats(dev, ret, tx_bufs, rx_bufs);

	return ret;
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
 * @note This function is a helper function calling spi_transceive.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param rx_bufs Buffer array where data to be read will be written to.
 *
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful.
 * @retval -ENOTSUP means some part of the spi config is not supported either by the
 *	   device hardware or the driver software.
 * @retval -EINVAL means that some parameter of the spi_config is invalid.
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
 * @note This function is a helper function calling spi_transceive.
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param config Pointer to a valid spi_config structure instance.
 *        Pointer-comparison may be used to detect changes from
 *        previous operations.
 * @param tx_bufs Buffer array where data to be sent originates from.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP means some part of the spi config is not supported either by the
 *	   device hardware or the driver software.
 * @retval -EINVAL means that some parameter of the spi_config is invalid.
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
/** @} */

#if defined(CONFIG_SPI_ASYNC) || defined(__DOXYGEN__)
/**
 * @name SPI Asynchronous Transfer Functions
 *
 * With this API the transfer function will return after the transfer is started and
 * report completion through a notification mechanism: callback or signal.
 *
 * @note Note that asynchronous API calls can still be blocking if the bus is already busy.
 *       The functions will block until the bus is available to start the requested transfer.
 *
 * @{
 */

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is available only if @kconfig{CONFIG_SPI_ASYNC}
 * is selected.
 *
 * @note The chip select behavior as described by @ref spi_transceive and
 *       the function of controller/peripheral modes is the same.
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
 * @retval -ENOTSUP means some part of the spi config is not supported either by the
 *	   device hardware or the driver software.
 * @retval -EINVAL means that some parameter of the spi_config is invalid.
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

#if defined(CONFIG_POLL) || defined(__DOXYGEN__)

/** @cond INTERNAL_HIDDEN */
void z_spi_transfer_signal_cb(const struct device *dev, int result, void *userdata);
/** @endcond */

/**
 * @brief Read/write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note The chip select behavior as described by @ref spi_transceive and
 *       the function of controller/peripheral modes is the same.
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
 * @retval -ENOTSUP means some part of the spi config is not supported either by the
 *	   device hardware or the driver software.
 * @retval -EINVAL means that some parameter of the spi_config is invalid.
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
 * @brief Read the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is a helper function calling spi_transceive_signal.
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
 * @retval frames Positive number of frames received in slave mode.
 * @retval 0 If successful
 * @retval -ENOTSUP means some part of the spi config is not supported either by the
 *	   device hardware or the driver software.
 * @retval -EINVAL means that some parameter of the spi_config is invalid.
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
 * @brief Write the specified amount of data from the SPI driver.
 *
 * @note This function is asynchronous.
 *
 * @note This function is a helper function calling spi_transceive_signal.
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
 * @retval -ENOTSUP means some part of the spi config is not supported either by the
 *	   device hardware or the driver software.
 * @retval -EINVAL means that some parameter of the spi_config is invalid.
 * @retval -errno Negative errno code on failure.
 */
static inline int spi_write_signal(const struct device *dev,
				  const struct spi_config *config,
				  const struct spi_buf_set *tx_bufs,
				  struct k_poll_signal *sig)
{
	return spi_transceive_signal(dev, config, tx_bufs, NULL, sig);
}

#endif /* CONFIG_POLL */

/** @} */
#endif /* CONFIG_SPI_ASYNC */


#if defined(CONFIG_SPI_RTIO) || defined(__DOXYGEN__)

/**
 * @name SPI RTIO API
 *
 * Theses functions are for using the SPI driver class through an RTIO-based API
 *
 * @{
 */
/**
 * @brief Submit a SPI device with a request
 *
 * @param iodev_sqe Prepared submissions queue entry connected to an iodev
 *                  defined by SPI_IODEV_DEFINE.
 *                  Must live as long as the request is in flight.
 */
static inline void spi_iodev_submit(struct rtio_iodev_sqe *iodev_sqe)
{
	const struct spi_dt_spec *dt_spec = (const struct spi_dt_spec *)iodev_sqe->sqe.iodev->data;
	const struct device *dev = dt_spec->bus;
	const struct spi_driver_api *api = (const struct spi_driver_api *)dev->api;

	api->iodev_submit(dt_spec->bus, iodev_sqe);
}

/** @cond INTERNAL_HIDDEN */
extern const struct rtio_iodev_api spi_iodev_api;
/** @endcond */

/**
 * @brief Define an iodev for a given dt node on the bus
 *
 * These do not need to be shared globally but doing so
 * will save a small amount of memory.
 *
 * @param name Symbolic name to use for defining the iodev
 * @param node_id Devicetree node identifier
 * @param operation_ SPI operational mode
 * @param delay_ Chip select delay in microseconds
 */
#define SPI_DT_IODEV_DEFINE(name, node_id, operation_, delay_)			\
	const struct spi_dt_spec _spi_dt_spec_##name =				\
		SPI_DT_SPEC_GET(node_id, operation_, delay_);			\
	RTIO_IODEV_DEFINE(name, &spi_iodev_api, (void *)&_spi_dt_spec_##name)

/**
 * @brief Validate that SPI bus (and CS gpio if defined) is ready.
 *
 * @param spi_iodev SPI iodev defined with SPI_DT_IODEV_DEFINE
 *
 * @retval true if the SPI bus is ready for use.
 * @retval false if the SPI bus (or the CS gpio defined) is not ready for use.
 */
static inline bool spi_is_ready_iodev(const struct rtio_iodev *spi_iodev)
{
	struct spi_dt_spec *spec = (struct spi_dt_spec *)spi_iodev->data;

	return spi_is_ready_dt(spec);
}
/** @} */

#endif /* CONFIG_SPI_RTIO */

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

#include <zephyr/syscalls/spi.h>

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_DRIVERS_SPI_H_ */
