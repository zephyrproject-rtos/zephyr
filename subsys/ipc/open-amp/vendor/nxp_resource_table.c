/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * NXP resource table
 *
 * Remote processors include a "resource table" section in addition
 * to standard ELF segments.
 *
 * The resource table lists system resources needed before powering on,
 * like contiguous memory allocation or IOMMU mapping.
 * It also includes entries for supported features and configurations,
 * such as trace buffers and virtio devices.
 *
 * Dependencies:
 *   Must be linked in the ".resource_table" section to comply with
 *   Linux kernel OS.
 *
 * Related documentation:
 *   https://www.kernel.org/doc/Documentation/remoteproc.txt
 *   https://openamp.readthedocs.io/en/latest/protocol_details/lifecyclemgmt.html
 */

#include "../vendor/nxp_resource_table.h"

#include <zephyr/kernel.h>

#define __resource Z_GENERIC_SECTION(.resource_table)

static struct nxp_resource_table __resource nxp_rsc_table = {
	.hdr = {
		.ver = 1,
		.num = 2,
	},
	.offset = {
		offsetof(struct nxp_resource_table, imx_vs_entry),
		offsetof(struct nxp_resource_table, vdev),
	},
	.imx_vs_entry = {
		.type = RSC_VENDOR_START,
		/* length of the resource entry, without type, but including len */
		.len = 0x10,
		/* FW_RSC_NXP_S_MAGIC, 'n''x''p''s' */
		.magic_num = 0x6E787073,
		.version = 0x0,
		/* require the host NOT to wait for a FW_READY response */
		.features = 0x1,
	},
	/* 8 is the number of rpmsg buffers used */
	VDEV_ENTRY_HELPER(8)
};

void rsc_table_get(void **table_ptr, int *length)
{
	*length = sizeof(nxp_rsc_table);
	*table_ptr = &nxp_rsc_table;
}

struct fw_rsc_vdev *rsc_table_to_vdev(void *rsc_table)
{
	return &((struct nxp_resource_table *)rsc_table)->vdev;
}

struct fw_rsc_vdev_vring *rsc_table_get_vring0(void *rsc_table)
{
	return &((struct nxp_resource_table *)rsc_table)->vring0;
}

struct fw_rsc_vdev_vring *rsc_table_get_vring1(void *rsc_table)
{
	return &((struct nxp_resource_table *)rsc_table)->vring1;
}
