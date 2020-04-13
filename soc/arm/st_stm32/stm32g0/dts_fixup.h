/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS			DT_ARM_V6M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_FLASH_DEV_NAME			DT_LABEL(DT_INST(0, st_stm32g0_flash_controller))

#define DT_PWM_STM32_3_DEV_NAME			DT_ST_STM32_PWM_40000400_PWM_LABEL
#define DT_PWM_STM32_3_PRESCALER		DT_ST_STM32_PWM_40000400_PWM_ST_PRESCALER

#define DT_WDT_0_NAME			DT_LABEL(DT_INST(0, st_stm32_watchdog))

/* End of SoC Level DTS fixup file */
