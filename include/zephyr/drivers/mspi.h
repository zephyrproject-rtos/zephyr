/*
 * Copyright (c) 2025, Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup mspi_interface
 * @brief Main header file for MSPI (Multi-Master Serial Peripheral Interface) driver API.
 */

#ifndef ZEPHYR_INCLUDE_MSPI_H_
#define ZEPHYR_INCLUDE_MSPI_H_

#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Interfaces for Multi-bit Serial Peripheral Interface (MSPI)
 *        controllers.
 * @defgroup mspi_interface MSPI
 * @since 3.7
 * @version 0.1.0
 * @ingroup io_interfaces
 * @{
 */

/**
 * @brief MSPI operational mode
 */
enum mspi_op_mode {
	MSPI_OP_MODE_CONTROLLER     = 0,
	MSPI_OP_MODE_PERIPHERAL     = 1,
};

/**
 * @brief MSPI duplex mode
 */
enum mspi_duplex {
	MSPI_HALF_DUPLEX            = 0,
	MSPI_FULL_DUPLEX            = 1,
};

/**
 * @brief MSPI I/O mode capabilities
 * Postfix like 1_4_4 stands for the number of lines used for
 * command, address and data phases.
 * Mode with no postfix has the same number of lines for all phases.
 */
enum mspi_io_mode {
	MSPI_IO_MODE_SINGLE         = 0,
	MSPI_IO_MODE_DUAL           = 1,
	MSPI_IO_MODE_DUAL_1_1_2     = 2,
	MSPI_IO_MODE_DUAL_1_2_2     = 3,
	MSPI_IO_MODE_QUAD           = 4,
	MSPI_IO_MODE_QUAD_1_1_4     = 5,
	MSPI_IO_MODE_QUAD_1_4_4     = 6,
	MSPI_IO_MODE_OCTAL          = 7,
	MSPI_IO_MODE_OCTAL_1_1_8    = 8,
	MSPI_IO_MODE_OCTAL_1_8_8    = 9,
	MSPI_IO_MODE_HEX            = 10,
	MSPI_IO_MODE_HEX_8_8_16     = 11,
	MSPI_IO_MODE_HEX_8_16_16    = 12,
	MSPI_IO_MODE_MAX,
};

/**
 * @brief MSPI data rate capabilities
 * SINGLE stands for single data rate for all phases.
 * DUAL stands for dual data rate for all phases.
 * S_S_D stands for single data rate for command and address phases but
 * dual data rate for data phase.
 * S_D_D stands for single data rate for command phase but dual data rate
 * for address and data phases.
 */
enum mspi_data_rate {
	MSPI_DATA_RATE_SINGLE       = 0,
	MSPI_DATA_RATE_S_S_D        = 1,
	MSPI_DATA_RATE_S_D_D        = 2,
	MSPI_DATA_RATE_DUAL         = 3,
	MSPI_DATA_RATE_MAX,
};

/**
 * @brief MSPI Polarity & Phase Modes
 */
enum mspi_cpp_mode {
	MSPI_CPP_MODE_0             = 0,
	MSPI_CPP_MODE_1             = 1,
	MSPI_CPP_MODE_2             = 2,
	MSPI_CPP_MODE_3             = 3,
};

/**
 * @brief MSPI Endian
 */
enum mspi_endian {
	MSPI_XFER_LITTLE_ENDIAN     = 0,
	MSPI_XFER_BIG_ENDIAN        = 1,
};

/**
 * @brief MSPI chip enable polarity
 */
enum mspi_ce_polarity {
	MSPI_CE_ACTIVE_LOW          = 0,
	MSPI_CE_ACTIVE_HIGH         = 1,
};

/**
 * @brief MSPI bus event.
 * This is a  preliminary list of events. I encourage the community
 * to fill it up.
 */
enum mspi_bus_event {
	MSPI_BUS_RESET              = 0,
	MSPI_BUS_ERROR              = 1,
	MSPI_BUS_XFER_COMPLETE      = 2,
	MSPI_BUS_EVENT_MAX,
};

/**
 * @brief MSPI bus event callback mask
 * This is a preliminary list same as mspi_bus_event. I encourage the
 * community to fill it up.
 */
enum mspi_bus_event_cb_mask {
	MSPI_BUS_NO_CB              = 0,
	MSPI_BUS_RESET_CB           = BIT(0),
	MSPI_BUS_ERROR_CB           = BIT(1),
	MSPI_BUS_XFER_COMPLETE_CB   = BIT(2),
};

/**
 * @brief MSPI transfer modes
 */
enum mspi_xfer_mode {
	MSPI_PIO,
	MSPI_DMA,
};

/**
 * @brief MSPI transfer priority
 * This is a preliminary list of priorities that are typically used with DMA
 */
enum mspi_xfer_priority {
	MSPI_XFER_PRIORITY_LOW,
	MSPI_XFER_PRIORITY_MEDIUM,
	MSPI_XFER_PRIORITY_HIGH,
};

/**
 * @brief MSPI transfer directions
 */
enum mspi_xfer_direction {
	MSPI_RX,
	MSPI_TX,
};

/**
 * @brief MSPI controller device specific configuration mask
 */
enum mspi_dev_cfg_mask {
	MSPI_DEVICE_CONFIG_NONE         = 0,
	MSPI_DEVICE_CONFIG_CE_NUM       = BIT(0),
	MSPI_DEVICE_CONFIG_FREQUENCY    = BIT(1),
	MSPI_DEVICE_CONFIG_IO_MODE      = BIT(2),
	MSPI_DEVICE_CONFIG_DATA_RATE    = BIT(3),
	MSPI_DEVICE_CONFIG_CPP          = BIT(4),
	MSPI_DEVICE_CONFIG_ENDIAN       = BIT(5),
	MSPI_DEVICE_CONFIG_CE_POL       = BIT(6),
	MSPI_DEVICE_CONFIG_DQS          = BIT(7),
	MSPI_DEVICE_CONFIG_RX_DUMMY     = BIT(8),
	MSPI_DEVICE_CONFIG_TX_DUMMY     = BIT(9),
	MSPI_DEVICE_CONFIG_READ_CMD     = BIT(10),
	MSPI_DEVICE_CONFIG_WRITE_CMD    = BIT(11),
	MSPI_DEVICE_CONFIG_CMD_LEN      = BIT(12),
	MSPI_DEVICE_CONFIG_ADDR_LEN     = BIT(13),
	MSPI_DEVICE_CONFIG_MEM_BOUND    = BIT(14),
	MSPI_DEVICE_CONFIG_BREAK_TIME   = BIT(15),
	MSPI_DEVICE_CONFIG_ALL          = BIT_MASK(16),
};

/**
 * @brief MSPI XIP access permissions
 */
enum mspi_xip_permit {
	MSPI_XIP_READ_WRITE     = 0,
	MSPI_XIP_READ_ONLY      = 1,
};

/**
 * @brief MSPI Configure API
 * @defgroup mspi_configure_api MSPI Configure API
 * @{
 */

/**
 * @brief Stub for timing parameter
 */
enum mspi_timing_param {
	MSPI_TIMING_PARAM_DUMMY
};

/**
 * @brief Stub for struct timing_cfg
 */
struct mspi_timing_cfg {
#ifdef __cplusplus
	/* For C++ compatibility. */
	uint8_t dummy;
#endif
};

/**
 * @brief MSPI device ID
 * The controller can identify its devices and determine whether the access is
 * allowed in a multiple device scheme.
 */
struct mspi_dev_id {
	/** @brief device gpio ce */
	struct gpio_dt_spec     ce;
	/** @brief device index on DT */
	uint16_t                dev_idx;
};

/**
 * @brief MSPI controller configuration
 */
struct mspi_cfg {
	/** @brief mspi channel number */
	uint8_t                 channel_num;
	/** @brief Configure operation mode */
	enum mspi_op_mode       op_mode;
	/** @brief Configure duplex mode */
	enum mspi_duplex        duplex;
	/** @brief DQS support flag */
	bool                    dqs_support;
	/** @brief Software managed multi peripheral enable */
	bool                    sw_multi_periph;
	/** @brief GPIO chip select lines (optional) */
	struct gpio_dt_spec     *ce_group;
	/** @brief GPIO chip-select line numbers (optional) */
	uint32_t                num_ce_gpios;
	/** @brief Peripheral number from 0 to host controller peripheral limit. */
	uint32_t                num_periph;
	/** @brief Maximum supported frequency in MHz */
	uint32_t                max_freq;
	/** @brief Whether to re-initialize controller */
	bool                    re_init;
};

/**
 * @brief MSPI DT information
 */
struct mspi_dt_spec {
	/** @brief MSPI bus */
	const struct device     *bus;
	/** @brief MSPI hardware specific configuration */
	struct mspi_cfg         config;
};

/**
 * @brief MSPI controller device specific configuration
 */
struct mspi_dev_cfg {
	/** @brief Configure CE0 or CE1 or more */
	uint8_t                 ce_num;
	/** @brief Configure frequency */
	uint32_t                freq;
	/** @brief Configure I/O mode */
	enum mspi_io_mode       io_mode;
	/** @brief Configure data rate */
	enum mspi_data_rate     data_rate;
	/** @brief Configure clock polarity and phase */
	enum mspi_cpp_mode      cpp;
	/** @brief Configure transfer endian */
	enum mspi_endian        endian;
	/** @brief Configure chip enable polarity */
	enum mspi_ce_polarity   ce_polarity;
	/** @brief Configure DQS mode */
	bool                    dqs_enable;
	/** @brief Configure number of clock cycles between
	 * addr and data in RX direction
	 */
	uint16_t                rx_dummy;
	/** @brief Configure number of clock cycles between
	 * addr and data in TX direction
	 */
	uint16_t                tx_dummy;
	/** @brief Configure read command       */
	uint32_t                read_cmd;
	/** @brief Configure write command      */
	uint32_t                write_cmd;
	/** @brief Configure command length     */
	uint8_t                 cmd_length;
	/** @brief Configure address length     */
	uint8_t                 addr_length;
	/** @brief Configure memory boundary    */
	uint32_t                mem_boundary;
	/** @brief Configure the time to break up a transfer into 2 */
	uint32_t                time_to_break;
};

/**
 * @brief MSPI controller XIP configuration
 */
struct mspi_xip_cfg {
	/** @brief XIP enable */
	bool                    enable;
	/** @brief XIP region start address =
	 * hardware default + address offset
	 */
	uint32_t                address_offset;
	/** @brief XIP region size */
	uint32_t                size;
	/** @brief XIP access permission */
	enum mspi_xip_permit    permission;
};

/**
 * @brief MSPI controller scramble configuration
 */
struct mspi_scramble_cfg {
	/** @brief scramble enable */
	bool                    enable;
	/** @brief scramble region start address =
	 * hardware default + address offset
	 */
	uint32_t                address_offset;
	/** @brief scramble region size */
	uint32_t                size;
};

/** @} */

/**
 * @brief MSPI Transfer API
 * @defgroup mspi_transfer_api MSPI Transfer API
 * @{
 */

/**
 * @brief MSPI Chip Select control structure
 *
 * This can be used to control a CE line via a GPIO line, instead of
 * using the controller inner CE logic.
 *
 */
struct mspi_ce_control {
	/**
	 * @brief GPIO devicetree specification of CE GPIO.
	 * The device pointer can be set to NULL to fully inhibit CE control if
	 * necessary. The GPIO flags GPIO_ACTIVE_LOW/GPIO_ACTIVE_HIGH should be
	 * the same as in MSPI configuration.
	 */
	struct gpio_dt_spec         gpio;
	/**
	 * @brief Delay to wait.
	 * In microseconds before starting the
	 * transmission and before releasing the CE line.
	 */
	uint32_t                    delay;
};

/**
 * @brief MSPI peripheral xfer packet format
 */
struct mspi_xfer_packet {
	/** @brief  Direction (Transmit/Receive) */
	enum mspi_xfer_direction    dir;
	/** @brief  Bus event callback masks     */
	enum mspi_bus_event_cb_mask cb_mask;
	/** @brief  Transfer command             */
	uint32_t                    cmd;
	/** @brief  Transfer Address             */
	uint32_t                    address;
	/** @brief  Number of bytes to transfer  */
	uint32_t                    num_bytes;
	/** @brief  Data Buffer                  */
	uint8_t                     *data_buf;
};

/**
 * @brief MSPI peripheral xfer format
 * This includes transfer related settings that may
 * require configuring the hardware.
 */
struct mspi_xfer {
	/** @brief  Async or sync transfer       */
	bool                        async;
	/** @brief  Transfer Mode                */
	enum mspi_xfer_mode         xfer_mode;
	/** @brief  Configure TX dummy cycles    */
	uint16_t                    tx_dummy;
	/** @brief  Configure RX dummy cycles    */
	uint16_t                    rx_dummy;
	/** @brief  Configure command length     */
	uint8_t                     cmd_length;
	/** @brief  Configure address length     */
	uint8_t                     addr_length;
	/** @brief  Hold CE active after xfer    */
	bool                        hold_ce;
	/** @brief  Software CE control          */
	struct mspi_ce_control      ce_sw_ctrl;
	/** @brief  MSPI transfer priority       */
	enum mspi_xfer_priority     priority;
	/** @brief  Transfer packets             */
	const struct mspi_xfer_packet *packets;
	/** @brief  Number of transfer packets   */
	uint32_t                    num_packet;
	/** @brief  Transfer timeout value(ms)   */
	uint32_t                    timeout;
};

/** @} */

/**
 * @brief MSPI callback API
 * @defgroup mspi_callback_api MSPI callback API
 * @{
 */

/**
 * @brief MSPI event data
 */
struct mspi_event_data {
	/** @brief Pointer to the bus controller */
	const struct device         *controller;
	/** @brief Pointer to the peripheral device ID */
	const struct mspi_dev_id    *dev_id;
	/** @brief Pointer to a transfer packet */
	const struct mspi_xfer_packet *packet;
	/** @brief MSPI event status */
	uint32_t                    status;
	/** @brief Packet index */
	uint32_t                    packet_idx;
};

/**
 * @brief MSPI event
 */
struct mspi_event {
	/** Event type */
	enum mspi_bus_event         evt_type;
	/** Data associated to the event */
	struct mspi_event_data      evt_data;
};

/**
 * @brief MSPI callback context
 */
struct mspi_callback_context {
	/** @brief MSPI event  */
	struct mspi_event           mspi_evt;
	/** @brief user defined context */
	void                        *ctx;
};

/**
 * @typedef mspi_callback_handler_t
 * @brief Define the application callback handler function signature.
 *
 * @param mspi_cb_ctx Pointer to the MSPI callback context
 *
 */
typedef void (*mspi_callback_handler_t)(struct mspi_callback_context *mspi_cb_ctx, ...);

/** @} */

/**
 * MSPI driver API definition and system call entry points
 */
typedef int (*mspi_api_config)(const struct mspi_dt_spec *spec);

typedef int (*mspi_api_dev_config)(const struct device *controller,
				   const struct mspi_dev_id *dev_id,
				   const enum mspi_dev_cfg_mask param_mask,
				   const struct mspi_dev_cfg *cfg);

typedef int (*mspi_api_get_channel_status)(const struct device *controller, uint8_t ch);

typedef int (*mspi_api_transceive)(const struct device *controller,
				   const struct mspi_dev_id *dev_id,
				   const struct mspi_xfer *req);

typedef int (*mspi_api_register_callback)(const struct device *controller,
					  const struct mspi_dev_id *dev_id,
					  const enum mspi_bus_event evt_type,
					  mspi_callback_handler_t cb,
					  struct mspi_callback_context *ctx);

typedef int (*mspi_api_xip_config)(const struct device *controller,
				   const struct mspi_dev_id *dev_id,
				   const struct mspi_xip_cfg *xip_cfg);

typedef int (*mspi_api_scramble_config)(const struct device *controller,
					const struct mspi_dev_id *dev_id,
					const struct mspi_scramble_cfg *scramble_cfg);

typedef int (*mspi_api_timing_config)(const struct device *controller,
				      const struct mspi_dev_id *dev_id, const uint32_t param_mask,
				      void *timing_cfg);

__subsystem struct mspi_driver_api {
	mspi_api_config                config;
	mspi_api_dev_config            dev_config;
	mspi_api_get_channel_status    get_channel_status;
	mspi_api_transceive            transceive;
	mspi_api_register_callback     register_callback;
	mspi_api_xip_config            xip_config;
	mspi_api_scramble_config       scramble_config;
	mspi_api_timing_config         timing_config;
};

/**
 * @addtogroup mspi_configure_api
 * @{
 */

/**
 * @brief Configure a MSPI controller.
 *
 * This routine provides a generic interface to override MSPI controller
 * capabilities.
 *
 * In the controller driver, one may implement this API to initialize or
 * re-initialize their controller hardware. Additional SoC platform specific
 * settings that are not in struct mspi_cfg may be added to one's own
 * binding(xxx,mspi-controller.yaml) so that one may derive the settings from
 * DTS and configure it in this API. In general, these settings should not
 * change during run-time. The bindings for @see mspi_cfg can be found in
 * mspi-controller.yaml.
 *
 * @param spec Pointer to MSPI DT information.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by MSPI peripheral.
 */
__syscall int mspi_config(const struct mspi_dt_spec *spec);

static inline int z_impl_mspi_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)spec->bus->api;

	return api->config(spec);
}

/**
 * @brief Configure a MSPI controller with device specific parameters.
 *
 * This routine provides a generic interface to override MSPI controller
 * device specific settings that should be derived from device datasheets.
 *
 * With @see mspi_dev_id defined as the device index and CE GPIO from device
 * tree, the API supports multiple devices on the same controller instance.
 * It is up to the controller driver implementation whether to support device
 * switching either by software or by hardware or not at all. If by software,
 * the switching should be done in this API's implementation.
 * The implementation may also support individual parameter configurations
 * specified by @see mspi_dev_cfg_mask.
 * The settings within @see mspi_dev_cfg don't typically change once the mode
 * of operation is determined after the device initialization.
 * The bindings for @see mspi_dev_cfg can be found in mspi-device.yaml.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param param_mask Macro definition of what to be configured in cfg.
 * @param cfg The device runtime configuration for the MSPI controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by MSPI peripheral.
 */
__syscall int mspi_dev_config(const struct device *controller,
			      const struct mspi_dev_id *dev_id,
			      const enum mspi_dev_cfg_mask param_mask,
			      const struct mspi_dev_cfg *cfg);

static inline int z_impl_mspi_dev_config(const struct device *controller,
					 const struct mspi_dev_id *dev_id,
					 const enum mspi_dev_cfg_mask param_mask,
					 const struct mspi_dev_cfg *cfg)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	return api->dev_config(controller, dev_id, param_mask, cfg);
}

/**
 * @brief Query to see if it a channel is ready.
 *
 * This routine allows to check if logical channel is ready before use.
 * Note that queries for channels not supported will always return false.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param ch the MSPI channel for which status is to be retrieved.
 *
 * @retval 0 If MSPI channel is ready.
 */
__syscall int mspi_get_channel_status(const struct device *controller, uint8_t ch);

static inline int z_impl_mspi_get_channel_status(const struct device *controller, uint8_t ch)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	return api->get_channel_status(controller, ch);
}

/** @} */

/**
 * @addtogroup mspi_transfer_api
 * @{
 */

/**
 * @brief Transfer request over MSPI.
 *
 * This routines provides a generic interface to transfer a request
 * synchronously/asynchronously.
 *
 * The @see mspi_xfer allows for dynamically changing the transfer related
 * settings once the mode of operation is determined and configured.
 * The API supports bulk transfers with different starting addresses and sizes
 * with @see mspi_xfer_packet. However, it is up to the controller
 * implementation whether to support scatter IO and callback management.
 * The controller can determine which user callback to trigger based on
 * @see mspi_bus_event_cb_mask upon completion of each async/sync transfer
 * if the callback had been registered. Or not to trigger any callback at all
 * with MSPI_BUS_NO_CB even if the callbacks are already registered.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param req Content of the request and request specific settings.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP
 * @retval -EIO General input / output error, failed to send over the bus.
 */
__syscall int mspi_transceive(const struct device *controller,
			      const struct mspi_dev_id *dev_id,
			      const struct mspi_xfer *req);

static inline int z_impl_mspi_transceive(const struct device *controller,
					 const struct mspi_dev_id *dev_id,
					 const struct mspi_xfer *req)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->transceive) {
		return -ENOTSUP;
	}

	return api->transceive(controller, dev_id, req);
}

/** @} */

/**
 * @addtogroup mspi_configure_api
 * @{
 */

/**
 * @brief Configure a MSPI XIP settings.
 *
 * This routine provides a generic interface to configure the XIP feature.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param cfg The controller XIP configuration for MSPI.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by MSPI peripheral.
 */
__syscall int mspi_xip_config(const struct device *controller,
			      const struct mspi_dev_id *dev_id,
			      const struct mspi_xip_cfg *cfg);

static inline int z_impl_mspi_xip_config(const struct device *controller,
					 const struct mspi_dev_id *dev_id,
					 const struct mspi_xip_cfg *cfg)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->xip_config) {
		return -ENOTSUP;
	}

	return api->xip_config(controller, dev_id, cfg);
}

/**
 * @brief Configure a MSPI scrambling settings.
 *
 * This routine provides a generic interface to configure the scrambling
 * feature.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param cfg The controller scramble configuration for MSPI.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by MSPI peripheral.
 */
__syscall int mspi_scramble_config(const struct device *controller,
				   const struct mspi_dev_id *dev_id,
				   const struct mspi_scramble_cfg *cfg);

static inline int z_impl_mspi_scramble_config(const struct device *controller,
					      const struct mspi_dev_id *dev_id,
					      const struct mspi_scramble_cfg *cfg)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->scramble_config) {
		return -ENOTSUP;
	}

	return api->scramble_config(controller, dev_id, cfg);
}

/**
 * @brief Configure a MSPI timing settings.
 *
 * This routine provides a generic interface to configure MSPI controller
 * timing if necessary.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param param_mask The macro definition of what should be configured in cfg.
 * @param cfg The controller timing configuration for MSPI.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by MSPI peripheral.
 */
__syscall int mspi_timing_config(const struct device *controller,
				 const struct mspi_dev_id *dev_id,
				 const uint32_t param_mask, void *cfg);

static inline int z_impl_mspi_timing_config(const struct device *controller,
					    const struct mspi_dev_id *dev_id,
					    const uint32_t param_mask, void *cfg)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->timing_config) {
		return -ENOTSUP;
	}

	return api->timing_config(controller, dev_id, param_mask, cfg);
}

/** @} */

/**
 * @addtogroup mspi_callback_api
 * @{
 */

/**
 * @brief Register the mspi callback functions.
 *
 * This routines provides a generic interface to register mspi callback functions.
 * In generally it should be called before mspi_transceive.
 *
 * @param controller Pointer to the device structure for the driver instance.
 * @param dev_id Pointer to the device ID structure from a device.
 * @param evt_type The event type associated the callback.
 * @param cb Pointer to the user implemented callback function.
 * @param ctx Pointer to the callback context.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP
 */
static inline int mspi_register_callback(const struct device *controller,
					 const struct mspi_dev_id *dev_id,
					 const enum mspi_bus_event evt_type,
					 mspi_callback_handler_t cb,
					 struct mspi_callback_context *ctx)
{
	const struct mspi_driver_api *api = (const struct mspi_driver_api *)controller->api;

	if (!api->register_callback) {
		return -ENOTSUP;
	}

	return api->register_callback(controller, dev_id, evt_type, cb, ctx);
}

/** @} */

#ifdef __cplusplus
}
#endif

#include <zephyr/drivers/mspi/devicetree.h>

/**
 * @addtogroup mspi_util
 * @{
 */
#include <zephyr/sys/util_macro.h>

/**
 * @brief Declare the optional XIP config in peripheral driver.
 */
#define MSPI_XIP_CFG_STRUCT_DECLARE(_name)                                                        \
	IF_ENABLED(CONFIG_MSPI_XIP, (struct mspi_xip_cfg _name;))

/**
 * @brief Declare the optional XIP base address in peripheral driver.
 */
#define MSPI_XIP_BASE_ADDR_DECLARE(_name)                                                         \
	IF_ENABLED(CONFIG_MSPI_XIP, (uint32_t _name;))

/**
 * @brief Declare the optional scramble config in peripheral driver.
 */
#define MSPI_SCRAMBLE_CFG_STRUCT_DECLARE(_name)                                                   \
	IF_ENABLED(CONFIG_MSPI_SCRAMBLE, (struct mspi_scramble_cfg _name;))

/**
 * @brief Declare the optional timing config in peripheral driver.
 */
#define MSPI_TIMING_CFG_STRUCT_DECLARE(_name)                                                     \
	IF_ENABLED(CONFIG_MSPI_TIMING, (mspi_timing_cfg _name;))

/**
 * @brief Declare the optional timing parameter in peripheral driver.
 */
#define MSPI_TIMING_PARAM_DECLARE(_name)                                                          \
	IF_ENABLED(CONFIG_MSPI_TIMING, (mspi_timing_param _name;))

/**
 * @brief Initialize the optional config structure in peripheral driver.
 */
#define MSPI_OPTIONAL_CFG_STRUCT_INIT(code, _name, _object)                                       \
	IF_ENABLED(code, (._name = _object,))

/**
 * @brief Initialize the optional XIP base address in peripheral driver.
 */
#define MSPI_XIP_BASE_ADDR_INIT(_name, _bus)                                                      \
	IF_ENABLED(CONFIG_MSPI_XIP, (._name = DT_REG_ADDR_BY_IDX(_bus, 1),))

/** @} */

/**
 * @}
 */
#include <zephyr/syscalls/mspi.h>
#endif /* ZEPHYR_INCLUDE_MSPI_H_ */
