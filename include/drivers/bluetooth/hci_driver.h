/** @file
 *  @brief Bluetooth HCI driver API.
 */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#if defined(CONFIG_BLUETOOTH_RECV_IS_RX_THREAD)
/** Helper for the HCI driver to know which events are ok to be passed
  * through the RX thread and which must be given to bt_recv() from another
  * context. If this function returns true it's safe to pass the event
  * through the RX thread, however if it returns false then this risks
  * a deadlock.
  *
  * @param evt HCI event code.
  *
  * @return true if the event can be processed in the RX thread, false
  *         if it cannot.
  */
static inline bool bt_hci_evt_is_prio(uint8_t evt)
{
	switch (evt) {
	case BT_HCI_EVT_CMD_COMPLETE:
	case BT_HCI_EVT_CMD_STATUS:
#if defined(CONFIG_BLUETOOTH_CONN)
	case BT_HCI_EVT_NUM_COMPLETED_PACKETS:
#endif
		return true;
	default:
		return false;
	}
}
#endif

/* Receive data from the controller/HCI driver */
int bt_recv(struct net_buf *buf);

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

struct bt_hci_driver {
	/* Name of the driver */
	const char *name;

	/* Bus of the transport (BT_HCI_DRIVER_BUS_*) */
	enum bt_hci_driver_bus bus;

	/* Open the HCI transport. If the driver uses its own RX thread,
	 * i.e. CONFIG_BLUETOOTH_RECV_IS_RX_THREAD is set, then this
	 * function is expected to start that thread.
	 */
	int (*open)(void);

	/* Send HCI buffer to controller */
	int (*send)(struct net_buf *buf);
};

/* Register a new HCI driver to the Bluetooth stack */
int bt_hci_driver_register(struct bt_hci_driver *drv);

/* Unregister a previously registered HCI driver */
void bt_hci_driver_unregister(struct bt_hci_driver *drv);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* __BT_HCI_DRIVER_H */
