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

#include <net/buf.h>
#include <bluetooth/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Allocate a buffer for an HCI event
 *
 *  This will set the BT_BUF_EVT buffer type so bt_buf_set_type()
 *  doesn't need to be explicitly called. Only available when
 *  CONFIG_BLUETOOTH_HOST_BUFFERS has been selected.
 *
 *  @param opcode HCI event opcode or 0 if not known
 *  @return A new buffer with the BT_BUF_EVT type.
 */
struct net_buf *bt_buf_get_evt(uint8_t opcode);

/** Allocate a buffer for incoming ACL data
 *
 *  This will set the BT_BUF_ACL_IN buffer type so bt_buf_set_type()
 *  doesn't need to be explicitly called. Only available when
 *  CONFIG_BLUETOOTH_HOST_BUFFERS has been selected.
 *
 *  @return A new buffer with the BT_BUF_ACL_IN type.
 */
struct net_buf *bt_buf_get_acl(void);

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

	/* Open the HCI transport */
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
