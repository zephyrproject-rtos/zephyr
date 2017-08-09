/** @file
 *  @brief Bluetooth HCI driver API.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_HCI_DRIVER_H
#define __BT_HCI_DRIVER_H

/**
 * @brief HCI drivers
 * @defgroup bt_hci_driver HCI drivers
 * @ingroup bluetooth
 * @{
 */

#include <stdbool.h>
#include <net/buf.h>
#include <bluetooth/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Check if an HCI event is high priority or not.
 *
 * Helper for the HCI driver to know which events are ok to be passed
 * through the RX thread and which must be given to bt_recv_prio() from
 * another context (e.g. ISR). If this function returns true it's safe
 * to pass the event through the RX thread, however if it returns false
 * then this risks a deadlock.
 *
 * @param evt HCI event code.
 *
 * @return true if the event can be processed in the RX thread, false
 *         if it cannot.
 */
static inline bool bt_hci_evt_is_prio(u8_t evt)
{
	switch (evt) {
	case BT_HCI_EVT_CMD_COMPLETE:
	case BT_HCI_EVT_CMD_STATUS:
#if defined(CONFIG_BT_CONN)
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
#endif
		return true;
	default:
		return false;
	}
}

/**
 * @brief Receive data from the controller/HCI driver.
 *
 * This is the main function through which the HCI driver provides the
 * host with data from the controller. The buffer needs to have its type
 * set with the help of bt_buf_set_type() before calling this API. This API
 * should not be used for so-called high priority HCI events, which should
 * instead be delivered to the host stack through bt_recv_prio().
 *
 * @param buf Network buffer containing data from the controller.
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_recv(struct net_buf *buf);

/**
 * @brief Receive high priority data from the controller/HCI driver.
 *
 * This is the same as bt_recv(), except that it should be used for
 * so-called high priority HCI events. There's a separate
 * bt_hci_evt_is_prio() helper that can be used to identify which events
 * are high priority.
 *
 * As with bt_recv(), the buffer needs to have its type set with the help of
 * bt_buf_set_type() before calling this API. The only exception is so called
 * high priority HCI events which should be delivered to the host stack through
 * bt_recv_prio() instead.
 *
 * @param buf Network buffer containing data from the controller.
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_recv_prio(struct net_buf *buf);

/** Possible values for the 'bus' member of the bt_hci_driver struct */
enum bt_hci_driver_bus {
	BT_HCI_DRIVER_BUS_VIRTUAL       = 0,
	BT_HCI_DRIVER_BUS_USB           = 1,
	BT_HCI_DRIVER_BUS_PCCARD        = 2,
	BT_HCI_DRIVER_BUS_UART          = 3,
	BT_HCI_DRIVER_BUS_RS232         = 4,
	BT_HCI_DRIVER_BUS_PCI           = 5,
	BT_HCI_DRIVER_BUS_SDIO          = 6,
	BT_HCI_DRIVER_BUS_SPI           = 7,
	BT_HCI_DRIVER_BUS_I2C           = 8,
};

/**
 * @brief Abstraction which represents the HCI transport to the controller.
 *
 * This struct is used to represent the HCI transport to the Bluetooth
 * controller.
 */
struct bt_hci_driver {
	/** Name of the driver */
	const char *name;

	/** Bus of the transport (BT_HCI_DRIVER_BUS_*) */
	enum bt_hci_driver_bus bus;

	/**
	 * @brief Open the HCI transport.
	 *
	 * Opens the HCI transport for operation. This function must not
	 * return until the transport is ready for operation, meaning it
	 * is safe to start calling the send() handler.
	 *
	 * If the driver uses its own RX thread, i.e.
	 * CONFIG_BT_RECV_IS_RX_THREAD is set, then this
	 * function is expected to start that thread.
	 *
	 * @return 0 on success or negative error number on failure.
	 */
	int (*open)(void);

	/**
	 * @brief Send HCI buffer to controller.
	 *
	 * Send an HCI command or ACL data to the controller. The exact
	 * type of the data can be checked with the help of bt_buf_get_type().
	 *
	 * @note This function must only be called from a cooperative thread.
	 *
	 * @param buf Buffer containing data to be sent to the controller.
	 *
	 * @return 0 on success or negative error number on failure.
	 */
	int (*send)(struct net_buf *buf);
};

/**
 * @brief Register a new HCI driver to the Bluetooth stack.
 *
 * This needs to be called before any application code runs. The bt_enable()
 * API will fail if there is no driver registered.
 *
 * @param drv A bt_hci_driver struct representing the driver.
 *
 * @return 0 on success or negative error number on failure.
 */
int bt_hci_driver_register(const struct bt_hci_driver *drv);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __BT_HCI_DRIVER_H */
