/*
 * Copyright (c) 2019 Richard Osterloh <richard.osterloh@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS			DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_FLASH_DEV_NAME			DT_LABEL(DT_INST(0, st_stm32g4_flash_controller))

#define DT_RTC_0_NAME				DT_LABEL(DT_INST(0, st_stm32_rtc))

#define DT_WDT_0_NAME				DT_LABEL(DT_INST(0, st_stm32_watchdog))

/* End of SoC Level DTS fixup file */
