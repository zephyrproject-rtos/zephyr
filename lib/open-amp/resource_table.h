/*
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCE_TABLE_H__
#define RESOURCE_TABLE_H__

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>

#ifdef __cplusplus
extern "C" {
#endif

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)

#define VDEV_ID                 0xFF
#define VRING0_ID               CONFIG_OPENAMP_RSC_TABLE_IPM_RX_ID /* (host to remote) */
#define VRING1_ID               CONFIG_OPENAMP_RSC_TABLE_IPM_TX_ID /* (remote to host) */

#define VRING_COUNT             2
#define RPMSG_IPU_C0_FEATURES   1

#define VRING_RX_ADDRESS        -1  /* allocated by Master processor */
#define VRING_TX_ADDRESS        -1  /* allocated by Master processor */
#define VRING_BUFF_ADDRESS      -1  /* allocated by Master processor */
#define VRING_ALIGNMENT         16  /* fixed to match with Linux constraint */

#endif

enum rsc_table_entries {
#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)
	RSC_TABLE_VDEV_ENTRY,
#endif
#if defined(CONFIG_RAM_CONSOLE)
	RSC_TABLE_TRACE_ENTRY,
#endif
	RSC_TABLE_NUM_ENTRY
};

struct fw_resource_table {
	struct resource_table hdr;
	uint32_t offset[RSC_TABLE_NUM_ENTRY];

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)
	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0;
	struct fw_rsc_vdev_vring vring1;
#endif

#if defined(CONFIG_RAM_CONSOLE)
	/* rpmsg trace entry */
	struct fw_rsc_trace cm_trace;
#endif
} METAL_PACKED_END;

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)
	#define vdev_offset	offsetof(struct fw_resource_table, vdev),
#else
	#define vdev_offset
#endif

#if defined(CONFIG_RAM_CONSOLE)
	#define cm_trace_offset	offsetof(struct fw_resource_table, cm_trace),
#else
	#define cm_trace_offset
#endif

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)
	#define vdev_entry							\
	.vdev = {								\
		RSC_VDEV, VIRTIO_ID_RPMSG, 0, RPMSG_IPU_C0_FEATURES, 0, 0, 0,	\
		VRING_COUNT, {0, 0},						\
	},									\
	.vring0 = {VRING_TX_ADDRESS, VRING_ALIGNMENT,				\
		   CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF,			\
		   VRING0_ID, 0},						\
	.vring1 = {VRING_RX_ADDRESS, VRING_ALIGNMENT,				\
		   CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF,			\
		   VRING1_ID, 0},
#else
	#define vdev_entry
#endif

#if defined(CONFIG_RAM_CONSOLE)
	#define cm_trace_entry							\
		.cm_trace = {							\
			RSC_TRACE,						\
			(uint32_t)ram_console_buf, CONFIG_RAM_CONSOLE_BUFFER_SIZE, 0,\
			"Zephyr_log",						\
		},
#else
	#define cm_trace_entry
#endif

#define RESOURCE_TABLE_INIT			\
{						\
	.hdr = {				\
		.ver = 1,			\
		.num = RSC_TABLE_NUM_ENTRY,	\
	},					\
	.offset = {				\
		vdev_offset			\
		cm_trace_offset			\
	},					\
	vdev_entry				\
	cm_trace_entry				\
}

void rsc_table_get(struct fw_resource_table **table_ptr, int *length);

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)

inline struct fw_rsc_vdev *rsc_table_to_vdev(struct fw_resource_table *rsc_table)
{
	return &rsc_table->vdev;
}

inline struct fw_rsc_vdev_vring *rsc_table_get_vring0(struct fw_resource_table *rsc_table)
{
	return &rsc_table->vring0;
}

inline struct fw_rsc_vdev_vring *rsc_table_get_vring1(struct fw_resource_table *rsc_table)
{
	return &rsc_table->vring1;
}

#endif

#ifdef __cplusplus
}
#endif

#endif
