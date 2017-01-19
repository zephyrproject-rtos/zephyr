/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Atmel SAM3 family processors.
 *
 * Refer to the datasheet for more information about these registers.
 */

#ifndef _ATMEL_SAM3_SOC_REGS_H_
#define _ATMEL_SAM3_SOC_REGS_H_

/* Peripheral DMA Controller
 *
 * DO NOT USE DIRECTLY! This is to be used within individual
 * peripheral's register struct.
 *
 * Starts at offset 0x100.
 */
struct __pdc {
	uint32_t	rpr;	/* 0x100 Receive Pointer       */
	uint32_t	rcr;	/* 0x104 Receive Counter       */
	uint32_t	tpr;	/* 0x108 Transmit Pointer      */
	uint32_t	tcr;	/* 0x10C Transmit Counter      */
	uint32_t	rnpr;	/* 0x110 Receive Next Pointer  */
	uint32_t	rncr;	/* 0x114 Receive Next Counter  */
	uint32_t	tnpr;	/* 0x118 Transmit Next Pointer */
	uint32_t	tncr;	/* 0x11C Transmit Next Counter */
	uint32_t	ptcr;	/* 0x120 Transfer Control      */
	uint32_t	ptsr;	/* 0x124 Transfer Status       */
};

/* Enhanced Embedded Flash Controller */
struct __eefc {
	uint32_t	fmr;	/* 0x00 Flash Mode Register    */
	uint32_t	fcr;	/* 0x04 Flash Command Register */
	uint32_t	fsr;	/* 0x08 Flash Status Register  */
	uint32_t	frr;	/* 0x0C Flash Result Register  */
};

/* PIO Controller */
struct __pio {
	uint32_t	per;	/* 0x00 Enable                      */
	uint32_t	pdr;	/* 0x04 Disable                     */
	uint32_t	psr;	/* 0x08 Status                      */

	uint32_t	res0;	/* 0x0C reserved                    */

	uint32_t	oer;	/* 0x10 Output Enable               */
	uint32_t	odr;	/* 0x14 Output Disable              */
	uint32_t	osr;	/* 0x18 Output Status               */

	uint32_t	res1;	/* 0x1C reserved                    */

	uint32_t	ifer;	/* 0x20 Glitch Input Filter Enable  */
	uint32_t	ifdr;	/* 0x24 Glitch Input Filter Disable */
	uint32_t	ifsr;	/* 0x28 Glitch Input Fitler Status  */

	uint32_t	res2;	/* 0x2C reserved                    */

	uint32_t	sodr;	/* 0x30 Set Output Data             */
	uint32_t	codr;	/* 0x34 Clear Output Data           */
	uint32_t	odsr;	/* 0x38 Output Data Status          */
	uint32_t	pdsr;	/* 0x3C Pin Data Status             */

	uint32_t	ier;	/* 0x40 Interrupt Enable            */
	uint32_t	idr;	/* 0x44 Interrupt Disable           */
	uint32_t	imr;	/* 0x48 Interrupt Mask              */
	uint32_t	isr;	/* 0x4C Interrupt Status            */

	uint32_t	mder;	/* 0x50 Multi-driver Enable         */
	uint32_t	mddr;	/* 0x54 Multi-driver Disable        */
	uint32_t	mdsr;	/* 0x58 Multi-driver Status         */

	uint32_t	res3;	/* 0x5C reserved                    */

	uint32_t	pudr;	/* 0x60 Pull-up Disable             */
	uint32_t	puer;	/* 0x64 Pull-up Enable              */
	uint32_t	pusr;	/* 0x68 Pad Pull-up Status          */

	uint32_t	res4;	/* 0x6C reserved                    */

	uint32_t	absr;	/* 0x70 Peripheral AB Select        */

	uint32_t	res5[3];	/* 0x74-0x7C reserved       */

	uint32_t	scifsr;	/* 0x80 System Clock Glitch Input   */
				/*        Filter Select             */

	uint32_t	difsr;	/* 0x84 Debouncing Input Filter     */
				/*        Select                    */

	uint32_t	ifdgsr;	/* 0x88 Glitch or Debouncing Input  */
				/*        Filter Clock Selection    */
				/*        Status                    */

	uint32_t	scdr;	/* 0x8C Slow Clock Divider Debounce */

	uint32_t	res6[4];	/* 0x90-0x9C reserved       */

	uint32_t	ower;	/* 0xA0 Output Write Enable         */
	uint32_t	owdr;	/* 0xA4 Output Write Disable        */
	uint32_t	owsr;	/* 0xA8 Output Write Status         */

	uint32_t	res7;	/* 0xAC reserved                    */

	uint32_t	aimer;	/* 0xB0 Additional Interrupt Modes  */
				/*        Enable                    */
	uint32_t	aimdr;	/* 0xB4 Additional Interrupt Modes  */
				/*        Disable                   */
	uint32_t	aimmr;	/* 0xB8 Additional Interrupt Modes  */
				/*        Mask                      */

	uint32_t	res8;	/* 0xBC reserved                    */

	uint32_t	esr;	/* 0xC0 Edge Select                 */
	uint32_t	lsr;	/* 0xC4 Level Select                */
	uint32_t	elsr;	/* 0xC8 Edge/Level Status           */

	uint32_t	res9;	/* 0xCC reserved                    */

	uint32_t	fellsr;	/* 0xD0 Falling Edge/Low Level Sel  */
	uint32_t	rehlsr;	/* 0xD4 Rising Edge/High Level Sel  */
	uint32_t	frlhsr;	/* 0xD8 Fall/Rise - Low/High Status */

	uint32_t	res10;	/* 0xDC reserved                    */

	uint32_t	locksr;	/* 0xE0 Lock Status                 */

	uint32_t	wpmr;	/* 0xE4 Write Protect Mode          */
	uint32_t	wpsr;	/* 0xE8 Write Protect Status        */
};

/* Power Management Controller */
struct __pmc {
	uint32_t	scer;	/* 0x00 System Clock Enable         */
	uint32_t	scdr;	/* 0x04 System Clock Disable        */
	uint32_t	scsr;	/* 0x08 System Clock Status         */

	uint32_t	res0;	/* 0x0C reserved                    */

	uint32_t	pcer0;	/* 0x10 Peripheral Clock Enable 0   */
	uint32_t	pcdr0;	/* 0x14 Peripheral Clock Disable 0  */
	uint32_t	pcsr0;	/* 0x18 Peripheral Clock Status 0   */

	uint32_t	ckgr_uckr;	/* 0x1C UTMI Clock          */
	uint32_t	ckgr_mor;	/* 0x20 Main Oscillator     */
	uint32_t	ckgr_mcfr;	/* 0x24 Main Clock Freq.    */
	uint32_t	ckgr_pllar;	/* 0x28 PLLA                */

	uint32_t	res1;	/* 0x2C reserved                    */

	uint32_t	mckr;	/* 0x30 Master Clock                */

	uint32_t	res2;	/* 0x34 reserved                    */

	uint32_t	usb;	/* 0x38 USB Clock                   */

	uint32_t	res3;	/* 0x3C reserved                    */

	uint32_t        pck0;	/* 0x40 Programmable Clock 0        */
	uint32_t        pck1;	/* 0x44 Programmable Clock 1        */
	uint32_t        pck2;	/* 0x48 Programmable Clock 2        */

	uint32_t	res4[5];	/* 0x4C-0x5C reserved       */

	uint32_t	ier;	/* 0x60 Interrupt Enable            */
	uint32_t	idr;	/* 0x64 Interrupt Disable           */
	uint32_t	sr;	/* 0x68 Status                      */
	uint32_t	imr;	/* 0x6C Interrupt Mask              */

	uint32_t	fsmr;	/* 0x70 Fast Startup Mode           */
	uint32_t	fspr;	/* 0x74 Fast Startup Polarity       */

	uint32_t	focr;	/* 0x78 Fault Outpu Clear           */

	uint32_t	res5[26];	/* 0x7C-0xE0 reserved       */

	uint32_t	wpmr;	/* 0xE4 Write Protect Mode          */
	uint32_t	wpsr;	/* 0xE8 Write Protect Status        */

	uint32_t	res6[5];	/* 0xEC-0xFC reserved       */

	uint32_t	pcer1;	/* 0x100 Peripheral Clock Enable 1  */
	uint32_t	pcdr1;	/* 0x104 Peripheral Clock Disable 1 */
	uint32_t	pcsr1;	/* 0x108 Peripheral Clock Status 1  */

	uint32_t	pcr;	/* 0x10C Peripheral Control         */
};

/* Supply Controller (SUPC) */
struct __supc {
	uint32_t	cr;	/* 0x00 Control                     */
	uint32_t	smmr;	/* 0x04 Supply Monitor Mode         */
	uint32_t	mr;	/* 0x08 Mode                        */
	uint32_t	wumr;	/* 0x0C Wake Up Mode                */
	uint32_t	wuir;	/* 0x10 Wake Up Inputs              */
	uint32_t	sr;	/* 0x14 Status                      */
};

/* Two-wire Interface (TWI), aka I2C */
struct __twi {
	uint32_t	cr;	/* 0x00 Control                     */
	uint32_t	mmr;	/* 0x04 Master Mode                 */
	uint32_t	smr;	/* 0x08 Slave Mode                  */
	uint32_t	iadr;	/* 0x0C Internal Address            */
	uint32_t	cwgr;	/* 0x10 Clock Waveform Generator    */

	uint32_t	rev0[3];	/* 0x14-0x1C reserved       */

	uint32_t	sr;	/* 0x20 Status                      */

	uint32_t	ier;	/* 0x24 Interrupt Enable            */
	uint32_t	idr;	/* 0x28 Interrupt Disable           */
	uint32_t	imr;	/* 0x2C Interrupt Mask              */

	uint32_t	rhr;	/* 0x30 Receive Holding             */
	uint32_t	thr;	/* 0x34 Transmit Holding            */

	uint32_t	rev1[50];	/* 0x38-0xFC Reserved       */

	struct __pdc	pdc;	/* 0x100 - 0x124 PDC                */
};

/* Watchdog timer (WDT) */
struct __wdt {
	uint32_t	cr;	/* 0x00 Control Register */
	uint32_t	mr;	/* 0x04 Mode Register    */
	uint32_t	sr;	/* 0x08 Status Register  */
};

#endif /* _ATMEL_SAM3_SOC_REGS_H_ */
