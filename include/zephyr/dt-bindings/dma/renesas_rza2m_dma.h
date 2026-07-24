/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief RZ/A2M DMA devicetree bindings flags and request-source definitions.
 *
 * This header provides the bit-field flags and pre-packed request-source
 * ( @c RZA2M_DMA_RS_* ) macros used to populate the @c dmas devicetree
 * property for Renesas RZ/A2M SoCs.
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZA2M_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZA2M_H_

/** @cond INTERNAL_HIDDEN */

/** Detection at edge sense */
#define RZA2M_DMA_F_LVL_EDGE  0x0U
/** Detection at level sense */
#define RZA2M_DMA_F_LVL_LEVEL 0x10000U
/** DMAACK output mode initial value */
#define RZA2M_DMA_F_AM_NONE   0x0U
/** Level mode DMA acknowledge */
#define RZA2M_DMA_F_AM_LEVEL  0x20000U
/** Bus cycle mode DMA acknowledge */
#define RZA2M_DMA_F_AM_BUS    0x40000U
/** DMAACK not output, required for auto request by STG and for SCIFA */
#define RZA2M_DMA_F_AM_NO_ACK 0x80000U
/** Request direction derived from the transfer direction at runtime */
#define RZA2M_DMA_F_REQD_AUTO 0x0U
/** Request issued on the source address */
#define RZA2M_DMA_F_REQD_SRC  0x100000U
/** Request issued on the destination address */
#define RZA2M_DMA_F_REQD_DST  0x200000U

/** @endcond */

/**
 * @brief Pack a DMARS request code and its fixed CHCFG flags into a slot value.
 *
 * @param dmars DMARS request value encoded in bits [15:0].
 * @param lvl  Detection level value encoded in bít [16].
 * @param am Acknowledge mode value encoded in bits [19:17].
 * @param reqd Request direction value encoded in bits [21:20].
 *
 */

#define RZA2M_DMA_RS(dmars, lvl, am, reqd) (((dmars) & 0xFFFFU) | (lvl) | (am) | (reqd))

/** Memory-to-memory transfer (software triggered) */
#define RZA2M_DMA_MEM_2_MEM 0x0U

/** OSTM0 timer interrupt */
#define RZA2M_DMA_RS_OSTM0TINT                                                                     \
	RZA2M_DMA_RS(0x023U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_AUTO)
/** OSTM1 timer interrupt */
#define RZA2M_DMA_RS_OSTM1TINT                                                                     \
	RZA2M_DMA_RS(0x027U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_AUTO)
/** OSTM2 timer interrupt */
#define RZA2M_DMA_RS_OSTM2TINT                                                                     \
	RZA2M_DMA_RS(0x02BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 0, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_0                                                                        \
	RZA2M_DMA_RS(0x043U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 0, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_0                                                                        \
	RZA2M_DMA_RS(0x047U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 0, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_0                                                                        \
	RZA2M_DMA_RS(0x04BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 0, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_0                                                                        \
	RZA2M_DMA_RS(0x04FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 1, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_1                                                                        \
	RZA2M_DMA_RS(0x053U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 1, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_1                                                                        \
	RZA2M_DMA_RS(0x057U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 2, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_2                                                                        \
	RZA2M_DMA_RS(0x05BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 2, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_2                                                                        \
	RZA2M_DMA_RS(0x05FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 3, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_3                                                                        \
	RZA2M_DMA_RS(0x063U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 3, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_3                                                                        \
	RZA2M_DMA_RS(0x067U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 3, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_3                                                                        \
	RZA2M_DMA_RS(0x06BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 3, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_3                                                                        \
	RZA2M_DMA_RS(0x06FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 4, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_4                                                                        \
	RZA2M_DMA_RS(0x073U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 4, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_4                                                                        \
	RZA2M_DMA_RS(0x077U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 4, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_4                                                                        \
	RZA2M_DMA_RS(0x07BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 4, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_4                                                                        \
	RZA2M_DMA_RS(0x07FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 4, overflow/underflow interrupt of TCNT */
#define RZA2M_DMA_RS_TCIV_4                                                                        \
	RZA2M_DMA_RS(0x083U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 5, compare match/input capture U */
#define RZA2M_DMA_RS_TGIU_5                                                                        \
	RZA2M_DMA_RS(0x087U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 5, compare match/input capture V */
#define RZA2M_DMA_RS_TGIV_5                                                                        \
	RZA2M_DMA_RS(0x08BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 5, compare match/input capture W */
#define RZA2M_DMA_RS_TGIW_5                                                                        \
	RZA2M_DMA_RS(0x08FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 6, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_6                                                                        \
	RZA2M_DMA_RS(0x093U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 6, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_6                                                                        \
	RZA2M_DMA_RS(0x097U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 6, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_6                                                                        \
	RZA2M_DMA_RS(0x09BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 6, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_6                                                                        \
	RZA2M_DMA_RS(0x09FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 7, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_7                                                                        \
	RZA2M_DMA_RS(0x0A3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 7, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_7                                                                        \
	RZA2M_DMA_RS(0x0A7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 7, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_7                                                                        \
	RZA2M_DMA_RS(0x0ABU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 7, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_7                                                                        \
	RZA2M_DMA_RS(0x0AFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 7, overflow/underflow interrupt of TCNT */
#define RZA2M_DMA_RS_TCIV_7                                                                        \
	RZA2M_DMA_RS(0x0B3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** MTU channel 8, compare match/input capture A */
#define RZA2M_DMA_RS_TGIA_8                                                                        \
	RZA2M_DMA_RS(0x0B7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 8, compare match/input capture B */
#define RZA2M_DMA_RS_TGIB_8                                                                        \
	RZA2M_DMA_RS(0x0BBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 8, compare match/input capture C */
#define RZA2M_DMA_RS_TGIC_8                                                                        \
	RZA2M_DMA_RS(0x0BFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** MTU channel 8, compare match/input capture D */
#define RZA2M_DMA_RS_TGID_8                                                                        \
	RZA2M_DMA_RS(0x0C3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** GPT channel 0, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_0                                                                       \
	RZA2M_DMA_RS(0x0C7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_0                                                                       \
	RZA2M_DMA_RS(0x0CBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, compare match C */
#define RZA2M_DMA_RS_CMPC_0                                                                        \
	RZA2M_DMA_RS(0x0CFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, compare match D */
#define RZA2M_DMA_RS_CMPD_0                                                                        \
	RZA2M_DMA_RS(0x0D3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, compare match E */
#define RZA2M_DMA_RS_CMPE_0                                                                        \
	RZA2M_DMA_RS(0x0E3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, compare match F */
#define RZA2M_DMA_RS_CMPF_0                                                                        \
	RZA2M_DMA_RS(0x0E7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_0                                                                      \
	RZA2M_DMA_RS(0x0EBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_0                                                                      \
	RZA2M_DMA_RS(0x0EFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, overflow */
#define RZA2M_DMA_RS_OVF_0                                                                         \
	RZA2M_DMA_RS(0x0F3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 0, underflow */
#define RZA2M_DMA_RS_UNF_0                                                                         \
	RZA2M_DMA_RS(0x0F7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** GPT channel 1, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_1                                                                       \
	RZA2M_DMA_RS(0x0FBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_1                                                                       \
	RZA2M_DMA_RS(0x0FFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, compare match C */
#define RZA2M_DMA_RS_CMPC_1                                                                        \
	RZA2M_DMA_RS(0x103U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, compare match D */
#define RZA2M_DMA_RS_CMPD_1                                                                        \
	RZA2M_DMA_RS(0x107U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, compare match E */
#define RZA2M_DMA_RS_CMPE_1                                                                        \
	RZA2M_DMA_RS(0x117U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, compare match F */
#define RZA2M_DMA_RS_CMPF_1                                                                        \
	RZA2M_DMA_RS(0x11BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_1                                                                      \
	RZA2M_DMA_RS(0x11FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_1                                                                      \
	RZA2M_DMA_RS(0x123U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, overflow */
#define RZA2M_DMA_RS_OVF_1                                                                         \
	RZA2M_DMA_RS(0x127U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 1, underflow */
#define RZA2M_DMA_RS_UNF_1                                                                         \
	RZA2M_DMA_RS(0x12BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** GPT channel 2, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_2                                                                       \
	RZA2M_DMA_RS(0x12FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_2                                                                       \
	RZA2M_DMA_RS(0x133U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, compare match C */
#define RZA2M_DMA_RS_CMPC_2                                                                        \
	RZA2M_DMA_RS(0x137U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, compare match D */
#define RZA2M_DMA_RS_CMPD_2                                                                        \
	RZA2M_DMA_RS(0x13BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, compare match E */
#define RZA2M_DMA_RS_CMPE_2                                                                        \
	RZA2M_DMA_RS(0x14BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, compare match F */
#define RZA2M_DMA_RS_CMPF_2                                                                        \
	RZA2M_DMA_RS(0x14FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_2                                                                      \
	RZA2M_DMA_RS(0x153U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_2                                                                      \
	RZA2M_DMA_RS(0x157U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, overflow */
#define RZA2M_DMA_RS_OVF_2                                                                         \
	RZA2M_DMA_RS(0x15BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 2, underflow */
#define RZA2M_DMA_RS_UNF_2                                                                         \
	RZA2M_DMA_RS(0x15FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** GPT channel 3, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_3                                                                       \
	RZA2M_DMA_RS(0x163U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_3                                                                       \
	RZA2M_DMA_RS(0x167U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, compare match C */
#define RZA2M_DMA_RS_CMPC_3                                                                        \
	RZA2M_DMA_RS(0x16BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, compare match D */
#define RZA2M_DMA_RS_CMPD_3                                                                        \
	RZA2M_DMA_RS(0x16FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, compare match E */
#define RZA2M_DMA_RS_CMPE_3                                                                        \
	RZA2M_DMA_RS(0x17FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, compare match F */
#define RZA2M_DMA_RS_CMPF_3                                                                        \
	RZA2M_DMA_RS(0x183U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_3                                                                      \
	RZA2M_DMA_RS(0x187U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_3                                                                      \
	RZA2M_DMA_RS(0x18BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, overflow */
#define RZA2M_DMA_RS_OVF_3                                                                         \
	RZA2M_DMA_RS(0x18FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 3, underflow */
#define RZA2M_DMA_RS_UNF_3                                                                         \
	RZA2M_DMA_RS(0x193U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** GPT channel 4, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_4                                                                       \
	RZA2M_DMA_RS(0x197U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_4                                                                       \
	RZA2M_DMA_RS(0x19BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, compare match C */
#define RZA2M_DMA_RS_CMPC_4                                                                        \
	RZA2M_DMA_RS(0x19FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, compare match D */
#define RZA2M_DMA_RS_CMPD_4                                                                        \
	RZA2M_DMA_RS(0x1A3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, compare match E */
#define RZA2M_DMA_RS_CMPE_4                                                                        \
	RZA2M_DMA_RS(0x1B3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, compare match F */
#define RZA2M_DMA_RS_CMPF_4                                                                        \
	RZA2M_DMA_RS(0x1B7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_4                                                                      \
	RZA2M_DMA_RS(0x1BBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_4                                                                      \
	RZA2M_DMA_RS(0x1BFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, overflow */
#define RZA2M_DMA_RS_OVF_4                                                                         \
	RZA2M_DMA_RS(0x1C3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 4, underflow */
#define RZA2M_DMA_RS_UNF_4                                                                         \
	RZA2M_DMA_RS(0x1C7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** GPT channel 5, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_5                                                                       \
	RZA2M_DMA_RS(0x1CBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_5                                                                       \
	RZA2M_DMA_RS(0x1CFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, compare match C */
#define RZA2M_DMA_RS_CMPC_5                                                                        \
	RZA2M_DMA_RS(0x1D3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, compare match D */
#define RZA2M_DMA_RS_CMPD_5                                                                        \
	RZA2M_DMA_RS(0x1D7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, compare match E */
#define RZA2M_DMA_RS_CMPE_5                                                                        \
	RZA2M_DMA_RS(0x1E7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, compare match F */
#define RZA2M_DMA_RS_CMPF_5                                                                        \
	RZA2M_DMA_RS(0x1EBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_5                                                                      \
	RZA2M_DMA_RS(0x1EFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_5                                                                      \
	RZA2M_DMA_RS(0x1F3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, overflow */
#define RZA2M_DMA_RS_OVF_5                                                                         \
	RZA2M_DMA_RS(0x1F7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 5, underflow */
#define RZA2M_DMA_RS_UNF_5                                                                         \
	RZA2M_DMA_RS(0x1FBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** GPT channel 6, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_6                                                                       \
	RZA2M_DMA_RS(0x1FFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_6                                                                       \
	RZA2M_DMA_RS(0x203U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, compare match C */
#define RZA2M_DMA_RS_CMPC_6                                                                        \
	RZA2M_DMA_RS(0x207U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, compare match D */
#define RZA2M_DMA_RS_CMPD_6                                                                        \
	RZA2M_DMA_RS(0x20BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, compare match E */
#define RZA2M_DMA_RS_CMPE_6                                                                        \
	RZA2M_DMA_RS(0x21BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, compare match F */
#define RZA2M_DMA_RS_CMPF_6                                                                        \
	RZA2M_DMA_RS(0x21FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_6                                                                      \
	RZA2M_DMA_RS(0x223U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_6                                                                      \
	RZA2M_DMA_RS(0x227U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, overflow */
#define RZA2M_DMA_RS_OVF_6                                                                         \
	RZA2M_DMA_RS(0x22BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 6, underflow */
#define RZA2M_DMA_RS_UNF_6                                                                         \
	RZA2M_DMA_RS(0x22FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** GPT channel 7, compare match/input capture A */
#define RZA2M_DMA_RS_CCMPA_7                                                                       \
	RZA2M_DMA_RS(0x233U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, compare match/input capture B */
#define RZA2M_DMA_RS_CCMPB_7                                                                       \
	RZA2M_DMA_RS(0x237U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, compare match C */
#define RZA2M_DMA_RS_CMPC_7                                                                        \
	RZA2M_DMA_RS(0x23BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, compare match D */
#define RZA2M_DMA_RS_CMPD_7                                                                        \
	RZA2M_DMA_RS(0x23FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, compare match E */
#define RZA2M_DMA_RS_CMPE_7                                                                        \
	RZA2M_DMA_RS(0x24FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, compare match F */
#define RZA2M_DMA_RS_CMPF_7                                                                        \
	RZA2M_DMA_RS(0x253U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, A/D conversion start A */
#define RZA2M_DMA_RS_ADTRGA_7                                                                      \
	RZA2M_DMA_RS(0x257U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, A/D conversion start B */
#define RZA2M_DMA_RS_ADTRGB_7                                                                      \
	RZA2M_DMA_RS(0x25BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, overflow */
#define RZA2M_DMA_RS_OVF_7                                                                         \
	RZA2M_DMA_RS(0x25FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** GPT channel 7, underflow */
#define RZA2M_DMA_RS_UNF_7                                                                         \
	RZA2M_DMA_RS(0x263U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** ADC scan end interrupt */
#define RZA2M_DMA_RS_S12ADI_0                                                                      \
	RZA2M_DMA_RS(0x267U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_SRC)
/** ADC group B scan end interrupt */
#define RZA2M_DMA_RS_S12GBADI_0                                                                    \
	RZA2M_DMA_RS(0x26BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_SRC)
/** ADC group C scan end interrupt */
#define RZA2M_DMA_RS_S12GCADI_0                                                                    \
	RZA2M_DMA_RS(0x26FU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_SRC)

/** SSIF channel 0, DMA receive */
#define RZA2M_DMA_RS_INT_SSIF_DMA_RX_0                                                             \
	RZA2M_DMA_RS(0x272U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** SSIF channel 0, DMA transmit */
#define RZA2M_DMA_RS_INT_SSIF_DMA_TX_0                                                             \
	RZA2M_DMA_RS(0x271U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** SSIF channel 1, DMA receive */
#define RZA2M_DMA_RS_INT_SSIF_DMA_RX_1                                                             \
	RZA2M_DMA_RS(0x276U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** SSIF channel 1, DMA transmit */
#define RZA2M_DMA_RS_INT_SSIF_DMA_TX_1                                                             \
	RZA2M_DMA_RS(0x275U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** SSIF channel 2, DMA receive */
#define RZA2M_DMA_RS_INT_SSIF_DMA_RX_2                                                             \
	RZA2M_DMA_RS(0x27BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** SSIF channel 2, DMA transmit */
#define RZA2M_DMA_RS_INT_SSIF_DMA_TX_2                                                             \
	RZA2M_DMA_RS(0x27BU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** SSIF channel 3, DMA receive */
#define RZA2M_DMA_RS_INT_SSIF_DMA_RX_3                                                             \
	RZA2M_DMA_RS(0x27EU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** SSIF channel 3, DMA transmit */
#define RZA2M_DMA_RS_INT_SSIF_DMA_TX_3                                                             \
	RZA2M_DMA_RS(0x27DU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)

/** SPDIF transmit interrupt */
#define RZA2M_DMA_RS_SPDIFTXI                                                                      \
	RZA2M_DMA_RS(0x283U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** SPDIF receive interrupt */
#define RZA2M_DMA_RS_SPDIFRXI                                                                      \
	RZA2M_DMA_RS(0x287U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)

/** RIIC channel 0, receive interrupt */
#define RZA2M_DMA_RS_INTRIIC_RI0                                                                   \
	RZA2M_DMA_RS(0x28AU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** RIIC channel 0, transmit interrupt */
#define RZA2M_DMA_RS_INTRIIC_TI0                                                                   \
	RZA2M_DMA_RS(0x289U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** RIIC channel 1, receive interrupt */
#define RZA2M_DMA_RS_INTRIIC_RI1                                                                   \
	RZA2M_DMA_RS(0x28EU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** RIIC channel 1, transmit interrupt */
#define RZA2M_DMA_RS_INTRIIC_TI1                                                                   \
	RZA2M_DMA_RS(0x28DU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** RIIC channel 2, receive interrupt */
#define RZA2M_DMA_RS_INTRIIC_RI2                                                                   \
	RZA2M_DMA_RS(0x292U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** RIIC channel 2, transmit interrupt */
#define RZA2M_DMA_RS_INTRIIC_TI2                                                                   \
	RZA2M_DMA_RS(0x291U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** RIIC channel 3, receive interrupt */
#define RZA2M_DMA_RS_INTRIIC_RI3                                                                   \
	RZA2M_DMA_RS(0x296U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** RIIC channel 3, transmit interrupt */
#define RZA2M_DMA_RS_INTRIIC_TI3                                                                   \
	RZA2M_DMA_RS(0x295U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)

/** SCIF channel 0, receive interrupt */
#define RZA2M_DMA_RS_RXI0                                                                          \
	RZA2M_DMA_RS(0x29AU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_SRC)
/** SCIF channel 0, transmit interrupt */
#define RZA2M_DMA_RS_TXI0                                                                          \
	RZA2M_DMA_RS(0x299U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_DST)
/** SCIF channel 1, receive interrupt */
#define RZA2M_DMA_RS_RXI1                                                                          \
	RZA2M_DMA_RS(0x29EU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_SRC)
/** SCIF channel 1, transmit interrupt */
#define RZA2M_DMA_RS_TXI1                                                                          \
	RZA2M_DMA_RS(0x29DU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_DST)
/** SCIF channel 2, receive interrupt */
#define RZA2M_DMA_RS_RXI2                                                                          \
	RZA2M_DMA_RS(0x2A2U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_SRC)
/** SCIF channel 2, transmit interrupt */
#define RZA2M_DMA_RS_TXI2                                                                          \
	RZA2M_DMA_RS(0x2A1U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_DST)
/** SCIF channel 3, receive interrupt */
#define RZA2M_DMA_RS_RXI3                                                                          \
	RZA2M_DMA_RS(0x2A6U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_SRC)
/** SCIF channel 3, transmit interrupt */
#define RZA2M_DMA_RS_TXI3                                                                          \
	RZA2M_DMA_RS(0x2A5U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_DST)
/** SCIF channel 4, receive interrupt */
#define RZA2M_DMA_RS_RXI4                                                                          \
	RZA2M_DMA_RS(0x2AAU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_SRC)
/** SCIF channel 4, transmit interrupt */
#define RZA2M_DMA_RS_TXI4                                                                          \
	RZA2M_DMA_RS(0x2A9U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_NO_ACK, RZA2M_DMA_F_REQD_DST)

/** CAN, receive FIFO DMA channel 0 */
#define RZA2M_DMA_RS_RXF_DMA0                                                                      \
	RZA2M_DMA_RS(0x2AFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, receive FIFO DMA channel 1 */
#define RZA2M_DMA_RS_RXF_DMA1                                                                      \
	RZA2M_DMA_RS(0x2B3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, receive FIFO DMA channel 2 */
#define RZA2M_DMA_RS_RXF_DMA2                                                                      \
	RZA2M_DMA_RS(0x2B7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, receive FIFO DMA channel 3 */
#define RZA2M_DMA_RS_RXF_DMA3                                                                      \
	RZA2M_DMA_RS(0x2BBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, receive FIFO DMA channel 4 */
#define RZA2M_DMA_RS_RXF_DMA4                                                                      \
	RZA2M_DMA_RS(0x2BFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, receive FIFO DMA channel 5 */
#define RZA2M_DMA_RS_RXF_DMA5                                                                      \
	RZA2M_DMA_RS(0x2C3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, receive FIFO DMA channel 6 */
#define RZA2M_DMA_RS_RXF_DMA6                                                                      \
	RZA2M_DMA_RS(0x2C7U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, receive FIFO DMA channel 7 */
#define RZA2M_DMA_RS_RXF_DMA7                                                                      \
	RZA2M_DMA_RS(0x2CBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, common FIFO DMA channel 0 */
#define RZA2M_DMA_RS_COM_DMA0                                                                      \
	RZA2M_DMA_RS(0x2CFU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)
/** CAN, common FIFO DMA channel 1 */
#define RZA2M_DMA_RS_COM_DMA1                                                                      \
	RZA2M_DMA_RS(0x2D3U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** RSPI channel 0, receive interrupt */
#define RZA2M_DMA_RS_SPRI0                                                                         \
	RZA2M_DMA_RS(0x2D6U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** RSPI channel 0, transmit interrupt */
#define RZA2M_DMA_RS_SPTI0                                                                         \
	RZA2M_DMA_RS(0x2D5U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** RSPI channel 1, receive interrupt */
#define RZA2M_DMA_RS_SPRI1                                                                         \
	RZA2M_DMA_RS(0x2DAU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** RSPI channel 1, transmit interrupt */
#define RZA2M_DMA_RS_SPTI1                                                                         \
	RZA2M_DMA_RS(0x2D9U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** RSPI channel 2, receive interrupt */
#define RZA2M_DMA_RS_SPRI2                                                                         \
	RZA2M_DMA_RS(0x2DEU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** RSPI channel 2, transmit interrupt */
#define RZA2M_DMA_RS_SPTI2                                                                         \
	RZA2M_DMA_RS(0x2DDU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)

/** SCI channel 0, receive interrupt */
#define RZA2M_DMA_RS_SCI_RXI0                                                                      \
	RZA2M_DMA_RS(0x2E2U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** SCI channel 0, transmit interrupt */
#define RZA2M_DMA_RS_SCI_TXI0                                                                      \
	RZA2M_DMA_RS(0x2E1U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** SCI channel 1, receive interrupt */
#define RZA2M_DMA_RS_SCI_RXI1                                                                      \
	RZA2M_DMA_RS(0x2E6U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** SCI channel 1, transmit interrupt */
#define RZA2M_DMA_RS_SCI_TXI1                                                                      \
	RZA2M_DMA_RS(0x2E5U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)

/** Ethernet MAC controller IPLS */
#define RZA2M_DMA_RS_IPLS                                                                          \
	RZA2M_DMA_RS(0x2EBU, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_LEVEL, RZA2M_DMA_F_REQD_AUTO)

/** DRP Tile 0 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_0_PAFI                                                                   \
	RZA2M_DMA_RS(0x3CEU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** DRP Tile 0 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_0_PAEI                                                                   \
	RZA2M_DMA_RS(0x3CDU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** DRP Tile 1 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_1_PAFI                                                                   \
	RZA2M_DMA_RS(0x3D2U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** DRP Tile 1 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_1_PAEI                                                                   \
	RZA2M_DMA_RS(0x3D1U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** DRP Tile 2 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_2_PAFI                                                                   \
	RZA2M_DMA_RS(0x3D6U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** DRP Tile 2 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_2_PAEI                                                                   \
	RZA2M_DMA_RS(0x3D5U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** DRP Tile 3 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_3_PAFI                                                                   \
	RZA2M_DMA_RS(0x3DAU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** DRP Tile 3 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_3_PAEI                                                                   \
	RZA2M_DMA_RS(0x3D9U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** DRP Tile 4 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_4_PAFI                                                                   \
	RZA2M_DMA_RS(0x3DEU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** DRP Tile 4 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_4_PAEI                                                                   \
	RZA2M_DMA_RS(0x3DDU, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)
/** DRP Tile 5 Programmable Almost Full */
#define RZA2M_DMA_RS_TILE_5_PAFI                                                                   \
	RZA2M_DMA_RS(0x3E2U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_SRC)
/** DRP Tile 5 Programmable Almost Empty */
#define RZA2M_DMA_RS_TILE_5_PAEI                                                                   \
	RZA2M_DMA_RS(0x3E1U, RZA2M_DMA_F_LVL_LEVEL, RZA2M_DMA_F_AM_BUS, RZA2M_DMA_F_REQD_DST)

/** External DMA request */
#define RZA2M_DMA_RS_DREQ0                                                                         \
	RZA2M_DMA_RS(0x003U, RZA2M_DMA_F_LVL_EDGE, RZA2M_DMA_F_AM_NONE, RZA2M_DMA_F_REQD_AUTO)

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_DMA_RENESAS_RZA2M_H_ */
