/*
 * Copyright 2020-2022, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCUX_LPC_SYSCON_H_
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCUX_LPC_SYSCON_H_

#define MCUX_FLEXCOMM0_CLK		0
#define MCUX_FLEXCOMM1_CLK		1
#define MCUX_FLEXCOMM2_CLK		2
#define MCUX_FLEXCOMM3_CLK		3
#define MCUX_FLEXCOMM4_CLK		4
#define MCUX_FLEXCOMM5_CLK		5
#define MCUX_FLEXCOMM6_CLK		6
#define MCUX_FLEXCOMM7_CLK		7
#define MCUX_FLEXCOMM8_CLK		8
#define MCUX_FLEXCOMM9_CLK		9
#define MCUX_FLEXCOMM10_CLK		10
#define MCUX_FLEXCOMM11_CLK		11
#define MCUX_FLEXCOMM12_CLK		12
#define MCUX_FLEXCOMM13_CLK		13
#define MCUX_HS_SPI_CLK			14
#define MCUX_PMIC_I2C_CLK		15
#define MCUX_HS_SPI1_CLK		16

#define MCUX_USDHC1_CLK			20
#define MCUX_USDHC2_CLK			21

#define MCUX_CTIMER_CLK_OFFSET		22

#define MCUX_CTIMER0_CLK		0
#define MCUX_CTIMER1_CLK		1
#define MCUX_CTIMER2_CLK		2
#define MCUX_CTIMER3_CLK		3
#define MCUX_CTIMER4_CLK		4

#define MCUX_MCAN_CLK			27

#define MCUX_BUS_CLK			28

#define MCUX_SDIF_CLK			29

#define MCUX_I3C_CLK			30

#define MCUX_OSC32K_CLK			31  /* The 32 kHz output of the RTC oscillator */
#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_MCUX_LPC_SYSCON_H_ */
