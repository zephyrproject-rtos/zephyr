/*
 * Copyright (c) 2023 Silicom Connectivity Solutions, Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_RESET_MEC172X_RESET_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_RESET_MEC172X_RESET_H_

#include "xec-common.h"

/* RC bus reset register offset */
#define XEC_PCR_PERIPH_RST0  0x70
#define XEC_PCR_PERIPH_RST1  0x74
#define XEC_PCR_PERIPH_RST2  0x78
#define XEC_PCR_PERIPH_RST3  0x7C
#define XEC_PCR_PERIPH_RST4  0x80

/* I2C resets */
#define MEC172X_RESET_I2C0	XEC_RESET(1, 10U)
#define MEC172X_RESET_I2C1	XEC_RESET(3, 13U)
#define MEC172X_RESET_I2C2	XEC_RESET(3, 14U)
#define MEC172X_RESET_I2C3	XEC_RESET(3, 15U)
#define MEC172X_RESET_I2C4	XEC_RESET(3, 20U)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_RESET_MEC172X_RESET_H_ */
