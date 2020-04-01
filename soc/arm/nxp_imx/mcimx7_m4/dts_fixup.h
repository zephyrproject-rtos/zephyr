/*
 * Copyright (c) 2018, Diego Sueiro <diego.sueiro@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS	    DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_IPM_IMX_MU_B_BASE_ADDRESS	DT_NXP_IMX_MU_30AB0000_BASE_ADDRESS
#define DT_IPM_IMX_MU_B_IRQ		DT_NXP_IMX_MU_30AB0000_IRQ_0
#define DT_IPM_IMX_MU_B_IRQ_PRI		DT_NXP_IMX_MU_30AB0000_IRQ_0_PRIORITY
#define DT_IPM_IMX_MU_B_NAME		DT_NXP_IMX_MU_30AB0000_LABEL

/* End of SoC Level DTS fixup file */
