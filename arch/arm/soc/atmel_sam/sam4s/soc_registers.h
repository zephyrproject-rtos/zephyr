/*
 * Copyright (c) 2017 Justin Watson
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Atmel SAM4S family processors.
 *
 * Refer to the datasheet for more information about these registers.
 */

#ifndef _ATMEL_SAM4S_SOC_REGS_H_
#define _ATMEL_SAM4S_SOC_REGS_H_

/* Enhanced Embedded Flash Controller */
struct __eefc {
	u32_t	fmr;	/* 0x00 Flash Mode Register    */
	u32_t	fcr;	/* 0x04 Flash Command Register */
	u32_t	fsr;	/* 0x08 Flash Status Register  */
	u32_t	frr;	/* 0x0C Flash Result Register  */
};

/* PIO Controller */
struct __pio {
	u32_t	per;	/* 0x00 Enable                      */
	u32_t	pdr;	/* 0x04 Disable                     */
	u32_t	psr;	/* 0x08 Status                      */

	u32_t	res0;	/* 0x0C reserved                    */

	u32_t	oer;	/* 0x10 Output Enable               */
	u32_t	odr;	/* 0x14 Output Disable              */
	u32_t	osr;	/* 0x18 Output Status               */

	u32_t	res1;	/* 0x1C reserved                    */

	u32_t	ifer;	/* 0x20 Glitch Input Filter Enable  */
	u32_t	ifdr;	/* 0x24 Glitch Input Filter Disable */
	u32_t	ifsr;	/* 0x28 Glitch Input Fitler Status  */

	u32_t	res2;	/* 0x2C reserved                    */

	u32_t	sodr;	/* 0x30 Set Output Data             */
	u32_t	codr;	/* 0x34 Clear Output Data           */
	u32_t	odsr;	/* 0x38 Output Data Status          */
	u32_t	pdsr;	/* 0x3C Pin Data Status             */

	u32_t	ier;	/* 0x40 Interrupt Enable            */
	u32_t	idr;	/* 0x44 Interrupt Disable           */
	u32_t	imr;	/* 0x48 Interrupt Mask              */
	u32_t	isr;	/* 0x4C Interrupt Status            */

	u32_t	mder;	/* 0x50 Multi-driver Enable         */
	u32_t	mddr;	/* 0x54 Multi-driver Disable        */
	u32_t	mdsr;	/* 0x58 Multi-driver Status         */

	u32_t	res3;	/* 0x5C reserved                    */

	u32_t	pudr;	/* 0x60 Pull-up Disable             */
	u32_t	puer;	/* 0x64 Pull-up Enable              */
	u32_t	pusr;	/* 0x68 Pad Pull-up Status          */

	u32_t	res4;	/* 0x6C reserved                    */

	u32_t	abcdsr1; /* 0x70 Peripheral ABCD Select 1   */
	u32_t    abcdsr2; /* 0x74 Peripheral ABCD Select 2   */

	u32_t	res5[2];	/* 0x78-0x7C reserved       */

	u32_t	scifsr;	/* 0x80 System Clock Glitch Input   */
				/*        Filter Select             */

	u32_t	difsr;	/* 0x84 Debouncing Input Filter     */
				/*        Select                    */

	u32_t	ifdgsr;	/* 0x88 Glitch or Debouncing Input  */
				/*        Filter Clock Selection    */
				/*        Status                    */

	u32_t	scdr;	/* 0x8C Slow Clock Divider Debounce */

	u32_t	res6[4];	/* 0x90-0x9C reserved       */

	u32_t	ower;	/* 0xA0 Output Write Enable         */
	u32_t	owdr;	/* 0xA4 Output Write Disable        */
	u32_t	owsr;	/* 0xA8 Output Write Status         */

	u32_t	res7;	/* 0xAC reserved                    */

	u32_t	aimer;	/* 0xB0 Additional Interrupt Modes  */
				/*        Enable                    */
	u32_t	aimdr;	/* 0xB4 Additional Interrupt Modes  */
				/*        Disable                   */
	u32_t	aimmr;	/* 0xB8 Additional Interrupt Modes  */
				/*        Mask                      */

	u32_t	res8;	/* 0xBC reserved                    */

	u32_t	esr;	/* 0xC0 Edge Select                 */
	u32_t	lsr;	/* 0xC4 Level Select                */
	u32_t	elsr;	/* 0xC8 Edge/Level Status           */

	u32_t	res9;	/* 0xCC reserved                    */

	u32_t	fellsr;	/* 0xD0 Falling Edge/Low Level Sel  */
	u32_t	rehlsr;	/* 0xD4 Rising Edge/High Level Sel  */
	u32_t	frlhsr;	/* 0xD8 Fall/Rise - Low/High Status */

	u32_t	res10;	/* 0xDC reserved                    */

	u32_t	locksr;	/* 0xE0 Lock Status                 */

	u32_t	wpmr;	/* 0xE4 Write Protect Mode          */
	u32_t	wpsr;	/* 0xE8 Write Protect Status        */
};

/* Power Management Controller */
struct __pmc {
	u32_t	scer;	/* 0x00 System Clock Enable         */
	u32_t	scdr;	/* 0x04 System Clock Disable        */
	u32_t	scsr;	/* 0x08 System Clock Status         */

	u32_t	res0;	/* 0x0C reserved                    */

	u32_t	pcer0;	/* 0x10 Peripheral Clock Enable 0   */
	u32_t	pcdr0;	/* 0x14 Peripheral Clock Disable 0  */
	u32_t	pcsr0;	/* 0x18 Peripheral Clock Status 0   */

	u32_t	ckgr_uckr;	/* 0x1C UTMI Clock          */
	u32_t	ckgr_mor;	/* 0x20 Main Oscillator     */
	u32_t	ckgr_mcfr;	/* 0x24 Main Clock Freq.    */
	u32_t	ckgr_pllar;	/* 0x28 PLLA                */
	u32_t	ckgr_pllbr; /* 0x2C PLLB                */

	u32_t	mckr;	/* 0x30 Master Clock                */

	u32_t	res2;	/* 0x34 reserved                    */

	u32_t	usb;	/* 0x38 USB Clock                   */

	u32_t	res3;	/* 0x3C reserved                    */

	u32_t        pck0;	/* 0x40 Programmable Clock 0        */
	u32_t        pck1;	/* 0x44 Programmable Clock 1        */
	u32_t        pck2;	/* 0x48 Programmable Clock 2        */

	u32_t	res4[5];	/* 0x4C-0x5C reserved       */

	u32_t	ier;	/* 0x60 Interrupt Enable            */
	u32_t	idr;	/* 0x64 Interrupt Disable           */
	u32_t	sr;	/* 0x68 Status                      */
	u32_t	imr;	/* 0x6C Interrupt Mask              */

	u32_t	fsmr;	/* 0x70 Fast Startup Mode           */
	u32_t	fspr;	/* 0x74 Fast Startup Polarity       */

	u32_t	focr;	/* 0x78 Fault Outpu Clear           */

	u32_t	res5[26];	/* 0x7C-0xE0 reserved       */

	u32_t	wpmr;	/* 0xE4 Write Protect Mode          */
	u32_t	wpsr;	/* 0xE8 Write Protect Status        */

	u32_t	res6[5];	/* 0xEC-0xFC reserved       */

	u32_t	pcer1;	/* 0x100 Peripheral Clock Enable 1  */
	u32_t	pcdr1;	/* 0x104 Peripheral Clock Disable 1 */
	u32_t	pcsr1;	/* 0x108 Peripheral Clock Status 1  */

	u32_t	pcr;	/* 0x10C Peripheral Control         */
};

/*
 * Supply Controller (SUPC)
 * Table 18-2 Section 18.5.2 Page 339
 */
struct __supc {
	u32_t	cr;	/* 0x00 Control                     */
	u32_t	smmr;	/* 0x04 Supply Monitor Mode         */
	u32_t	mr;	/* 0x08 Mode                        */
	u32_t	wumr;	/* 0x0C Wake Up Mode                */
	u32_t	wuir;	/* 0x10 Wake Up Inputs              */
	u32_t	sr;	/* 0x14 Status                      */
};

/* Watchdog timer (WDT) */
struct __wdt {
	u32_t	cr;	/* 0x00 Control Register */
	u32_t	mr;	/* 0x04 Mode Register    */
	u32_t	sr;	/* 0x08 Status Register  */
};

#endif /* _ATMEL_SAM4S_SOC_REGS_H_ */
