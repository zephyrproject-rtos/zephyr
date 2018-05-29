/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCE_TABLE_H__
#define RESOURCE_TABLE_H__

#include <openamp/open_amp.h>

#define RSC_TABLE_ADDRESS       0x04000000

OPENAMP_PACKED_BEGIN
struct lpc_resource_table {
	uint32_t ver;
	uint32_t num;
	uint32_t reserved[2];
	uint32_t offset[2];
	struct fw_rsc_rproc_mem mem;
	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0, vring1;
} OPENAMP_PACKED_END;

void resource_table_init(void **table_ptr, int *length);

#endif

