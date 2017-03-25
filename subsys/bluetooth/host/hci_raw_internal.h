/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __BT_HCI_RAW_INTERNAL_H
#define __BT_HCI_RAW_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

struct bt_dev_raw {
	/* Registered HCI driver */
	const struct bt_hci_driver *drv;
};

extern struct bt_dev_raw bt_dev;

#ifdef __cplusplus
}
#endif

#endif /* __BT_HCI_RAW_INTERNAL_H */
