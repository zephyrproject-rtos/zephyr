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
 * @brief TI LM3S6965 System Control Peripherals interface
 *
 * This module defines the System Control Peripheral Registers for TI LM3S6965
 * processor. The registers defined are in region 0x400fe000.
 *
 *   System Control	0x400fe000
 *
 * These modules are not defined:
 *
 *   Hibernation Module	0x400fc000
 *   Internal Memory	0x400fd000
 *   Hibernation Module	0x400fc000
 *
 * The registers and bit field names are taken from the 'Stellaris LM3S6965
 * Microcontroller DATA SHEET (DS-LM3S6965-12746.2515) revision H' document,
 * section 5.4/5.5, pp .184-200.
 */

#ifndef _SCP_H_
#define _SCP_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define _SCP_OSC_SOURCE_MAIN 0
#define _SCP_OSC_SOURCE_INTERNAL 1
#define _SCP_OSC_SOURCE_INTERNAL_DIV4 2
#define _SCP_OSC_SOURCE_INTERNAL_20KHZ 3
#define _SCP_OSC_SOURCE_EXTERNAL_32KHZ 7 /* Valid with RCC2 only */

#define _SCP_CRYSTAL_1MHZ_NOPLL 0
#define _SCP_CRYSTAL_1_8432MHZ_NOPLL 1
#define _SCP_CRYSTAL_2MHZ_NOPLL 2
#define _SCP_CRYSTAL_2_4576MHZ_NOPLL 3
#define _SCP_CRYSTAL_3_579545MHZ 4
#define _SCP_CRYSTAL_3_6864MHZ 5
#define _SCP_CRYSTAL_4MHZ 6
#define _SCP_CRYSTAL_4_0964MHZ 7
#define _SCP_CRYSTAL_4_9152MHZ 8
#define _SCP_CRYSTAL_5MHZ 9
#define _SCP_CRYSTAL_5_12MHZ 10
#define _SCP_CRYSTAL_6MHZ 11 /* reset value */
#define _SCP_CRYSTAL_6_144MHZ 12
#define _SCP_CRYSTAL_7_3728MHZ 13
#define _SCP_CRYSTAL_8MHZ 14
#define _SCP_CRYSTAL_8_192MHZ 15

union __rcc {
	uint32_t value;
	struct {
		uint32_t moscdis : 1 __packed;
		uint32_t ioscdis : 1 __packed;
		uint32_t rsvd__2_3 : 2 __packed;
		uint32_t oscsrc : 2 __packed;
		uint32_t xtal : 4 __packed;
		uint32_t rsvd__10 : 1 __packed;
		uint32_t bypass : 1 __packed;
		uint32_t rsvd__12 : 1 __packed;
		uint32_t pwrdn : 1 __packed;
		uint32_t rsvd__14_16 : 3 __packed;
		uint32_t pwmdiv : 3 __packed; /* 2**(n+1) */
		uint32_t usepwmdiv : 1 __packed;
		uint32_t rsvd__21 : 1 __packed;
		uint32_t usesysdiv : 1 __packed;
		uint32_t sysdiv : 4 __packed;
		uint32_t acg : 1 __packed;
		uint32_t rsvd__28_31 : 4 __packed;
	} bit;
};

union __rcc2 {
	uint32_t value;
	struct {
		uint8_t rsvd__0_3 : 4 __packed;
		uint8_t oscsrc2 : 3 __packed;
		uint16_t rsvd__7_10 : 4 __packed;
		uint8_t bypass2 : 1 __packed;
		uint8_t rsvd__12 : 1 __packed;
		uint8_t pwrdn2 : 1 __packed;
		uint16_t rsvd__14_22 : 9 __packed;
		uint16_t sysdiv2 : 6 __packed;
		uint8_t rsvd__29_30 : 2 __packed;
		uint8_t usercc2 : 1 __packed;
	} bit;
};

struct __scp {
	uint32_t did0; /* 0x000 RO Device ID*/
	uint32_t did1; /* 0x004 RO Device ID*/
	uint32_t dc0;  /* 0x008 RO Device Capabilities */
	uint32_t dc1;  /* 0x00c RO Device Capabilities */
	uint32_t dc2;  /* 0x010 RO Device Capabilities */
	uint32_t dc3;  /* 0x014 RO Device Capabilities */
	uint32_t dc4;  /* 0x018 RO Device capabilities */

	uint32_t rsvd__01c_02f[(0x30 - 0x1c) / 4];

	uint32_t pborctl; /* 0x030 RW Brown-Out Reset ConTroL */
	uint32_t ldopctl; /* 0x034 RW LDO Power ConTroL */

	uint32_t rsvd__038_03f[(0x40 - 0x38) / 4];

	uint32_t srcr0; /* 0x040 RW Software Reset Control Register */
	uint32_t srcr1; /* 0x044 RW Software Reset Control Register */
	uint32_t srcr2; /* 0x048 RW Software Reset Control Register */

	uint32_t rsvd__04c_04f;

	uint32_t ris;  /* 0x050 RO Raw Interrupt Status */
	uint32_t imc;  /* 0x054 RW Interrupt Mask Control */
	uint32_t misc; /* 0x058 RW1C Masked Int. Status & Clear */
	uint32_t resc; /* 0x05C RW RESet Cause */
	struct {
		union __rcc rcc; /* 0x060 RW Run-mode Clock Configuration */
		uint32_t pllcfg; /* 0x064 RW xtal-to-pll translation */

		uint32_t rsvd__068_06f[(0x70 - 0x068) / 4];

		union __rcc2 rcc2; /* 0x070 RW Run-mode Clock Configuration */

		uint32_t rsvd__074_0ff[(0x100 - 0x074) / 4];

		uint32_t rcgc0; /* 0x100 RW Run-mode Clock Gating */
		uint32_t rcgc1; /* 0x104 RW Run-mode Clock Gating */
		uint32_t rcgc2; /* 0x108 RW Run-mode Clock Gating */

		uint32_t rsvd__10c_10f;

		uint32_t scgc0; /* 0x110 RW Sleep-mode Clock Gating */
		uint32_t scgc1; /* 0x114 RW Sleep-mode Clock Gating */
		uint32_t scgc2; /* 0x118 RW Sleep-mode Clock Gating */

		uint32_t rsvd__11c_11f;

		uint32_t dcgc0; /* 0x120 RW Deep sleep mode Clock Gating */
		uint32_t dcgc1; /* 0x124 RW Deep sleep mode Clock Gating */
		uint32_t dcgc2; /* 0x128 RW Deep sleep mode Clock Gating */

		uint32_t rsvd__12c_143[(0x144 - 0x12c) / 4];

		uint32_t
			dslpclkcfg; /* 0x144 RW Deep SLeeP CLocK ConFiGuration
				       */
	} clock;
};

extern volatile struct __scp __scp;

#ifdef __cplusplus
}
#endif

#endif /* _SCP_H_ */
