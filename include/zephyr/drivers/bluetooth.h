/** @file
 *  @brief Bluetooth HCI driver API.
 *
 *  Copyright (c) 2024 Johan Hedberg
 *
 *  SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H_
#define ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H_

/**
 * @brief Bluetooth HCI APIs
 * @defgroup bt_hci_api Bluetooth HCI APIs
 *
 * @since 3.7
 * @version 0.2.0
 *
 * @ingroup bluetooth
 * @{
 */

#include <stdbool.h>
#include <stdint.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

struct bt_hci_setup_params {
	/** The public identity address to give to the controller. This field is used when the
	 *  driver selects @kconfig{CONFIG_BT_HCI_SET_PUBLIC_ADDR} to indicate that it supports
	 *  setting the controller's public address.
	 */
	bt_addr_t public_addr;
};

enum {
	/* The host should never send HCI_Reset */
	BT_HCI_QUIRK_NO_RESET = BIT(0),
	/* The controller does not auto-initiate a DLE procedure when the
	 * initial connection data length parameters are not equal to the
	 * default data length parameters. Therefore the host should initiate
	 * the DLE procedure after connection establishment.
	 */
	BT_HCI_QUIRK_NO_AUTO_DLE = BIT(1),
};

/** Possible values for the 'bus' member of the bt_hci_driver struct */
enum bt_hci_bus {
	BT_HCI_BUS_VIRTUAL       = 0,
	BT_HCI_BUS_USB           = 1,
	BT_HCI_BUS_PCCARD        = 2,
	BT_HCI_BUS_UART          = 3,
	BT_HCI_BUS_RS232         = 4,
	BT_HCI_BUS_PCI           = 5,
	BT_HCI_BUS_SDIO          = 6,
	BT_HCI_BUS_SPI           = 7,
	BT_HCI_BUS_I2C           = 8,
	BT_HCI_BUS_IPM           = 9,
};

#define BT_DT_HCI_QUIRK_OR(node_id, prop, idx) DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)
#define BT_DT_HCI_QUIRKS_GET(node_id) COND_CODE_1(DT_NODE_HAS_PROP(node_id, bt_hci_quirks), \
						  (DT_FOREACH_PROP_ELEM_SEP(node_id, \
									    bt_hci_quirks, \
									    BT_DT_HCI_QUIRK_OR, \
									    (|))), \
						  (0))
#define BT_DT_HCI_QUIRKS_INST_GET(inst) BT_DT_HCI_QUIRKS_GET(DT_DRV_INST(inst))

#define BT_DT_HCI_NAME_GET(node_id) DT_PROP_OR(node_id, bt_hci_name, "HCI")
#define BT_DT_HCI_NAME_INST_GET(inst) BT_DT_HCI_NAME_GET(DT_DRV_INST(inst))

#define BT_DT_HCI_BUS_GET(node_id) DT_STRING_TOKEN_OR(node_id, bt_hci_bus, BT_HCI_BUS_VIRTUAL)
#define BT_DT_HCI_BUS_INST_GET(inst) BT_DT_HCI_BUS_GET(DT_DRV_INST(inst))

typedef int (*bt_hci_recv_t)(const struct device *dev, struct net_buf *buf);

__subsystem struct bt_hci_driver_api {
	int (*open)(const struct device *dev, bt_hci_recv_t recv);
	int (*close)(const struct device *dev);
	int (*send)(const struct device *dev, struct net_buf *buf);
#if defined(CONFIG_BT_HCI_SETUP)
	int (*setup)(const struct device *dev,
		     const struct bt_hci_setup_params *param);
#endif /* defined(CONFIG_BT_HCI_SETUP) */
};

/**
 * @brief Open the HCI transport.
 *
 * Opens the HCI transport for operation. This function must not
 * return until the transport is ready for operation, meaning it
 * is safe to start calling the send() handler.
 *
 * @param dev  HCI device
 * @param recv This is callback through which the HCI driver provides the
 *             host with data from the controller. The buffer passed to
 *             the callback will have its type set with bt_buf_set_type().
 *             The callback is expected to be called from thread context.
 *
 * @return 0 on success or negative POSIX error number on failure.
 */
static inline int bt_hci_open(const struct device *dev, bt_hci_recv_t recv)
{
	const struct bt_hci_driver_api *api = (const struct bt_hci_driver_api *)dev->api;

	return api->open(dev, recv);
}

/**
 * @brief Close the HCI transport.
 *
 * Closes the HCI transport. This function must not return until the
 * transport is closed.
 *
 * @param dev HCI device
 *
 * @return 0 on success or negative POSIX error number on failure.
 */
static inline int bt_hci_close(const struct device *dev)
{
	const struct bt_hci_driver_api *api = (const struct bt_hci_driver_api *)dev->api;

	if (api->close == NULL) {
		return -ENOSYS;
	}

	return api->close(dev);
}

/**
 * @brief Send HCI buffer to controller.
 *
 * Send an HCI packet to the controller. The packet type of the buffer
 * must be set using bt_buf_set_type().
 *
 * @note This function must only be called from a cooperative thread.
 *
 * @param dev HCI device
 * @param buf Buffer containing data to be sent to the controller.
 *
 * @return 0 on success or negative POSIX error number on failure.
 */
static inline int bt_hci_send(const struct device *dev, struct net_buf *buf)
{
	const struct bt_hci_driver_api *api = (const struct bt_hci_driver_api *)dev->api;

	return api->send(dev, buf);
}

#if defined(CONFIG_BT_HCI_SETUP) || defined(__DOXYGEN__)
/**
 * @brief HCI vendor-specific setup
 *
 * Executes vendor-specific commands sequence to initialize
 * BT Controller before BT Host executes Reset sequence. This is normally
 * called directly after bt_hci_open().
 *
 * @note @kconfig{CONFIG_BT_HCI_SETUP} must be selected for this
 * field to be available.
 *
 * @return 0 on success or negative POSIX error number on failure.
 */
static inline int bt_hci_setup(const struct device *dev, struct bt_hci_setup_params *params)
{
	const struct bt_hci_driver_api *api = (const struct bt_hci_driver_api *)dev->api;

	if (api->setup == NULL) {
		return -ENOSYS;
	}

	return api->setup(dev, params);
}
#endif

/**
 * @}
 */

/* The following functions are not strictly part of the HCI driver API, in that
 * they do not take as input a struct device which implements the HCI driver API.
 */

/**
 * @brief Setup the HCI transport, which usually means to reset the
 * Bluetooth IC.
 *
 * @note A weak version of this function is included in the H4 driver, so
 *       defining it is optional per board.
 *
 * @param dev The device structure for the bus connecting to the IC
 *
 * @return 0 on success, negative error value on failure
 */
int bt_hci_transport_setup(const struct device *dev);

/**
 * @brief Teardown the HCI transport.
 *
 * @note A weak version of this function is included in the IPC driver, so
 *		defining it is optional. NRF5340 includes support to put network core
 *		in reset state.
 *
 * @param dev The device structure for the bus connecting to the IC
 *
 * @return 0 on success, negative error value on failure
 */
int bt_hci_transport_teardown(const struct device *dev);

/** Allocate an HCI event buffer.
 *
 * This function allocates a new buffer for an HCI event. It is given the
 * event code and the total length of the parameters. Upon successful return
 * the buffer is ready to have the parameters encoded into it.
 *
 * @param evt        HCI event OpCode.
 * @param len        Length of event parameters.
 *
 * @return Newly allocated buffer.
 */
struct net_buf *bt_hci_evt_create(uint8_t evt, uint8_t len);

/** Allocate an HCI Command Complete event buffer.
 *
 * This function allocates a new buffer for HCI Command Complete event.
 * It is given the OpCode (encoded e.g. using the BT_OP macro) and the total
 * length of the parameters. Upon successful return the buffer is ready to have
 * the parameters encoded into it.
 *
 * @param op         HCI command OpCode.
 * @param plen       Length of command parameters.
 *
 * @return Newly allocated buffer.
 */
struct net_buf *bt_hci_cmd_complete_create(uint16_t op, uint8_t plen);

/** Allocate an HCI Command Status event buffer.
 *
 * This function allocates a new buffer for HCI Command Status event.
 * It is given the OpCode (encoded e.g. using the BT_OP macro) and the status
 * code. Upon successful return the buffer is ready to have the parameters
 * encoded into it.
 *
 * @param op         HCI command OpCode.
 * @param status     Status code.
 *
 * @return Newly allocated buffer.
 */
struct net_buf *bt_hci_cmd_status_create(uint16_t op, uint8_t status);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_BLUETOOTH_H_ */
