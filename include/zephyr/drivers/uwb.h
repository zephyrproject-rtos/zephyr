/** @file
 *  @brief Ultrawide Band driver API.
 *
 *  Copyright 2026 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup uwb_uci_api
 * @brief Main header file for UWB driver API.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UWB_H_
#define ZEPHYR_INCLUDE_DRIVERS_UWB_H_

/**
 * @brief Interfaces for Ultra-Wide Band Command Interface (UCI).
 * @defgroup uwb_uci_api Ultra-Wide Band UCI
 *
 * @ingroup uwb
 * @{
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Get UWB device name from device node */
#define UWB_DT_UCI_NAME_GET(node_id)   DT_PROP_OR(node_id, uwb_uci_name, "UCI")
/** Get UWB device node from instance */
#define UWB_DT_UCI_NAME_INST_GET(inst) UWB_DT_UCI_NAME_GET(DT_DRV_INST(inst))
/** Fallback default when there's no property, same as "virtual" */
#define UWB_PRIV_UCI_BUS_DEFAULT    (0)
/** Get UWB bus number from device node */
#define UWB_DT_UCI_BUS_GET(node_id) DT_ENUM_IDX_OR(node_id, uwb_uci_bus, UWB_PRIV_UCI_BUS_DEFAULT)
/** Get UWB bus number from instance */
#define UWB_DT_UCI_BUS_INST_GET(inst) UWB_DT_UCI_BUS_GET(DT_DRV_INST(inst))

/**
 * @def_driverbackendgroup{UltraWideBand UCI,uwb_uci_api}
 * @{
 */

/**
 * @brief Callback API to open the UCI transport.
 * See uwb_uci_open() for argument description
 */
typedef int (*uwb_uci_api_open_t)(const struct device *dev);

/**
 * @brief Callback API to close the UCI transport.
 * See uwb_uci_close() for argument description
 */
typedef int (*uwb_uci_api_close_t)(const struct device *dev);

/**
 * @brief Callback API to send an UCI buffer to the controller.
 * See uwb_uci_send() for argument description
 */
typedef int (*uwb_uci_api_send_t)(const struct device *dev, uint8_t *buffer,
				  uint16_t bytes_to_write);

/**
 * @brief Callback API for UCI vendor-specific setup.
 * See uwb_uci_recv() for argument description
 */
typedef int (*uwb_uci_api_recv_t)(const struct device *dev, uint8_t *buffer, int bytes_to_read);

/**
 * @driver_ops{UltraWideBand UCI}
 */
__subsystem struct uwb_uci_driver_api {
	/**
	 * @driver_ops_mandatory @copybrief uwb_uci_open
	 */
	uwb_uci_api_open_t open;
	/**
	 * @driver_ops_mandatory @copybrief uwb_uci_close
	 */
	uwb_uci_api_close_t close;
	/**
	 * @driver_ops_mandatory @copybrief uwb_uci_send
	 */
	uwb_uci_api_send_t send;
	/**
	 * @driver_ops_mandatory @copybrief uwb_uci_recv
	 */
	uwb_uci_api_recv_t recv;
};
/**
 * @}
 */

/**
 * @brief Open UCI Transport layer
 *
 * Opens the UCI transport for operation. This API will initialize
 * lower layer UWB driver to enable communication with the UWB device
 * Driver specific open API is called
 *
 * @param dev  UCI device
 *
 * @retval 0, if successful
 * @retval non-zero, otherwise
 */
static inline int uwb_uci_open(const struct device *dev)
{
	return DEVICE_API_GET(uwb_uci, dev)->open(dev);
}

/**
 * @brief Close UCI Transport layer
 *
 * Closes the UCI transport for operation. This API will de-initialize
 * lower layer UWB driver to disable communication with the UWB device
 * Driver specific close API is called
 *
 * @param dev  UCI device
 *
 * @retval 0, if successful
 * @retval non-zero, otherwise
 */
static inline int uwb_uci_close(const struct device *dev)
{
	return DEVICE_API_GET(uwb_uci, dev)->close(dev);
}

/**
 * @brief Write data to UWB device
 *
 * Writes UCI encoded data to UWB device.
 * Driver specific send API is called.
 *
 * @param dev  UCI device
 * @param[out] buffer Input buffer to be written to UWB device
 * @param[in] bytes_to_write Number of bytes to write to UWB device
 *
 * @retval 0, if successful
 * @retval non-zero, otherwise
 */
static inline int uwb_uci_send(const struct device *dev, uint8_t *buffer, uint16_t bytes_to_write)
{
	return DEVICE_API_GET(uwb_uci, dev)->send(dev, buffer, bytes_to_write);
}

/**
 * @brief Read data from UWB device
 *
 * This API returns stream of data from UWB device. Return value
 * is the number of bytes read. If there is no data available, -EAGAIN is returned
 * Driver specific read API is called
 *
 * @param dev  UCI device
 * @param[out] buffer Output buffer to be populated with read data
 * @param[in] bytes_to_read Maximum bytes to read from UWB device
 *
 * @retval 0, if successful
 * @retval -EAGAIN, no data available
 * @retval non-zero, otherwise
 */
static inline int uwb_uci_recv(const struct device *dev, uint8_t *buffer, int bytes_to_read)
{
	return DEVICE_API_GET(uwb_uci, dev)->recv(dev, buffer, bytes_to_read);
}

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_UWB_H_ */
