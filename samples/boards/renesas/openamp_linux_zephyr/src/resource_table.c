/*
 * Copyright (c) 2024 EPAM Systems
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Overriding Zephyr resource_table and set correct vring 0 and vring 1 addresses
 * to match the work behaviour of master device. According to the commit:
 * 25ec73986b (lib: open-amp: add helper to add resource table in project)
 * vring RX and TX addresses should be allocated by master processor.
 * Current Master Sample implementation expects slave to provide valid
 * rsc_table so update vring addresses here.
 *
 * Dependencies:
 *   to be compliant with Linux kernel OS the resource table must be linked in a
 *   specific section named ".resource_table".
 */

#include <zephyr/kernel.h>
#include <resource_table.h>

#define __resource Z_GENERIC_SECTION(.resource_table)

static struct fw_resource_table __resource resource_table = {
	.ver = 1,
	.num = RSC_TABLE_NUM_ENTRY,
	.reserved = {0, 0},
	/* Offset */
	{
		offsetof(struct fw_resource_table, vdev),
	},
	/* Virtio device entry */
	{
		RSC_VDEV,
		VIRTIO_ID_RPMSG,
		0,
		RPMSG_IPU_C0_FEATURES,
		0,
		0,
		0,
		VRING_COUNT,
		{0, 0},
	},
	/* Vring rsc entry - part of vdev rsc entry */
	.vring0 = {VRING_TX_ADDR_A55, VRING_ALIGNMENT, RSC_TABLE_NUM_RPMSG_BUFF, VRING0_ID, 0},
	.vring1 = {VRING_RX_ADDR_A55, VRING_ALIGNMENT, RSC_TABLE_NUM_RPMSG_BUFF, VRING1_ID, 0},
};

void rsc_table_get(void **table_ptr, int *length)
{
	*table_ptr = (void *)&resource_table;
	*length = sizeof(resource_table);
}
