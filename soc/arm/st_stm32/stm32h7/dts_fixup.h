/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS		DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_NUM_MPU_REGIONS		DT_ARM_ARMV7M_MPU_E000ED90_ARM_NUM_MPU_REGIONS

#define DT_RTC_0_NAME				DT_LABEL(DT_INST(0, st_stm32_rtc))

/* End of SoC Level DTS fixup file */
