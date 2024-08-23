/*
 * Copyright (c) 2023-2024 MUNIC SA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */
#ifndef __INCLUDE_DT_BINDINGS_LPM_RA2_LPM_H__
#define __INCLUDE_DT_BINDINGS_LPM_RA2_LPM_H__

/* Definitions of modules valid for RA2 series only (for now) */
/* The modules IDs encoded as: (MSTPCRxx_reg_id + bit_offset + 1),
 * where MSTPCRxx_reg_id is one of:
 *   MSTPCRA	0
 *   MSTPCRB	32
 *   MSTPCRC	64
 *   MSTPCRD	96
 *
 * Following above rule, as an example, the LPM_RA_DTC will be encoded
 * as (MSTPCRA + 22 + 1) = 23, whilst LPM_RA_CAN0 will be (MSTPCRB + 2 + 1) = 33
 */
#define LPM_RA_MSTPCRA		0
#define LPM_RA_MSTPCRB		32
#define LPM_RA_MSTPCRC		64
#define LPM_RA_MSTPCRD		96

#define LPM_RA_MSTPCR_Pos	5
#define LPM_RA_MSTPCR_Msk	GENMASK(LPM_RA_MSTPCR_Pos - 1, 0)

#define LPM_RA_DTC	(LPM_RA_MSTPCRA + 22 + 1)

#define LPM_RA_CAN0	(LPM_RA_MSTPCRB + 2 + 1)
#define LPM_RA_IIC1	(LPM_RA_MSTPCRB + 8 + 1)
#define LPM_RA_IIC0	(LPM_RA_MSTPCRB + 9 + 1)
#define LPM_RA_SPI1	(LPM_RA_MSTPCRB + 18 + 1)
#define LPM_RA_SPI0	(LPM_RA_MSTPCRB + 19 + 1)
#define LPM_RA_SCI9	(LPM_RA_MSTPCRB + 22 + 1)
#define LPM_RA_SCI3	(LPM_RA_MSTPCRB + 28 + 1)
#define LPM_RA_SCI2	(LPM_RA_MSTPCRB + 29 + 1)
#define LPM_RA_SCI1	(LPM_RA_MSTPCRB + 30 + 1)
#define LPM_RA_SCI0	(LPM_RA_MSTPCRB + 31 + 1)

#define LPM_RA_CAC	(LPM_RA_MSTPCRC + 0 + 1)
#define LPM_RA_CRC	(LPM_RA_MSTPCRC + 1 + 1)
#define LPM_RA_CTSU	(LPM_RA_MSTPCRC + 3 + 1)
#define LPM_RA_DOC	(LPM_RA_MSTPCRC + 13 + 1)
#define LPM_RA_ELC	(LPM_RA_MSTPCRC + 14 + 1)
#define LPM_RA_TRNG	(LPM_RA_MSTPCRC + 28 + 1)
#define LPM_RA_AES	(LPM_RA_MSTPCRC + 31 + 1)

#define LPM_RA_AGT1	(LPM_RA_MSTPCRD + 2 + 1)
#define LPM_RA_AGT0	(LPM_RA_MSTPCRD + 3 + 1)
#define LPM_RA_GPT_LO	(LPM_RA_MSTPCRD + 5 + 1)
#define LPM_RA_GPT_HI	(LPM_RA_MSTPCRD + 6 + 1)
#define LPM_RA_POEG	(LPM_RA_MSTPCRD + 14 + 1)
#define LPM_RA_ADC120	(LPM_RA_MSTPCRD + 16 + 1)
#define LPM_RA_DAC12	(LPM_RA_MSTPCRD + 20 + 1)
#define LPM_RA_ACMPLP	(LPM_RA_MSTPCRD + 29 + 1)

#endif /* __INCLUDE_DT_BINDINGS_LPM_RA2_LPM_H__ */
