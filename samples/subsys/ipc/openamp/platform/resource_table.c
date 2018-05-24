/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Implement the resource table that will get parsed by OpenAMP to convey
 * the shared memory region used for message passing and ring setup.
 */

#include "platform.h"
#include "resource_table.h"

struct lpc_resource_table *rsc_table_ptr = (void *) RSC_TABLE_ADDRESS;

#if defined(CPU_LPC54114J256BD64_cm4)
static const struct lpc_resource_table rsc_table = {
	.ver = 1,
	.num = 2,
	.offset = {
		offsetof(struct lpc_resource_table, mem),
		offsetof(struct lpc_resource_table, vdev),
	},
	.mem = {
		RSC_RPROC_MEM, SHM_START_ADDRESS, SHM_START_ADDRESS, SHM_SIZE,
		0,
	},

	.vdev = {
		RSC_VDEV, VIRTIO_ID_RPMSG, 0, 1 << VIRTIO_RPMSG_F_NS, 0, 0, 0,
		VRING_COUNT, { 0, 0 },
	},

	.vring0 = { VRING_TX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, 1, 0 },
	.vring1 = { VRING_RX_ADDRESS, VRING_ALIGNMENT, VRING_SIZE, 2, 0 },
};
#endif

void resource_table_init(void **table_ptr, int *length)
{
#if defined(CPU_LPC54114J256BD64_cm4)
	/* Master: copy the resource table to shared memory. */
	memcpy(rsc_table_ptr, &rsc_table, sizeof(struct lpc_resource_table));
#endif

	*length = sizeof(struct lpc_resource_table);
	*table_ptr = rsc_table_ptr;
}
