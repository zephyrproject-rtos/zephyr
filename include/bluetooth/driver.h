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

enum bt_buf_type {
	BT_CMD,			/** HCI command */
	BT_EVT,			/** HCI event */
	BT_ACL_OUT,		/** Outgoing ACL data */
	BT_ACL_IN,		/** Incoming ACL data */
};

/* Allocate a buffer for an HCI event */
struct net_buf *bt_buf_get_evt(void);

/* Allocate a buffer for incoming ACL data */
struct net_buf *bt_buf_get_acl(void);

/* Receive data from the controller/HCI driver */
void bt_recv(struct net_buf *buf);

struct bt_driver {
	/* Open the HCI transport */
	int (*open)(void);

	/* Send buffer command to controller */
	int (*send)(enum bt_buf_type type, struct net_buf *buf);
};

/* Register a new HCI driver to the Bluetooth stack */
int bt_driver_register(struct bt_driver *drv);

/* Unregister a previously registered HCI driver */
void bt_driver_unregister(struct bt_driver *drv);

#endif /* __BT_DRIVER_H */
