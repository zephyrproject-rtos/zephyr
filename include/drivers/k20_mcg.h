/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief K20 Microprocessor MCG register definitions
 *
 * This module defines the Multipurpose Clock Generator (MCG) and Oscillator
 * (OSC) registers for the K20 Family of microprocessors.
 */
#ifndef _K20MCG_H_
#define _K20MCG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* MCG - module register structure */
typedef volatile struct
{
	uint8_t c1; /* 0x0 */
	uint8_t c2; /* 0x1 */
	uint8_t c3; /* 0x2 */
	uint8_t c4; /* 0x3 */
	uint8_t c5; /* 0x4 */
	uint8_t c6; /* 0x5 */
	uint8_t s;  /* 0x6 */
	uint8_t res_7;
	uint8_t sc; /* 0x8 */
	uint8_t res_9;
	uint8_t atcvh; /* 0xA */
	uint8_t atcvl; /* 0xB */
	uint8_t c7;    /* 0xC */
	uint8_t c8;    /* 0xD */
} K20_MCG_t;

/* Control 1 register fields */
#define MCG_C1_IREFSTEN_MASK 0x1
#define MCG_C1_IREFSTEN_SHIFT 0
#define MCG_C1_IRCLKEN_MASK 0x2
#define MCG_C1_IRCLKEN_SHIFT 1
#define MCG_C1_IREFS_MASK 0x4
#define MCG_C1_IREFS_SHIFT 2
#define MCG_C1_IREFS_EXT ((uint8_t)(0 << MCG_C1_IREFS_SHIFT))
#define MCG_C1_IREFS_INT ((uint8_t)(1 << MCG_C1_IREFS_SHIFT))
#define MCG_C1_FRDIV_MASK 0x38
#define MCG_C1_FRDIV_SHIFT 3
#define MCG_C1_FRDIV_1_32 ((uint8_t)(0 << MCG_C1_FRDIV_SHIFT))
#define MCG_C1_FRDIV_2_64 ((uint8_t)(1 << MCG_C1_FRDIV_SHIFT))
#define MCG_C1_FRDIV_4_128 ((uint8_t)(2 << MCG_C1_FRDIV_SHIFT))
#define MCG_C1_FRDIV_8_256 ((uint8_t)(3 << MCG_C1_FRDIV_SHIFT))
#define MCG_C1_FRDIV_16_512 ((uint8_t)(4 << MCG_C1_FRDIV_SHIFT))
#define MCG_C1_FRDIV_32_1024 ((uint8_t)(5 << MCG_C1_FRDIV_SHIFT))
#define MCG_C1_FRDIV_64_1280 ((uint8_t)(6 << MCG_C1_FRDIV_SHIFT))
#define MCG_C1_FRDIV_128_1536 ((uint8_t)(7 << MCG_C1_FRDIV_SHIFT))
#define MCG_C1_CLKS_MASK 0xC0
#define MCG_C1_CLKS_SHIFT 6
#define MCG_C1_CLKS_FLL_PLL ((uint8_t)(0 << MCG_C1_CLKS_SHIFT))
#define MCG_C1_CLKS_INT_REF ((uint8_t)(1 << MCG_C1_CLKS_SHIFT))
#define MCG_C1_CLKS_EXT_REF ((uint8_t)(2 << MCG_C1_CLKS_SHIFT))

/* Control 2 register fields */
#define MCG_C2_IRCS_MASK 0x1
#define MCG_C2_IRCS_SHIFT 0
#define MCG_C2_LP_MASK 0x2
#define MCG_C2_LP_SHIFT 1
#define MCG_C2_EREFS_MASK 0x4
#define MCG_C2_EREFS_SHIFT 2
#define MCG_C2_EREFS_EXT_CLK ((uint8_t)(0 << MCG_C2_EREFS_SHIFT))
#define MCG_C2_EREFS_OSC ((uint8_t)(1 << MCG_C2_EREFS_SHIFT))
#define MCG_C2_HGO_MASK 0x8
#define MCG_C2_HGO_SHIFT 3
#define MCG_C2_HGO_LO_PWR ((uint8_t)(0 << MCG_C2_HGO_SHIFT))
#define MCG_C2_HGO_HI_GAIN ((uint8_t)(1 << MCG_C2_HGO_SHIFT))
#define MCG_C2_RANGE_MASK 0x30
#define MCG_C2_RANGE_SHIFT 4
#define MCG_C2_RANGE_LOW ((uint8_t)(0 << MCG_C2_RANGE_SHIFT))
#define MCG_C2_RANGE_HIGH ((uint8_t)(1 << MCG_C2_RANGE_SHIFT))
#define MCG_C2_RANGE_VHIGH ((uint8_t)(2 << MCG_C2_RANGE_SHIFT))
#define MCG_C2_LOCRE0_MASK 0x80
#define MCG_C2_LOCRE0_SHIFT 7

/* Control 3 register fields */
#define MCG_C3_SCTRIM_MASK 0xFF
#define MCG_C3_SCTRIM_SHIFT 0

/* Control 4 register fields */
#define MCG_C4_SCFTRIM_MASK 0x1
#define MCG_C4_SCFTRIM_SHIFT 0
#define MCG_C4_FCTRIM_MASK 0x1E
#define MCG_C4_FCTRIM_SHIFT 1
#define MCG_C4_DRST_DRS_MASK 0x60
#define MCG_C4_DRST_DRS_SHIFT 5
#define MCG_C4_DMX32_MASK 0x80
#define MCG_C4_DMX32_SHIFT 7

/* Control 5 register fields */
#define MCG_C5_PRDIV0_MASK 0x1F
#define MCG_C5_PRDIV0_SHIFT 0
#define MCG_C5_PLLSTEN0_MASK 0x20
#define MCG_C5_PLLSTEN0_SHIFT 5
#define MCG_C5_PLLCLKEN0_MASK 0x40
#define MCG_C5_PLLCLKEN0_SHIFT 6

/* Control 6 register fields */
#define MCG_C6_VDIV0_MASK 0x1F
#define MCG_C6_VDIV0_SHIFT 0
#define MCG_C6_CME0_MASK 0x20
#define MCG_C6_CME0_SHIFT 5
#define MCG_C6_PLLS_MASK 0x40
#define MCG_C6_PLLS_SHIFT 6
#define MCG_C6_PLLS_FLL ((uint8_t)(0 << MCG_C6_PLLS_SHIFT))
#define MCG_C6_PLLS_PLL ((uint8_t)(1 << MCG_C6_PLLS_SHIFT))
#define MCG_C6_LOLIE0_MASK 0x80
#define MCG_C6_LOLIE0_SHIFT 7

/* Status register fields */
#define MCG_S_IRCST_MASK 0x1
#define MCG_S_IRCST_SHIFT 0
#define MCG_S_OSCINIT0_MASK 0x2
#define MCG_S_OSCINIT0_SHIFT 1
#define MCG_S_CLKST_MASK 0xC
#define MCG_S_CLKST_SHIFT 2
#define MCG_S_CLKST_FLL ((uint8_t)(0 << MCG_S_CLKST_SHIFT))
#define MCG_S_CLKST_INT_REF ((uint8_t)(1 << MCG_S_CLKST_SHIFT))
#define MCG_S_CLKST_EXT_REF ((uint8_t)(2 << MCG_S_CLKST_SHIFT))
#define MCG_S_CLKST_PLL ((uint8_t)(3 << MCG_S_CLKST_SHIFT))
#define MCG_S_IREFST_MASK 0x10
#define MCG_S_IREFST_SHIFT 4
#define MCG_S_PLLST_MASK 0x20
#define MCG_S_PLLST_SHIFT 5
#define MCG_S_LOCK0_MASK 0x40
#define MCG_S_LOCK0_SHIFT 6
#define MCG_S_LOLS0_MASK 0x80
#define MCG_S_LOLS0_SHIFT 7

/* Status and Control register fields */
#define MCG_SC_LOCS0_MASK 0x1
#define MCG_SC_LOCS0_SHIFT 0
#define MCG_SC_FCRDIV_MASK 0xE
#define MCG_SC_FCRDIV_SHIFT 1
#define MCG_SC_FLTPRSRV_MASK 0x10
#define MCG_SC_FLTPRSRV_SHIFT 4
#define MCG_SC_ATMF_MASK 0x20
#define MCG_SC_ATMF_SHIFT 5
#define MCG_SC_ATMS_MASK 0x40
#define MCG_SC_ATMS_SHIFT 6
#define MCG_SC_ATME_MASK 0x80
#define MCG_SC_ATME_SHIFT 7

/* Auto Trim Compare Value High register fields */
#define MCG_ATCVH_ATCVH_MASK 0xFF
#define MCG_ATCVH_ATCVH_SHIFT 0

/* Auto Trim Compare Value Low register fields */
#define MCG_ATCVL_ATCVL_MASK 0xFF
#define MCG_ATCVL_ATCVL_SHIFT 0

/* Control 7 register fields */
#define MCG_C7_OSCSEL_MASK 0x03
#define MCG_C7_OSCSEL_SHIFT 0
#define MCG_C7_OSCSEL_OSC0 ((uint8_t)0x0)
#define MCG_C7_OSCSEL_32KHZ_RTC ((uint8_t)0x1)
#define MCG_C7_OSCSEL_OSC1 ((uint8_t)0x2)

/* Control 8 register fields */
#define MCG_C8_LOCS1_MASK 0x1
#define MCG_C8_LOCS1_SHIFT 0
#define MCG_C8_CME1_MASK 0x20
#define MCG_C8_CME1_SHIFT 5
#define MCG_C8_LOLRE_MASK 0x40
#define MCG_C8_LOLRE_SHIFT 6

/* OSC - module register structure */
typedef volatile struct {
	uint8_t cr; /* 0x0 */
} K20_OSC_t;

typedef union {
	uint32_t value; /* reset 0x00 */
	struct {
		uint8_t sc16p : 1 __packed;
		uint8_t sc8p : 1 __packed;
		uint8_t sc4p : 1 __packed;
		uint8_t sc2p : 1 __packed;
		uint8_t res_4 : 1 __packed;
		uint8_t erefsten : 1 __packed;
		uint8_t res_6 : 1 __packed;
		uint8_t erclken : 1 __packed;
	} field;
} OSC_CR_t; /* 0x0 */

/* Control register fields */
#define OSC_CR_EXT_CLK_EN 0x80

#ifdef __cplusplus
}
#endif

#endif /* _K20MCG_H_ */
