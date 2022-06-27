/*
 * Copyright (c) 2018 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/init.h>

#define PMODMUX_BASE_ADDR	0xF0000000

/*
 * 32-bits, offset 0x0, This register controls mapping of the peripheral device
 * signals on Pmod connectors
 */
#define PMOD_MUX_CTRL	0
/* 32-bits, offset 0x4 */
#define I2C_MAP_CTRL	1

/*
 * 32-bits, offset 0x8, SPI_MAP_CTRL[0] selects the mode of operation of the SPI
 * Slave: Normal operation, SPI_MAP_CTRL[0]=0: SPI Slave is connected to Pmod1
 * at connector J1. Loop-back mode, SPI_MAP_CTRL[0]=1: SPI Slave is connected to
 * the SPI Master inside the FPGA using CS4.
 */
#define SPI_MAP_CTRL	2
/*
 * 32-bits, offset 0x8, This register controls the mapping of the UART signals
 * on the Pmod1 connector.
 */
#define UART_MAP_CTRL	3

#define BIT0			(0)
#define BIT1			(1)
#define BIT2			(2)
#define BIT3			(3)
#define PM1_OFFSET		(0)
#define PM2_OFFSET		(4)
#define PM3_OFFSET		(8)
#define PM4_OFFSET		(12)
#define PM5_OFFSET		(16)
#define PM6_OFFSET		(20)
#define PM7_OFFSET		(24)

#define PM1_MASK		(0xf << PM1_OFFSET)
#define PM2_MASK		(0xf << PM2_OFFSET)
#define PM3_MASK		(0xf << PM3_OFFSET)
#define PM4_MASK		(0xf << PM4_OFFSET)
#define PM5_MASK		(0xf << PM5_OFFSET)
#define PM6_MASK		(0xf << PM6_OFFSET)
#define PM7_MASK		(0xf << PM7_OFFSET)

#define SPI_MAP_NORMAL		(0)
#define SPI_MAP_LOOPBACK	(1)

#define UART_MAP_TYPE4		(0xE4)
#define UART_MAP_TYPE3		(0x6C)

/* all pins are configured as GPIO inputs */
#define PMOD_MUX_CTRL_DEFAULT	(0)
/* normal SPI mode */
#define SPI_MAP_CTRL_DEFAULT	(SPI_MAP_NORMAL)
/* TYPE4 PMOD compatible */
#define UART_MAP_CTRL_DEFAULT	(UART_MAP_TYPE4)

/* Pmod1[4:1] are connected to DW GPIO Port C[11:8] */
#define PM1_UR_GPIO_C		((0 << BIT0) << PM1_OFFSET)
/* Pmod1[4:1] are connected to DW UART0 signals */
#define PM1_UR_UART_0		((1 << BIT0) << PM1_OFFSET)

/* Pmod1[10:7] are connected to DW GPIO Port A[11:8] */
#define PM1_LR_GPIO_A		((0 << BIT2) << PM1_OFFSET)
/* Pmod1[10:7] are connected to DW SPI Slave signals */
#define PM1_LR_SPI_S		((1 << BIT2) << PM1_OFFSET)
/*
 * Pmod2[4:1]	 are connected to DW GPIO Port C[15:12],
 * Pmod2[10:7] are connected to DW GPIO Port A[15:12]
 */
#define PM2_GPIO_AC		((0 << BIT0) << PM2_OFFSET)
/* connect I2C to Pmod2[4:1] and halt/run interface to Pmod2[10:7] */
#define PM2_I2C_HRI		((1 << BIT0) << PM2_OFFSET)
/*
 * Pmod3[4:1]  are connected to DW GPIO Port C[19:16],
 * Pmod3[10:7] are connected to DW GPIO Port A[19:16]
 */
#define PM3_GPIO_AC		((0 << BIT0) << PM3_OFFSET)
/*
 * Pmod3[4:3]  are connected to DW I2C signals,
 * Pmod3[2:1]  are connected to DW GPIO Port D[1:0],
 * Pmod3[10:7] are connected to DW GPIO Port D[3:2]
 */
#define PM3_I2C_GPIO_D		((1 << BIT0) << PM3_OFFSET)
/*
 * Pmod4[4:1]  are connected to DW GPIO Port C[23:20],
 * Pmod4[10:7] are connected to DW GPIO Port A[23:20]
 */
#define PM4_GPIO_AC		((0 << BIT0) << PM4_OFFSET)
/*
 * Pmod4[4:3]  are connected to DW I2C signals,
 * Pmod4[2:1]  are connected to DW GPIO Port D[5:4],
 * Pmod4[10:7] are connected to DW GPIO Port D[7:6]
 */
#define PM4_I2C_GPIO_D		((1 << BIT0) << PM4_OFFSET)

/* Pmod5[4:1] are connected to DW GPIO Port C[27:24] */
#define PM5_UR_GPIO_C		((0 << BIT0) << PM5_OFFSET)
/* Pmod5[4:1] are connected to DW SPI Master signals using CS1_N */
#define PM5_UR_SPI_M1		((1 << BIT0) << PM5_OFFSET)
/* Pmod5[10:7] are connected to DW GPIO Port A[27:24] */
#define PM5_LR_GPIO_A		((0 << BIT2) << PM5_OFFSET)
/* Pmod5[10:7] are connected to DW SPI Master signals using CS2_N */
#define PM5_LR_SPI_M2		((1 << BIT2) << PM5_OFFSET)

/* Pmod6[4:1] are connected to DW GPIO Port C[31:28] */
#define PM6_UR_GPIO_C		((0 << BIT0) << PM6_OFFSET)
/* Pmod6[4:1] are connected to DW SPI Master signals using CS0_N */
#define PM6_UR_SPI_M0		((1 << BIT0) << PM6_OFFSET)
/* Pmod6[10:7] are connected to DW GPIO Port A[31:28] */
#define PM6_LR_GPIO_A		((0 << BIT2) << PM6_OFFSET)
/*
 * Pmod6[8:7] are connected to the DW SPI Master chip select signals CS1_N and
 * CS2_N, Pmod6[6:5] are connected to the ARC EM halt and sleep status signals
 */
#define PM6_LR_CSS_STAT		((1 << BIT2) << PM6_OFFSET)


static int pmod_mux_init(const struct device *dev)
{
	volatile uint32_t *mux_regs = (uint32_t *)(PMODMUX_BASE_ADDR);

	mux_regs[SPI_MAP_CTRL] =  SPI_MAP_CTRL_DEFAULT;
	mux_regs[UART_MAP_CTRL] = UART_MAP_CTRL_DEFAULT;

/**
 * + Please refer to corresponding EMSK User Guide for detailed
 *   information -> Appendix: A  Hardware Functional Description ->
 *   Pmods Configuration summary
 * + Set up pin-multiplexer of all PMOD connections
 *   - PM1 J1: Upper row as UART 0, lower row as SPI Slave
 *   - PM2 J2: IIC 0 and run/halt signals
 *   - PM3 J3: GPIO Port A and Port C
 *   - PM4 J4: IIC 1 and Port D
 *   - PM5 J5: Upper row as SPI Master, lower row as Port A
 *   - PM6 J6: Upper row as SPI Master, lower row as Port A
 */
	mux_regs[PMOD_MUX_CTRL] = PM1_UR_UART_0 | PM1_LR_SPI_S
				| PM2_I2C_HRI | PM3_GPIO_AC
				| PM4_I2C_GPIO_D | PM5_UR_SPI_M1
				| PM5_LR_GPIO_A	| PM6_UR_SPI_M0
				| PM6_LR_GPIO_A;
	return 0;
}


SYS_INIT(pmod_mux_init, PRE_KERNEL_1,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
