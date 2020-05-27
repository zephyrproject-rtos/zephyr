/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the ARM Ltd MPS2.
 *
 */

#ifndef _ARM_MPS2_REGS_H_
#define _ARM_MPS2_REGS_H_

#include <sys/util.h>
#include <zephyr/types.h>

/* Registers in the FPGA system control block */
struct mps2_fpgaio {
	/* Offset: 0x000 LED connections */
	volatile uint32_t led0;
	/* Offset: 0x004 RESERVED */
	volatile uint32_t reserved1;
	/* Offset: 0x008 Buttons */
	volatile uint32_t button;
	/* Offset: 0x00c RESERVED */
	volatile uint32_t reserved2;
	/* Offset: 0x010 1Hz up counter */
	volatile uint32_t clk1hz;
	/* Offset: 0x014 100Hz up counter */
	volatile uint32_t clk100hz;
	/* Offset: 0x018 Cycle up counter */
	volatile uint32_t counter;
	/* Offset: 0x01c Reload value for prescale counter */
	volatile uint32_t prescale;
	/* Offset: 0x020 32-bit Prescale counter */
	volatile uint32_t pscntr;
	/* Offset: 0x024 RESERVED */
	volatile uint32_t reserved3[10];
	/* Offset: 0x04c Misc control */
	volatile uint32_t misc;
};

/* Defines for bits in fpgaio led0 register */
#define FPGAIO_LED0_USERLED0		0
#define FPGAIO_LED0_USERLED1		1

/* Defines for bits in fpgaio button register */
#define FPGAIO_BUTTON_USERPB0		0
#define FPGAIO_BUTTON_USERPB1		1

/* Defines for bits in fpgaio misc register */
#define FPGAIO_MISC_CLCD_CS		0
#define FPGAIO_MISC_SPI_SS		1
#define FPGAIO_MISC_CLCD_RESET		3
#define FPGAIO_MISC_CLCD_RS		4
#define FPGAIO_MISC_CLCD_RD		5
#define FPGAIO_MISC_CLCD_BL_CTRL	6
#define FPGAIO_MISC_ADC_SPI_CS		7
#define FPGAIO_MISC_SHIELD0_SPI_CS	8
#define FPGAIO_MISC_SHIELD1_SPI_CS	9

#endif /* _ARM_MPS2_REGS_H_ */
