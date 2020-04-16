/*
 * Copyright (c) 2019 Interay Solutions B.V.
 * Copyright (c) 2019 Oane Kingma
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* SoC level DTS fixup file */

#define DT_FLASH_DEV_BASE_ADDRESS	DT_SILABS_GECKO_FLASH_CONTROLLER_40000000_BASE_ADDRESS
#define DT_FLASH_DEV_NAME		DT_SILABS_GECKO_FLASH_CONTROLLER_40000000_LABEL

#define DT_GPIO_GECKO_SWO_LOCATION	DT_SILABS_GECKO_GPIO_40088400_LOCATION_SWO

#define DT_RTC_0_NAME			DT_LABEL(DT_INST(0, silabs_gecko_rtcc))

/* End of SoC Level DTS fixup file */
