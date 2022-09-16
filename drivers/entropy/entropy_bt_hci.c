/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_bt_hci_entropy

#include <zephyr/drivers/entropy.h>
#include <zephyr/bluetooth/hci.h>
#include <string.h>

static int entropy_bt_init(const struct device *dev)
{
	/* Nothing to do */
	return 0;
}

static int entropy_bt_get_entropy(const struct device *dev,
				  uint8_t *buffer, uint16_t length)
{
	if (!bt_is_ready()) {
		return -EAGAIN;
	}

	return bt_hci_le_rand(buffer, length);
}

/* HCI commands cannot be run from an interrupt context */
static const struct entropy_driver_api entropy_bt_api = {
	.get_entropy = entropy_bt_get_entropy,
	.get_entropy_isr = NULL
};

#define ENTROPY_BT_HCI_INIT(inst)				  \
	DEVICE_DT_INST_DEFINE(inst, entropy_bt_init,		  \
			      NULL, NULL, NULL,			  \
			      PRE_KERNEL_1,			  \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, \
			      &entropy_bt_api);

DT_INST_FOREACH_STATUS_OKAY(ENTROPY_BT_HCI_INIT)
