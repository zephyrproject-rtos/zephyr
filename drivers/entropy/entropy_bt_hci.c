/*
 * Copyright (c) 2022, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_bt_hci_entropy

#include <drivers/entropy.h>
#include <bluetooth/hci.h>
#include <string.h>

static int entropy_bt_init(const struct device *dev)
{
	/* Nothing to do */
	return 0;
}

static int entropy_bt_get_entropy(const struct device *dev,
				  uint8_t *buffer, uint16_t length)
{
	struct bt_hci_rp_le_rand *rp;
	struct net_buf *rsp;
	int req, ret;

	if (!bt_is_ready()) {
		return -EAGAIN;
	}

	while (length > 0) {
		/* Number of bytes to fill on this iteration */
		req = MIN(length, sizeof(rp->rand));
		/* Request the next 8 bytes over HCI */
		ret = bt_hci_cmd_send_sync(BT_HCI_OP_LE_RAND, NULL, &rsp);
		if (ret) {
			return ret;
		}
		/* Copy random data into buffer */
		rp = (void *)rsp->data;
		memcpy(buffer, rp->rand, req);
		buffer += req;
		length -= req;
	}
	return 0;
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
