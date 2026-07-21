/*
 * Copyright (c) 2023, Advanced Micro Devices Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _IPM_XLNX_IPI_H_
#define _IPM_XLNX_IPI_H_

/* IPI Channel ID bits */
#define IPI_CH0_BIT  0
#define IPI_CH1_BIT  8
#define IPI_CH2_BIT  9
#define IPI_CH3_BIT  16
#define IPI_CH4_BIT  17
#define IPI_CH5_BIT  18
#define IPI_CH6_BIT  19
#define IPI_CH7_BIT  24
#define IPI_CH8_BIT  25
#define IPI_CH9_BIT  26
#define IPI_CH10_BIT 27

/* Register offsets */
#define IPI_TRIG 0x00
#define IPI_OBS  0x04
#define IPI_ISR  0x10
#define IPI_IMR  0x14
#define IPI_IER  0x18
#define IPI_IDR  0x1C

#endif /* _IPM_XLNX_IPI_H_ */
