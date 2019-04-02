/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_FLASH_DEV_NAME DT_ATMEL_SAM0_NVMCTRL_0_LABEL

#define CONFIG_WDT_0_NAME DT_ATMEL_SAM0_WATCHDOG_0_LABEL

#define CONFIG_COUNTER_0_NAME DT_ATMEL_SAM0_TC32_0_LABEL

#define DT_NUM_IRQ_PRIO_BITS	DT_ARM_V6M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

/* End of SoC Level DTS fixup file */
