/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

/* CCM configuration */
#define DT_DCCM_BASE_ADDRESS       DT_ARC_DCCM_80000000_BASE_ADDRESS
#define DT_DCCM_SIZE               (DT_ARC_DCCM_80000000_SIZE >> 10)

#define DT_ICCM_BASE_ADDRESS       DT_ARC_ICCM_60000000_BASE_ADDRESS
#define DT_ICCM_SIZE               (DT_ARC_ICCM_60000000_SIZE >> 10)

/*
 * GPIO configuration
 */
#define DT_INST_0_SNPS_DESIGNWARE_GPIO_IRQ_0_FLAGS	0
#define DT_INST_1_SNPS_DESIGNWARE_GPIO_IRQ_0_FLAGS	0

/* End of SoC Level DTS fixup file */
