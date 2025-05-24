/*
 * Copyright (c) 2020 STMicroelectronics
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCE_TABLE_INIT_H__
#define RESOURCE_TABLE_INIT_H__

	.hdr = {
		.ver = 1,
		.num = RSC_TABLE_NUM_ENTRY,
	},
	.offset = {

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)
		offsetof(struct fw_resource_table, vdev),
#endif

#if defined(CONFIG_RAM_CONSOLE)
		offsetof(struct fw_resource_table, cm_trace),
#endif

#if defined(CONFIG_OPENAMP_VENDOR_RSC_TABLE_ENTRY)
		offsetof(struct fw_resource_table, vendor_type),
#endif
	},

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)
	/* Virtio device entry */
	.vdev = {
		RSC_VDEV, VIRTIO_ID_RPMSG, 0, RPMSG_IPU_C0_FEATURES, 0, 0, 0,
		VRING_COUNT, {0, 0},
	},

	/* Vring rsc entry - part of vdev rsc entry */
	.vring0 = {VRING_TX_ADDRESS, VRING_ALIGNMENT,
		   CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF,
		   VRING0_ID, 0},
	.vring1 = {VRING_RX_ADDRESS, VRING_ALIGNMENT,
		   CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF,
		   VRING1_ID, 0},
#endif

#if defined(CONFIG_RAM_CONSOLE)
	.cm_trace = {
		RSC_TRACE,
		(uint32_t)ram_console_buf, CONFIG_RAM_CONSOLE_BUFFER_SIZE, 0,
		"Zephyr_log",
	},
#endif

#if defined(CONFIG_OPENAMP_VENDOR_RSC_TABLE_ENTRY)
	/* Initialize vendor-specific type */
	.vendor_type = CONFIG_OPENAMP_VENDOR_RSC_TYPE,
#endif


#endif
