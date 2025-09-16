/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mipi_dbi_interface
 * @brief Main header file for MIPI-DBI (Display Bus Interface) driver API.
 *
 * MIPI-DBI defines the following 3 interfaces:
 * - Type A: Motorola 6800 type parallel bus
 * - Type B: Intel 8080 type parallel bus
 * - Type C: SPI Type (1 bit bus) with 3 options:
 *     1. 9 write clocks per byte, final bit is command/data selection bit
 *     2. Same as above, but 16 write clocks per byte
 *     3. 8 write clocks per byte. Command/data selected via GPIO pin
 * The current driver interface does not support type C with 16 write clocks (option 2).
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MIPI_DBI_H_
#define ZEPHYR_INCLUDE_DRIVERS_MIPI_DBI_H_

/**
 * @brief Interfaces for MIPI-DBI (Display Bus Interface).
 * @defgroup mipi_dbi_interface MIPI-DBI
 * @since 3.6
 * @version 0.8.0
 * @ingroup display_interface
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/mipi_display.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/dt-bindings/mipi_dbi/mipi_dbi.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief initialize a MIPI DBI SPI configuration struct from devicetree
 *
 * This helper allows drivers to initialize a MIPI DBI SPI configuration
 * structure using devicetree.
 * @param node_id Devicetree node identifier for the MIPI DBI device whose
 *                struct spi_config to create an initializer for
 * @param operation_ the desired operation field in the struct spi_config
 * @param delay_ the desired delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define MIPI_DBI_SPI_CONFIG_DT(node_id, operation_, delay_)		\
	{								\
		.frequency = DT_PROP(node_id, mipi_max_frequency),	\
		.operation = (operation_) |				\
			DT_PROP_OR(node_id, duplex, 0) |			\
			COND_CODE_1(DT_PROP(node_id, mipi_cpol), SPI_MODE_CPOL, (0)) |	\
			COND_CODE_1(DT_PROP(node_id, mipi_cpha), SPI_MODE_CPHA, (0)) |	\
			COND_CODE_1(DT_PROP(node_id, mipi_hold_cs), SPI_HOLD_ON_CS, (0)),	\
		.slave = DT_REG_ADDR(node_id),				\
		.cs = {							\
			.gpio = GPIO_DT_SPEC_GET_BY_IDX_OR(DT_PHANDLE(DT_PARENT(node_id), \
							   spi_dev), cs_gpios, \
							   DT_REG_ADDR_RAW(node_id), \
							   {}),		\
			.delay = (delay_),				\
		},							\
	}

/**
 * @brief Initialize a MIPI DBI SPI configuration from devicetree instance
 *
 * This helper initializes a MIPI DBI SPI configuration from a devicetree
 * instance. It is equivalent to MIPI_DBI_SPI_CONFIG_DT(DT_DRV_INST(inst))
 * @param inst Instance number to initialize configuration from
 * @param operation_ the desired operation field in the struct spi_config
 * @param delay_ the desired delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define MIPI_DBI_SPI_CONFIG_DT_INST(inst, operation_, delay_)		\
	MIPI_DBI_SPI_CONFIG_DT(DT_DRV_INST(inst), operation_, delay_)

/**
 * @brief Initialize a MIPI DBI configuration from devicetree
 *
 * This helper allows drivers to initialize a MIPI DBI configuration
 * structure from devicetree. It sets the MIPI DBI mode, as well
 * as configuration fields in the SPI configuration structure
 * @param node_id Devicetree node identifier for the MIPI DBI device to
 *                initialize
 * @param operation_ the desired operation field in the struct spi_config
 * @param delay_ the desired delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define MIPI_DBI_CONFIG_DT(node_id, operation_, delay_)			\
	{								\
		.mode = DT_STRING_UPPER_TOKEN(node_id, mipi_mode),	\
		.config = MIPI_DBI_SPI_CONFIG_DT(node_id, operation_, delay_), \
	}

/**
 * @brief Initialize a MIPI DBI configuration from device instance
 *
 * Equivalent to MIPI_DBI_CONFIG_DT(DT_DRV_INST(inst), operation_, delay_)
 * @param inst Instance of the device to initialize a MIPI DBI configuration for
 * @param operation_ the desired operation field in the struct spi_config
 * @param delay_ the desired delay field in the struct spi_config's
 *               spi_cs_control, if there is one
 */
#define MIPI_DBI_CONFIG_DT_INST(inst, operation_, delay_)		\
	MIPI_DBI_CONFIG_DT(DT_DRV_INST(inst), operation_, delay_)

/**
 * @brief Get the MIPI DBI TE mode from devicetree
 *
 * Gets the MIPI DBI TE mode from a devicetree property.
 * @param node_id Devicetree node identifier for the MIPI DBI device with the
 *                TE mode property
 * @param edge_prop Property name for the TE mode that should be read from
 *                  devicetree
 */
#define MIPI_DBI_TE_MODE_DT(node_id, edge_prop)                           \
	DT_STRING_UPPER_TOKEN(node_id, edge_prop)

/**
 * @brief Get the MIPI DBI TE mode for device instance
 *
 * Gets the MIPI DBI TE mode from a devicetree property. Equivalent to
 * MIPI_DBI_TE_MODE_DT(DT_DRV_INST(inst), edge_mode).
 * @param inst Instance of the device to get the TE mode for
 * @param edge_prop Property name for the TE mode that should be read from
 *                  devicetree
 */
#define MIPI_DBI_TE_MODE_DT_INST(inst, edge_prop)                         \
	DT_STRING_UPPER_TOKEN(DT_DRV_INST(inst), edge_prop)

/**
 * @brief MIPI DBI controller configuration
 *
 * Configuration for MIPI DBI controller write
 */
struct mipi_dbi_config {
	/** MIPI DBI mode */
	uint8_t mode;
	/** SPI configuration */
	struct spi_config config;
};


/** MIPI-DBI host driver API */
__subsystem struct mipi_dbi_driver_api {
	int (*command_write)(const struct device *dev,
			     const struct mipi_dbi_config *config, uint8_t cmd,
			     const uint8_t *data, size_t len);
	int (*command_read)(const struct device *dev,
			    const struct mipi_dbi_config *config, uint8_t *cmds,
			    size_t num_cmds, uint8_t *response, size_t len);
	int (*write_display)(const struct device *dev,
			     const struct mipi_dbi_config *config,
			     const uint8_t *framebuf,
			     struct display_buffer_descriptor *desc,
			     enum display_pixel_format pixfmt);
	int (*reset)(const struct device *dev, k_timeout_t delay);
	int (*release)(const struct device *dev,
		       const struct mipi_dbi_config *config);
	int (*configure_te)(const struct device *dev,
			    uint8_t edge,
			    k_timeout_t delay);
};

/**
 * @brief Write a command to the display controller
 *
 * Writes a command, along with an optional data buffer to the display.
 * If data buffer and buffer length are NULL and 0 respectively, then
 * only a command will be sent. Note that if the SPI configuration passed
 * to this function locks the SPI bus, it is the caller's responsibility
 * to release it with mipi_dbi_release()
 *
 * @param dev mipi dbi controller
 * @param config MIPI DBI configuration
 * @param cmd command to write to display controller
 * @param data optional data buffer to write after command
 * @param len size of data buffer in bytes. Set to 0 to skip sending data.
 * @retval 0 command write succeeded
 * @retval -EIO I/O error
 * @retval -ETIMEDOUT transfer timed out
 * @retval -EBUSY controller is busy
 * @retval -ENOSYS not implemented
 */
static inline int mipi_dbi_command_write(const struct device *dev,
					 const struct mipi_dbi_config *config,
					 uint8_t cmd, const uint8_t *data,
					 size_t len)
{
	const struct mipi_dbi_driver_api *api =
		(const struct mipi_dbi_driver_api *)dev->api;

	if (api->command_write == NULL) {
		return -ENOSYS;
	}
	return api->command_write(dev, config, cmd, data, len);
}

/**
 * @brief Read a command response from the display controller
 *
 * Reads a command response from the display controller.
 *
 * @param dev mipi dbi controller
 * @param config MIPI DBI configuration
 * @param cmds array of one byte commands to send to display controller
 * @param num_cmd number of commands to write to display controller
 * @param response response buffer, filled with display controller response
 * @param len size of response buffer in bytes.
 * @retval 0 command read succeeded
 * @retval -EIO I/O error
 * @retval -ETIMEDOUT transfer timed out
 * @retval -EBUSY controller is busy
 * @retval -ENOSYS not implemented
 */
static inline int mipi_dbi_command_read(const struct device *dev,
					const struct mipi_dbi_config *config,
					uint8_t *cmds, size_t num_cmd,
					uint8_t *response, size_t len)
{
	const struct mipi_dbi_driver_api *api =
		(const struct mipi_dbi_driver_api *)dev->api;

	if (api->command_read == NULL) {
		return -ENOSYS;
	}
	return api->command_read(dev, config, cmds, num_cmd, response, len);
}

/**
 * @brief Write a display buffer to the display controller.
 *
 * Writes a display buffer to the controller. If the controller requires
 * a "Write memory" command before writing display data, this should be
 * sent with @ref mipi_dbi_command_write
 * @param dev mipi dbi controller
 * @param config MIPI DBI configuration
 * @param framebuf: framebuffer to write to display
 * @param desc: descriptor of framebuffer to write. Note that the pitch must
 *   be equal to width. "buf_size" field determines how many bytes will be
 *   written.
 * @param pixfmt: pixel format of framebuffer data
 * @retval 0 buffer write succeeded.
 * @retval -EIO I/O error
 * @retval -ETIMEDOUT transfer timed out
 * @retval -EBUSY controller is busy
 * @retval -ENOSYS not implemented
 */
static inline int mipi_dbi_write_display(const struct device *dev,
					 const struct mipi_dbi_config *config,
					 const uint8_t *framebuf,
					 struct display_buffer_descriptor *desc,
					 enum display_pixel_format pixfmt)
{
	const struct mipi_dbi_driver_api *api =
		(const struct mipi_dbi_driver_api *)dev->api;

	if (api->write_display == NULL) {
		return -ENOSYS;
	}
	return api->write_display(dev, config, framebuf, desc, pixfmt);
}

/**
 * @brief Resets attached display controller
 *
 * Resets the attached display controller.
 * @param dev mipi dbi controller
 * @param delay_ms duration to set reset signal for, in milliseconds
 * @retval 0 reset succeeded
 * @retval -EIO I/O error
 * @retval -ENOSYS not implemented
 * @retval -ENOTSUP not supported
 */
static inline int mipi_dbi_reset(const struct device *dev, uint32_t delay_ms)
{
	const struct mipi_dbi_driver_api *api =
		(const struct mipi_dbi_driver_api *)dev->api;

	if (api->reset == NULL) {
		return -ENOSYS;
	}
	return api->reset(dev, K_MSEC(delay_ms));
}

/**
 * @brief Releases a locked MIPI DBI device.
 *
 * Releases a lock on a MIPI DBI device and/or the device's CS line if and
 * only if the given config parameter was the last one to be used in any
 * of the above functions, and if it has the SPI_LOCK_ON bit set and/or
 * the SPI_HOLD_ON_CS bit set into its operation bits field.
 * This lock functions exactly like the SPI lock, and can be used if the caller
 * needs to keep CS asserted for multiple transactions, or the MIPI DBI device
 * locked.
 * @param dev mipi dbi controller
 * @param config MIPI DBI configuration
 * @retval 0 reset succeeded
 * @retval -EIO I/O error
 * @retval -ENOSYS not implemented
 * @retval -ENOTSUP not supported
 */
static inline int mipi_dbi_release(const struct device *dev,
				   const struct mipi_dbi_config *config)
{
	const struct mipi_dbi_driver_api *api =
		(const struct mipi_dbi_driver_api *)dev->api;

	if (api->release == NULL) {
		return -ENOSYS;
	}
	return api->release(dev, config);
}

/**
 * @brief Configures MIPI DBI tearing effect signal
 *
 * Many displays provide a tearing effect signal, which can be configured
 * to pulse at each vsync interval or each hsync interval. This signal can be
 * used by the MCU to determine when to transmit a new frame so that the
 * read pointer of the display never overlaps with the write pointer from the
 * MCU. This function configures the MIPI DBI controller to delay transmitting
 * display frames until the selected tearing effect signal edge occurs.
 *
 * The delay will occur on the on each call to @ref mipi_dbi_write_display
 * where the ``frame_incomplete`` flag was set within the buffer descriptor
 * provided with the prior call, as this indicates the buffer being written
 * in this call is the first buffer of a new frame.
 *
 * Note that most display controllers will need to enable the TE signal
 * using vendor specific commands before the MIPI DBI controller can react
 * to it.
 *
 * @param dev mipi dbi controller
 * @param edge which edge of the TE signal to start transmitting on
 * @param delay_us how many microseconds after TE edge to start transmission
 * @retval -EIO I/O error
 * @retval -ENOSYS not implemented
 * @retval -ENOTSUP not supported
 */
static inline int mipi_dbi_configure_te(const struct device *dev,
					uint8_t edge,
					uint32_t delay_us)
{
	const struct mipi_dbi_driver_api *api =
		(const struct mipi_dbi_driver_api *)dev->api;

	if (api->configure_te == NULL) {
		return -ENOSYS;
	}
	return api->configure_te(dev, edge, K_USEC(delay_us));
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MIPI_DBI_H_ */
