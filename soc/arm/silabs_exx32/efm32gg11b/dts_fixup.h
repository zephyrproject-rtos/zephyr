/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_NUM_IRQ_PRIO_BITS		DT_ARM_V7M_NVIC_E000E100_ARM_NUM_IRQ_PRIORITY_BITS

#define DT_FLASH_DEV_BASE_ADDRESS	DT_SILABS_GECKO_FLASH_CONTROLLER_40000000_BASE_ADDRESS
#define DT_FLASH_DEV_NAME		DT_SILABS_GECKO_FLASH_CONTROLLER_40000000_LABEL

#define DT_GPIO_GECKO_SWO_LOCATION	DT_SILABS_GECKO_GPIO_40088400_LOCATION_SWO

#define DT_RTC_0_NAME			DT_LABEL(DT_INST(0, silabs_gecko_rtcc))

#define DT_WDT_0_NAME			DT_LABEL(DT_INST(0, silabs_gecko_wdog))
#define DT_WDT_1_NAME			DT_LABEL(DT_INST(1, silabs_gecko_wdog))

/* End of SoC Level DTS fixup file */
