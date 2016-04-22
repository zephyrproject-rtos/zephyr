/** @file
 *  @brief Bluetooth HCI driver API.
 */

/*
 * Copyright (c) 2015 Intel Corporation
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
#ifndef __BT_DRIVER_H
#define __BT_DRIVER_H

#include <net/buf.h>
#include <bluetooth/buf.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_BLUETOOTH_HOST_BUFFERS)

/** Allocate a buffer for an HCI event
 *
 *  This will set the BT_BUF_EVT buffer type so bt_buf_set_type()
 *  doesn't need to be explicitly called.
 *
 *  @return A new buffer with the BT_BUF_EVT type.
 */
struct net_buf *bt_buf_get_evt(void);

/** Allocate a buffer for incoming ACL data
 *
 *  This will set the BT_BUF_ACL_IN buffer type so bt_buf_set_type()
 *  doesn't need to be explicitly called.
 *
 *  @return A new buffer with the BT_BUF_ACL_IN type.
 */
struct net_buf *bt_buf_get_acl(void);

#endif /* CONFIG_BLUETOOTH_HOST_BUFFERS */

/* Receive data from the controller/HCI driver */
int bt_recv(struct net_buf *buf);

enum bt_driver_bus {
	BT_DRIVER_BUS_VIRTUAL       = 0,
	BT_DRIVER_BUS_USB           = 1,
	BT_DRIVER_BUS_PCCARD        = 2,
	BT_DRIVER_BUS_UART          = 3,
	BT_DRIVER_BUS_RS232         = 4,
	BT_DRIVER_BUS_PCI           = 5,
	BT_DRIVER_BUS_SDIO          = 6,
	BT_DRIVER_BUS_SPI           = 7,
	BT_DRIVER_BUS_I2C           = 8,
};

struct bt_driver {
	/* Name of the driver */
	const char *name;

	/* Bus of the transport (BT_DRIVER_BUS_*) */
	enum bt_driver_bus bus;

	/* Open the HCI transport */
	int (*open)(void);

	/* Send HCI buffer to controller */
	int (*send)(struct net_buf *buf);
};

/* Register a new HCI driver to the Bluetooth stack */
int bt_driver_register(struct bt_driver *drv);

/* Unregister a previously registered HCI driver */
void bt_driver_unregister(struct bt_driver *drv);

#ifdef __cplusplus
}
#endif

#endif /* __BT_DRIVER_H */
