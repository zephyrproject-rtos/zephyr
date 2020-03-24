/*
 * Copyright (c) 2019, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS DT_ARM_V8M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS
#define DT_NUM_MPU_REGIONS   DT_PROP(DT_INST(0, arm_armv8m_mpu), arm_num_mpu_regions)

#define DT_FLASH_DEV_NAME    DT_LABEL(DT_INST(0, nxp_lpc_iap))

/* End of SoC Level DTS fixup file */
