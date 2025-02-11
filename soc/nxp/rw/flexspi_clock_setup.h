/*
 * Copyright 2022-2023 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FLEXSPI_CLOCK_SETUP_H_
#define _FLEXSPI_CLOCK_SETUP_H_

#include "fsl_common.h"

void set_flexspi_clock(FLEXSPI_Type *base, uint32_t src, uint32_t divider);

#endif /* _FLEXSPI_CLOCK_SETUP_H_ */
