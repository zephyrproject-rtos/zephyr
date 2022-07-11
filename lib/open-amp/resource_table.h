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
#define VRING0_ID 0 /* (master to remote) fixed to 0 for Linux compatibility */
#define VRING1_ID 1 /* (remote to master) fixed to 1 for Linux compatibility */

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
	unsigned int ver;
	unsigned int num;
	unsigned int reserved[2];
	unsigned int offset[RSC_TABLE_NUM_ENTRY];

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

void rsc_table_get(void **table_ptr, int *length);

#if (CONFIG_OPENAMP_RSC_TABLE_NUM_RPMSG_BUFF > 0)

inline struct fw_rsc_vdev *rsc_table_to_vdev(void *rsc_table)
{
	return &((struct fw_resource_table *)rsc_table)->vdev;
}

inline struct fw_rsc_vdev_vring *rsc_table_get_vring0(void *rsc_table)
{
	return &((struct fw_resource_table *)rsc_table)->vring0;
}

inline struct fw_rsc_vdev_vring *rsc_table_get_vring1(void *rsc_table)
{
	return &((struct fw_resource_table *)rsc_table)->vring1;
}

#endif

#ifdef __cplusplus
}
#endif

#endif
