/*
 * Copyright (c) 2024 EPAM Systems
 * Copyright (c) 2024 Renesas Electronics Corporation
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef RESOURCE_TABLE_H__
#define RESOURCE_TABLE_H__

#include <openamp/remoteproc.h>
#include <openamp/virtio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VDEV_ID   0xFF
#define VRING0_ID 0
#define VRING1_ID 1

#define VRING_COUNT           2
#define RPMSG_IPU_C0_FEATURES 1

#define R_VRING_TX         DT_NODELABEL(vring_ctrl0)
#define R_VRING_RX         DT_NODELABEL(vring_ctrl1)
#define VRING_TX_ADDR_CM33 DT_REG_ADDR(R_VRING_TX)
#define VRING_RX_ADDR_CM33 DT_REG_ADDR(R_VRING_RX)

/*
 * Macro to convert across CM33/CM33_FPU and A55 address spaces
 * This macro are designed to convert CM33 addresses to A55 addresses
 * in secure and non-secure space. According to Table 5.2 of HW Manual.
 */
#ifndef _ASMLANGUAGE

#define CM33_ADDRESS_OFFSET_SECURE    (0x30000000)
#define CM33_ADDRESS_OFFSET_NONSECURE (0x20000000)
#define CM33_TO_A55_ADDR_S(x)         ((x) - CM33_ADDRESS_OFFSET_SECURE)
#define CM33_TO_A55_ADDR_NS(x)        ((x) - CM33_ADDRESS_OFFSET_NONSECURE)

#endif
/* End of convert macro */

#define VRING_TX_ADDR_A55 CM33_TO_A55_ADDR_NS(VRING_TX_ADDR_CM33)
#define VRING_RX_ADDR_A55 CM33_TO_A55_ADDR_NS(VRING_RX_ADDR_CM33)
#define VRING_ALIGNMENT   (0x100U)

#define RSC_TABLE_NUM_RPMSG_BUFF 512

enum rsc_table_entries {
	RSC_TABLE_VDEV_ENTRY,
	RSC_TABLE_NUM_ENTRY
};

struct fw_resource_table {
	unsigned int ver;
	unsigned int num;
	unsigned int reserved[2];
	unsigned int offset[RSC_TABLE_NUM_ENTRY];

	struct fw_rsc_vdev vdev;
	struct fw_rsc_vdev_vring vring0;
	struct fw_rsc_vdev_vring vring1;
} METAL_PACKED_END;

void rsc_table_get(void **table_ptr, int *length);

static inline struct fw_rsc_vdev *rsc_table_to_vdev(void *rsc_table)
{
	return &((struct fw_resource_table *)rsc_table)->vdev;
}

static inline struct fw_rsc_vdev_vring *rsc_table_get_vring0(void *rsc_table)
{
	return &((struct fw_resource_table *)rsc_table)->vring0;
}

static inline struct fw_rsc_vdev_vring *rsc_table_get_vring1(void *rsc_table)
{
	return &((struct fw_resource_table *)rsc_table)->vring1;
}

#ifdef __cplusplus
}
#endif

#endif
