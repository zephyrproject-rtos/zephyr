/*
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS	        DT_ARM_V6M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_FLASH_DEV_BASE_ADDRESS		DT_ST_STM32G0_FLASH_CONTROLLER_40022000_BASE_ADDRESS
#define DT_FLASH_DEV_NAME			    DT_ST_STM32G0_FLASH_CONTROLLER_40022000_LABEL

/* End of SoC Level DTS fixup file */
