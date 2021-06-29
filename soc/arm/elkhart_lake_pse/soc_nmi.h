/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*Note: This will be a temp file for all HW/PSS work around and
 * need to remove later.
 */

#ifndef SOC_TEMP_H
#define SOC_TEMP_H
#define REGION_NO_ACCESS_ATTR(size) { (size | NO_ACCESS_Msk) }

#define  ARM_ADDRESS_SPACE    (0x00000000U)
#define  BADDRESS_SPACE   (0x71000000U)
#define  IO_ADDRESS_SPACE     (0x40000000U)

#define SOC_PMU_TO_IOAPIC_IRQ   (10)
#define SOC_HBW_PER_FABRIC_IRQ  (14)
#define SOC_FABRIC_IRQ_PRI (0)

#define ERRNO_MASK              0xF000
#define NMI_STT_SHIFT   (24)
#define ERRNO_NMI               0x8000
#define STS_NMI         (1 << 15)

typedef struct _fw_info_ {
	uint32_t reset_cnt;
	uint32_t fuse_holder;
	uint32_t fw_step;
	uint32_t fw_status;
	uint32_t fw_error;
	uint32_t fw_signature;
} fw_info_t;

int soc_nmi_init(void);

#endif
