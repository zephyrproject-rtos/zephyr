/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS	DT_ARM_V8M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_NUM_MPU_REGIONS		DT_ARM_ARMV8M_MPU_E000ED90_ARM_NUM_MPU_REGIONS

#if defined (CONFIG_ARM_NONSECURE_FIRMWARE)

/* SCC */
#define DT_ARM_SCC_BASE_ADDRESS			DT_ARM_SCC_4010B000_BASE_ADDRESS

#else

/* SCC */
#define DT_ARM_SCC_BASE_ADDRESS			DT_ARM_SCC_5010B000_BASE_ADDRESS

#endif

/* End of SoC Level DTS fixup file */
