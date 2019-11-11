/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS DT_ARM_V8M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS
#define DT_NUM_MPU_REGIONS   DT_INST_0_ARM_ARMV8M_MPU_ARM_NUM_MPU_REGIONS

#define DT_FLASH_DEV_NAME    DT_INST_0_NXP_LPC_IAP_LABEL

/* End of SoC Level DTS fixup file */
