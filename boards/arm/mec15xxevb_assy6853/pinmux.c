/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <drivers/pinmux.h>

#include "soc.h"

static int board_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_XEC_GPIO000_036
	struct device *porta =
		device_get_binding(CONFIG_PINMUX_XEC_GPIO000_036_NAME);
#endif
#ifdef CONFIG_PINMUX_XEC_GPIO040_076
	struct device *portb =
		device_get_binding(CONFIG_PINMUX_XEC_GPIO040_076_NAME);
#endif
#ifdef CONFIG_PINMUX_XEC_GPIO100_136
	struct device *portc =
		device_get_binding(CONFIG_PINMUX_XEC_GPIO100_136_NAME);
#endif
#ifdef CONFIG_PINMUX_XEC_GPIO140_176
	struct device *portd =
		device_get_binding(CONFIG_PINMUX_XEC_GPIO140_176_NAME);
#endif
#ifdef CONFIG_PINMUX_XEC_GPIO200_236
	struct device *porte =
		device_get_binding(CONFIG_PINMUX_XEC_GPIO200_236_NAME);
#endif
#ifdef CONFIG_PINMUX_XEC_GPIO240_276
	struct device *portf =
		device_get_binding(CONFIG_PINMUX_XEC_GPIO240_276_NAME);
#endif

	/* Configure GPIO bank before usage
	 * VTR1 is not configurable
	 * VTR2 doesn't need configuration if setting VTR2_STRAP
	 */
#ifdef CONFIG_SOC_MEC1501_VTR3_1_8V
	ECS_REGS->GPIO_BANK_PWR |= MCHP_ECS_VTR3_LVL_18;
#endif
	/* Release JTAG TDI and JTAG TDO pins so they can be
	 * controlled by their respective PCR register (UART2).
	 * For more details see table 44-1
	 */
	ECS_REGS->DEBUG_CTRL = (MCHP_ECS_DCTRL_DBG_EN |
				MCHP_ECS_DCTRL_MODE_SWD);

	/* See table 2-4 from the data sheet for pin multiplexing*/
#ifdef CONFIG_UART_NS16550_PORT_2
	/* Set muxing, for UART 2 TX/RX and power up */
	mchp_pcr_periph_slp_ctrl(PCR_UART2, MCHP_PCR_SLEEP_DIS);

	UART2_REGS->CFG_SEL = (MCHP_UART_LD_CFG_INTCLK +
		MCHP_UART_LD_CFG_RESET_SYS + MCHP_UART_LD_CFG_NO_INVERT);
	UART2_REGS->ACTV = MCHP_UART_LD_ACTIVATE;

	pinmux_pin_set(portd, MCHP_GPIO_145, MCHP_GPIO_CTRL_MUX_F2);
	pinmux_pin_set(portd, MCHP_GPIO_146, MCHP_GPIO_CTRL_MUX_F2);
#endif

#ifdef CONFIG_ADC_XEC
	/* Disable sleep for ADC block */
	mchp_pcr_periph_slp_ctrl(PCR_ADC, MCHP_PCR_SLEEP_DIS);

	/* ADC pin muxes, ADC00 - ADC07 */
	pinmux_pin_set(porte, MCHP_GPIO_200, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porte, MCHP_GPIO_201, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porte, MCHP_GPIO_202, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porte, MCHP_GPIO_203, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porte, MCHP_GPIO_204, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porte, MCHP_GPIO_205, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porte, MCHP_GPIO_206, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porte, MCHP_GPIO_207, MCHP_GPIO_CTRL_MUX_F1);

	/* VREF2_ADC */
	pinmux_pin_set(portb, MCHP_GPIO_067, MCHP_GPIO_CTRL_MUX_F1);
#endif /* CONFIG_ADC_XEC */

#ifdef CONFIG_I2C_XEC_0
	/* Set muxing, for I2C0 - SMB00 */
	pinmux_pin_set(porta, MCHP_GPIO_003, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porta, MCHP_GPIO_004, MCHP_GPIO_CTRL_MUX_F1);
#endif

#ifdef CONFIG_I2C_XEC_1
	/* Set muxing for I2C1 - SMB01 */
	pinmux_pin_set(portc, MCHP_GPIO_130, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portc, MCHP_GPIO_131, MCHP_GPIO_CTRL_MUX_F1);
#endif

#ifdef CONFIG_I2C_XEC_2
	/* Set muxing, for I2C2 - SMB04 */
	pinmux_pin_set(portd, MCHP_GPIO_143, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portd, MCHP_GPIO_144, MCHP_GPIO_CTRL_MUX_F1);
#endif

#ifdef CONFIG_ESPI_XEC
	mchp_pcr_periph_slp_ctrl(PCR_ESPI, MCHP_PCR_SLEEP_DIS);
	/* ESPI RESET */
	pinmux_pin_set(portb, MCHP_GPIO_061, MCHP_GPIO_CTRL_MUX_F1);
	/* ESPI ALERT */
	pinmux_pin_set(portb, MCHP_GPIO_063, MCHP_GPIO_CTRL_MUX_F1);
	/* ESPI CS */
	pinmux_pin_set(portb, MCHP_GPIO_066, MCHP_GPIO_CTRL_MUX_F1);
	/* ESPI CLK */
	pinmux_pin_set(portb, MCHP_GPIO_065, MCHP_GPIO_CTRL_MUX_F1);
	/* ESPI IO1-4*/
	pinmux_pin_set(portb, MCHP_GPIO_070, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portb, MCHP_GPIO_071, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portb, MCHP_GPIO_072, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portb, MCHP_GPIO_073, MCHP_GPIO_CTRL_MUX_F1);
#endif

#ifdef CONFIG_PS2_XEC_0
	/* Set muxing for PS20B*/
	mchp_pcr_periph_slp_ctrl(PCR_PS2_0, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(porta, MCHP_GPIO_007, MCHP_GPIO_CTRL_MUX_F2 |
		       MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	pinmux_pin_set(porta, MCHP_GPIO_010, MCHP_GPIO_CTRL_MUX_F2 |
		       MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
#endif

#ifdef CONFIG_PS2_XEC_1
	/* Set muxing for PS21B*/
	mchp_pcr_periph_slp_ctrl(PCR_PS2_1, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(portd, MCHP_GPIO_154, MCHP_GPIO_CTRL_MUX_F2 |
		       MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	pinmux_pin_set(portd, MCHP_GPIO_155, MCHP_GPIO_CTRL_MUX_F2 |
		       MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
#endif

#ifdef CONFIG_PWM_XEC
#if defined(DT_INST_0_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM0, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(portb, MCHP_GPIO_053, MCHP_GPIO_CTRL_MUX_F1);
#endif

#if defined(DT_INST_1_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM1, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(portb, MCHP_GPIO_054, MCHP_GPIO_CTRL_MUX_F1);
#endif

#if defined(DT_INST_2_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM2, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(portb, MCHP_GPIO_055, MCHP_GPIO_CTRL_MUX_F1);
#endif

#if defined(DT_INST_3_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM3, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(portb, MCHP_GPIO_056, MCHP_GPIO_CTRL_MUX_F1);
#endif

#if defined(DT_INST_4_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM4, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(porta, MCHP_GPIO_011, MCHP_GPIO_CTRL_MUX_F2);
#endif

#if defined(DT_INST_5_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM5, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(porta, MCHP_GPIO_002, MCHP_GPIO_CTRL_MUX_F1);
#endif

#if defined(DT_INST_6_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM6, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(porta, MCHP_GPIO_014, MCHP_GPIO_CTRL_MUX_F1);
#endif

#if defined(DT_INST_7_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM7, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(porta, MCHP_GPIO_015, MCHP_GPIO_CTRL_MUX_F1);
#endif

#if defined(DT_INST_8_MICROCHIP_XEC_PWM)
	mchp_pcr_periph_slp_ctrl(PCR_PWM8, MCHP_PCR_SLEEP_DIS);
	pinmux_pin_set(porta, MCHP_GPIO_035, MCHP_GPIO_CTRL_MUX_F1);
#endif
#endif /* CONFIG_PWM_XEC  */

#if defined CONFIG_KSCAN_XEC
       /* KSCAN KSO00 */
	pinmux_pin_set(portb, MCHP_GPIO_040,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO01 */
	pinmux_pin_set(portb, MCHP_GPIO_045,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO02 */
	pinmux_pin_set(portb, MCHP_GPIO_046,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO012 */
	pinmux_pin_set(portc, MCHP_GPIO_125,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO013 */
	pinmux_pin_set(portc, MCHP_GPIO_126,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO03 */
	pinmux_pin_set(portb, MCHP_GPIO_047,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO04 */
	pinmux_pin_set(portc, MCHP_GPIO_107,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO05 */
	pinmux_pin_set(portc, MCHP_GPIO_112,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO06 */
	pinmux_pin_set(portc, MCHP_GPIO_113,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO14 */
	pinmux_pin_set(portd, MCHP_GPIO_152,
			MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO15 */
	pinmux_pin_set(portd, MCHP_GPIO_151,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO07 */
	pinmux_pin_set(portc, MCHP_GPIO_120,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO08 */
	pinmux_pin_set(portc, MCHP_GPIO_121,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO09 */
	pinmux_pin_set(portc, MCHP_GPIO_122,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO10 */
	pinmux_pin_set(portc, MCHP_GPIO_123,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);
	/* KSCAN KSO11 */
	pinmux_pin_set(portc, MCHP_GPIO_124,
		       MCHP_GPIO_CTRL_MUX_F2 | MCHP_GPIO_CTRL_PUD_PU);

	/* KSCAN KSI00 */
	pinmux_pin_set(porta, MCHP_GPIO_017,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	/* KSCAN KSI01 */
	pinmux_pin_set(porta, MCHP_GPIO_020,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	/* KSCAN KSI02 */
	pinmux_pin_set(porta, MCHP_GPIO_021,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	/* KSCAN KSI03 */
	pinmux_pin_set(porta, MCHP_GPIO_026,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	/* KSCAN KSI04 */
	pinmux_pin_set(porta, MCHP_GPIO_027,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	/* KSCAN KSI05 */
	pinmux_pin_set(porta, MCHP_GPIO_030,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	/* KSCAN KSI06 */
	pinmux_pin_set(porta, MCHP_GPIO_031,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
	/* KSCAN KSI07 */
	pinmux_pin_set(porta, MCHP_GPIO_032,
		       MCHP_GPIO_CTRL_MUX_F1 | MCHP_GPIO_CTRL_BUFT_OPENDRAIN);
#endif /* CONFIG_KSCAN_XEC */

#ifdef CONFIG_SPI_XEC_QMSPI
#if defined(DT_INST_0_MICROCHIP_XEC_QMSPI)
	mchp_pcr_periph_slp_ctrl(PCR_QMSPI, MCHP_PCR_SLEEP_DIS);

#if DT_SPI_XEC_QMSPI_0_PORT_SEL == 0
	/* Port 0: Shared SPI pins. Shared has two chip selects */
#if DT_SPI_XEC_QMSPI_0_CHIP_SELECT == 0
	pinmux_pin_set(portb, MCHP_GPIO_055, MCHP_GPIO_CTRL_MUX_F2);
#else
	pinmux_pin_set(porta, MCHP_GPIO_002, MCHP_GPIO_CTRL_MUX_F2);
#endif
	pinmux_pin_set(portb, MCHP_GPIO_056, MCHP_GPIO_CTRL_MUX_F2);
	pinmux_pin_set(porte, MCHP_GPIO_223, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porte, MCHP_GPIO_224, MCHP_GPIO_CTRL_MUX_F2);
#if DT_SPI_XEC_QMSPI_0_LINES == 4
	pinmux_pin_set(porte, MCHP_GPIO_227, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(porta, MCHP_GPIO_016, MCHP_GPIO_CTRL_MUX_F2);
#endif

#else
	/* Port 1: Private SPI pins. Only one chip select */
	pinmux_pin_set(portc, MCHP_GPIO_124, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portc, MCHP_GPIO_125, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portc, MCHP_GPIO_121, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portc, MCHP_GPIO_122, MCHP_GPIO_CTRL_MUX_F1);
#if DT_SPI_XEC_QMSPI_0_LINES == 4
	pinmux_pin_set(portc, MCHP_GPIO_123, MCHP_GPIO_CTRL_MUX_F1);
	pinmux_pin_set(portc, MCHP_GPIO_126, MCHP_GPIO_CTRL_MUX_F1);
#endif
#endif /* DT_SPI_XEC_QMSPI_0_PORT_SEL == 0 */

#endif /* DT_INST_0_MICROCHIP_XEC_QMSPI */
#endif /* CONFIG_SPI_XEC_QMSPI */

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
