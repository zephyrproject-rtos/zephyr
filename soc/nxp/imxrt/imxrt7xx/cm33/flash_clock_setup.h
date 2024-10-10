/*
 * Copyright 2024 NXP
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _FLASH_CLOCK_SETUP_H_
#define _FLASH_CLOCK_SETUP_H_

#include "fsl_common.h"

#ifdef CONFIG_SOC_MIMXRT798S_CPU0
#include "MIMXRT798S_cm33_core0.h"
#endif

void BOARD_XspiClockSafeConfig(void);
void BOARD_SetXspiClock(XSPI_Type *base, uint32_t src, uint32_t divider);

#endif /* _FLASH_CLOCK_SETUP_H_ */
