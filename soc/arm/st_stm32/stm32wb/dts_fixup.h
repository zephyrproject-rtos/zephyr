/*
 * Copyright (c) 2019 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_RTC_0_NAME				DT_LABEL(DT_INST(0, st_stm32_rtc))

#define DT_FLASH_DEV_NAME			DT_LABEL(DT_INST(0, st_stm32wb_flash_controller))

#define DT_ADC_1_NAME			DT_ST_STM32_ADC_50040000_LABEL

/* End of SoC Level DTS fixup file */
