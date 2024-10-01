/*
 * Copyright 2022 NXP
 * SPDX-License-Identifier: Apache-2.0
 */


#ifndef _FLASH_CLOCK_SETUP_H_
#define _FLASH_CLOCK_SETUP_H_

#include "fsl_common.h"

void flexspi_clock_safe_config(void);
void flexspi_setup_clock(FLEXSPI_Type *base, uint32_t src, uint32_t divider);

#endif /* _FLASH_CLOCK_SETUP_H_ */
