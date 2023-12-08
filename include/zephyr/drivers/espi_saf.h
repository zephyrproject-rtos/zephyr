/*
 * Copyright (c) 2019 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for eSPI driver
 */

#ifndef ZEPHYR_INCLUDE_ESPI_SAF_H_
#define ZEPHYR_INCLUDE_ESPI_SAF_H_

#include <zephyr/sys/__assert.h>
#include <zephyr/types.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief eSPI SAF Driver APIs
 * @defgroup espi_interface ESPI Driver APIs
 * @ingroup io_interfaces
 * @{
 */


/**
 * @code
 *+----------------------------------------------------------------------+
 *|                                                                      |
 *|  eSPI host                           +-------------+                 |
 *|                      +-----------+   |    Power    |   +----------+  |
 *|                      |Out of band|   |  management |   |   GPIO   |  |
 *|   ------------       |processor  |   |  controller |   |  sources |  |
 *|                      +-----------+   +-------------+   +----------+  |
 *|                            |                |               |        |
 *|   ------------             |                |               |        |
 *|                            +--------+       +---------------+        |
 *|                                     |               |                |
 *|            -----+   +--------+   +----------+  +----v-----+          |
 *|                 |   |  LPC   |   | Tunneled |  | Tunneled |          |
 *|                 |   | bridge |   |  SMBus   |  |   GPIO   |          |
 *|                 |   +--------+   +----------+  +----------+          |
 *|                 |        |             |             |               |
 *|                 |        ------+       |             |               |
 *|                 |              |       |             |               |
 *|          +------v-----+    +---v-------v-------------v----+          |
 *|          | eSPI Flash |    |    eSPI protocol block       |          |
 *|          |   access   +--->+                              |          |
 *|          +------------+    +------------------------------+          |
 *|                                    |                                 |
 *|       -----------                  |                                 |
 *|                                    v                                 |
 *|               XXXXXXXXXXXXXXXXXXXXXXX                                |
 *|                XXXXXXXXXXXXXXXXXXXXX                                 |
 *|                 XXXXXXXXXXXXXXXXXXX                                  |
 *+----------------------------------------------------------------------+
 *                          |
 *                 +-----------------+
 *  ---------      |  |   |   |   |  |
 *                 |  |   |   |   |  |
 *  ---------      |  +   +   +   +  |    eSPI bus
 *                 | CH0 CH1 CH2 CH3 |    (logical channels)
 *                 |  +   +   +   +  |
 *                 |  |   |   |   |  |
 *                 +-----------------+
 *                          |
 *+-----------------------------------------------------------------------+
 *|  eSPI slave                                                           |
 *|                                                                       |
 *|       CH0         |     CH1      |      CH2      |    CH3             |
 *|   eSPI endpoint   |    VWIRE     |      OOB      |   Flash            |
 *+-----------------------------------------------------------------------+
 *   |                                 |
 *   v                                 |
 * +---------+                         |
 * |  Flash  |  Slave Attached Flash   |
 * +---------+                         |
 *                                     |
 * @endcode
 */


/**
 * @cond INTERNAL_HIDDEN
 *
 */


/** @endcond */

struct espi_saf_hw_cfg;
struct espi_saf_flash_cfg;
struct espi_saf_pr;

/**
 * @brief eSPI SAF configuration parameters
 */
struct espi_saf_cfg {
	uint8_t nflash_devices;
	struct espi_saf_hw_cfg hwcfg;
	struct espi_saf_flash_cfg *flash_cfgs;
};

/**
 * @brief eSPI SAF transaction packet format
 */
struct espi_saf_packet {
	uint32_t flash_addr;
	uint8_t *buf;
	uint32_t len;
};

/*
 *defined in espi.h
 * struct espi_callback
 * typedef void (*espi_callback_handler_t)()
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * eSPI driver API definition and system call entry points
 *
 * (Internal use only.)
 */
typedef int (*espi_saf_api_config)(const struct device *dev,
				   const struct espi_saf_cfg *cfg);

typedef int (*espi_saf_api_set_protection_regions)(
				const struct device *dev,
				const struct espi_saf_protection *pr);

typedef int (*espi_saf_api_activate)(const struct device *dev);

typedef bool (*espi_saf_api_get_channel_status)(const struct device *dev);

typedef int (*espi_saf_api_flash_read)(const struct device *dev,
				       struct espi_saf_packet *pckt);
typedef int (*espi_saf_api_flash_write)(const struct device *dev,
					struct espi_saf_packet *pckt);
typedef int (*espi_saf_api_flash_erase)(const struct device *dev,
					struct espi_saf_packet *pckt);
typedef int (*espi_saf_api_flash_unsuccess)(const struct device *dev,
					struct espi_saf_packet *pckt);
/* Callbacks and traffic intercept */
typedef int (*espi_saf_api_manage_callback)(const struct device *dev,
					    struct espi_callback *callback,
					    bool set);

__subsystem struct espi_saf_driver_api {
	espi_saf_api_config config;
	espi_saf_api_set_protection_regions set_protection_regions;
	espi_saf_api_activate activate;
	espi_saf_api_get_channel_status get_channel_status;
	espi_saf_api_flash_read flash_read;
	espi_saf_api_flash_write flash_write;
	espi_saf_api_flash_erase flash_erase;
	espi_saf_api_flash_unsuccess flash_unsuccess;
	espi_saf_api_manage_callback manage_callback;
};

/**
 * @endcond
 */

/**
 * @brief Configure operation of a eSPI controller.
 *
 * This routine provides a generic interface to override eSPI controller
 * capabilities.
 *
 * If this eSPI controller is acting as slave, the values set here
 * will be discovered as part through the GET_CONFIGURATION command
 * issued by the eSPI master during initialization.
 *
 * If this eSPI controller is acting as master, the values set here
 * will be used by eSPI master to determine minimum common capabilities with
 * eSPI slave then send via SET_CONFIGURATION command.
 *
 * @code
 * +--------+   +---------+     +------+          +---------+   +---------+
 * |  eSPI  |   |  eSPI   |     | eSPI |          |  eSPI   |   |  eSPI   |
 * |  slave |   | driver  |     |  bus |          |  driver |   |  host   |
 * +--------+   +---------+     +------+          +---------+   +---------+
 *     |              |            |                   |             |
 *     | espi_config  | Set eSPI   |       Set eSPI    | espi_config |
 *     +--------------+ ctrl regs  |       cap ctrl reg| +-----------+
 *     |              +-------+    |          +--------+             |
 *     |              |<------+    |          +------->|             |
 *     |              |            |                   |             |
 *     |              |            |                   |             |
 *     |              |            | GET_CONFIGURATION |             |
 *     |              |            +<------------------+             |
 *     |              |<-----------|                   |             |
 *     |              | eSPI caps  |                   |             |
 *     |              |----------->+    response       |             |
 *     |              |            |------------------>+             |
 *     |              |            |                   |             |
 *     |              |            | SET_CONFIGURATION |             |
 *     |              |            +<------------------+             |
 *     |              |            |  accept           |             |
 *     |              |            +------------------>+             |
 *     +              +            +                   +             +
 * @endcode
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param cfg the device runtime configuration for the eSPI controller.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by eSPI slave.
 */
__syscall int espi_saf_config(const struct device *dev,
			      const struct espi_saf_cfg *cfg);

static inline int z_impl_espi_saf_config(const struct device *dev,
					 const struct espi_saf_cfg *cfg)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	return api->config(dev, cfg);
}

/**
 * @brief Set one or more SAF protection regions
 *
 * This routine provides an interface to override the default flash
 * protection regions of the SAF controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pr Pointer to the SAF protection region structure.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error, failed to configure device.
 * @retval -EINVAL invalid capabilities, failed to configure device.
 * @retval -ENOTSUP capability not supported by eSPI slave.
 */
__syscall int espi_saf_set_protection_regions(
				const struct device *dev,
				const struct espi_saf_protection *pr);

static inline int z_impl_espi_saf_set_protection_regions(
					const struct device *dev,
					const struct espi_saf_protection *pr)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	return api->set_protection_regions(dev, pr);
}

/**
 * @brief Activate SAF block
 *
 * This routine activates the SAF block and should only be
 * called after SAF has been configured and the eSPI Master
 * has enabled the Flash Channel.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful
 * @retval -EINVAL if failed to activate SAF.
 */
__syscall int espi_saf_activate(const struct device *dev);

static inline int z_impl_espi_saf_activate(const struct device *dev)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	return api->activate(dev);
}

/**
 * @brief Query to see if SAF is ready
 *
 * This routine allows to check if SAF is ready before use.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval true If eSPI SAF is ready.
 * @retval false otherwise.
 */
__syscall bool espi_saf_get_channel_status(const struct device *dev);

static inline bool z_impl_espi_saf_get_channel_status(
					const struct device *dev)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	return api->get_channel_status(dev);
}

/**
 * @brief Sends a read request packet for slave attached flash.
 *
 * This routines provides an interface to send a request to read the flash
 * component shared between the eSPI master and eSPI slaves.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of read flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by master.
 * @retval -EIO General input / output error, failed request to master.
 */
__syscall int espi_saf_flash_read(const struct device *dev,
				  struct espi_saf_packet *pckt);

static inline int z_impl_espi_saf_flash_read(const struct device *dev,
					     struct espi_saf_packet *pckt)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->flash_read) {
		return -ENOTSUP;
	}

	return api->flash_read(dev, pckt);
}

/**
 * @brief Sends a write request packet for slave attached flash.
 *
 * This routines provides an interface to send a request to write to the flash
 * components shared between the eSPI master and eSPI slaves.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of write flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by master.
 * @retval -EIO General input / output error, failed request to master.
 */
__syscall int espi_saf_flash_write(const struct device *dev,
				   struct espi_saf_packet *pckt);

static inline int z_impl_espi_saf_flash_write(const struct device *dev,
					      struct espi_saf_packet *pckt)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->flash_write) {
		return -ENOTSUP;
	}

	return api->flash_write(dev, pckt);
}

/**
 * @brief Sends a write request packet for slave attached flash.
 *
 * This routines provides an interface to send a request to write to the flash
 * components shared between the eSPI master and eSPI slaves.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of erase flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by master.
 * @retval -EIO General input / output error, failed request to master.
 */
__syscall int espi_saf_flash_erase(const struct device *dev,
				   struct espi_saf_packet *pckt);

static inline int z_impl_espi_saf_flash_erase(const struct device *dev,
					      struct espi_saf_packet *pckt)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->flash_erase) {
		return -ENOTSUP;
	}

	return api->flash_erase(dev, pckt);
}

/**
 * @brief Response unsuccessful completion for slave attached flash.
 *
 * This routines provides an interface to response that transaction is
 * invalid and return unsuccessful completion from target to controller.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param pckt Address of the representation of flash transaction.
 *
 * @retval -ENOTSUP eSPI flash logical channel transactions not supported.
 * @retval -EBUSY eSPI flash channel is not ready or disabled by master.
 * @retval -EIO General input / output error, failed request to master.
 */
__syscall int espi_saf_flash_unsuccess(const struct device *dev,
				       struct espi_saf_packet *pckt);

static inline int z_impl_espi_saf_flash_unsuccess(const struct device *dev,
						  struct espi_saf_packet *pckt)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->flash_unsuccess) {
		return -ENOTSUP;
	}

	return api->flash_unsuccess(dev, pckt);
}

/**
 * Callback model
 *
 * @code
 *+-------+                  +-------------+   +------+     +---------+
 *|  App  |                  | eSPI driver |   |  HW  |     |eSPI Host|
 *+---+---+                  +-------+-----+   +---+--+     +----+----+
 *    |                              |             |             |
 *    |   espi_init_callback         |             |             |
 *    +----------------------------> |             |             |
 *    |   espi_add_callback          |             |
 *    +----------------------------->+             |
 *    |                              |             |  eSPI reset |  eSPI host
 *    |                              |    IRQ      +<------------+  resets the
 *    |                              | <-----------+             |  bus
 *    |                              |             |             |
 *    |                              | Processed   |             |
 *    |                              | within the  |             |
 *    |                              | driver      |             |
 *    |                              |             |             |

 *    |                              |             |  VW CH ready|  eSPI host
 *    |                              |    IRQ      +<------------+  enables VW
 *    |                              | <-----------+             |  channel
 *    |                              |             |             |
 *    |                              | Processed   |             |
 *    |                              | within the  |             |
 *    |                              | driver      |             |
 *    |                              |             |             |
 *    |                              |             | Memory I/O  |  Peripheral
 *    |                              |             <-------------+  event
 *    |                              +<------------+             |
 *    +<-----------------------------+ callback    |             |
 *    | Report peripheral event      |             |             |
 *    | and data for the event       |             |             |
 *    |                              |             |             |
 *    |                              |             | SLP_S5      |  eSPI host
 *    |                              |             <-------------+  send VWire
 *    |                              +<------------+             |
 *    +<-----------------------------+ callback    |             |
 *    | App enables/configures       |             |             |
 *    | discrete regulator           |             |             |
 *    |                              |             |             |
 *    |   espi_send_vwire_signal     |             |             |
 *    +------------------------------>------------>|------------>|
 *    |                              |             |             |
 *    |                              |             | HOST_RST    |  eSPI host
 *    |                              |             <-------------+  send VWire
 *    |                              +<------------+             |
 *    +<-----------------------------+ callback    |             |
 *    | App reset host-related       |             |             |
 *    | data structures              |             |             |
 *    |                              |             |             |
 *    |                              |             |   C10       |  eSPI host
 *    |                              |             +<------------+  send VWire
 *    |                              <-------------+             |
 *    <------------------------------+             |             |
 *    | App executes                 |             |             |
 *    + power mgmt policy            |             |             |
 * @endcode
 */

/**
 * @brief Helper to initialize a struct espi_callback properly.
 *
 * @param callback A valid Application's callback structure pointer.
 * @param handler A valid handler function pointer.
 * @param evt_type indicates the eSPI event relevant for the handler.
 * for VWIRE_RECEIVED event the data will indicate the new level asserted
 */
static inline void espi_saf_init_callback(struct espi_callback *callback,
					  espi_callback_handler_t handler,
					  enum espi_bus_event evt_type)
{
	__ASSERT(callback, "Callback pointer should not be NULL");
	__ASSERT(handler, "Callback handler pointer should not be NULL");

	callback->handler = handler;
	callback->evt_type = evt_type;
}

/**
 * @brief Add an application callback.
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback A valid Application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 *
 * @note Callbacks may be added to the device from within a callback
 * handler invocation, but whether they are invoked for the current
 * eSPI event is not specified.
 *
 * Note: enables to add as many callback as needed on the same device.
 */
static inline int espi_saf_add_callback(const struct device *dev,
					struct espi_callback *callback)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->manage_callback) {
		return -ENOTSUP;
	}

	return api->manage_callback(dev, callback, true);
}

/**
 * @brief Remove an application callback.
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback A valid application's callback structure pointer.
 * @return 0 if successful, negative errno code on failure.
 *
 * @warning It is explicitly permitted, within a callback handler, to
 * remove the registration for the callback that is running, i.e. @p
 * callback.  Attempts to remove other registrations on the same
 * device may result in undefined behavior, including failure to
 * invoke callbacks that remain registered and unintended invocation
 * of removed callbacks.
 *
 * Note: enables to remove as many callbacks as added through
 *       espi_add_callback().
 */
static inline int espi_saf_remove_callback(const struct device *dev,
					   struct espi_callback *callback)
{
	const struct espi_saf_driver_api *api =
		(const struct espi_saf_driver_api *)dev->api;

	if (!api->manage_callback) {
		return -ENOTSUP;
	}

	return api->manage_callback(dev, callback, false);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */
#include <syscalls/espi_saf.h>
#endif /* ZEPHYR_INCLUDE_ESPI_SAF_H_ */
