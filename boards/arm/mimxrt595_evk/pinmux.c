/*
 * Copyright (c) 2022,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iopctl.h>
#include <soc.h>

static int mimxrt595_evk_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_usart, okay) && CONFIG_SERIAL
	/* USART0 RX,  TX */
	uint32_t port0_pin1_config = (/* Pin is configured as FC0_TXD_SCL_MISO_WS */
			IOPCTL_PIO_FUNC1 |
			/* Disable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_DI |
			/* Enable pull-down function */
			IOPCTL_PIO_PULLDOWN_EN |
			/* Disable input buffer function */
			IOPCTL_PIO_INBUF_DI |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Normal drive */
			IOPCTL_PIO_FULLDRIVE_DI |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is disabled */
			IOPCTL_PIO_PSEDRAIN_DI |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT0 PIN1 (coords: G2) is configured as FC0_TXD_SCL_MISO_WS */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 1U, port0_pin1_config);

	uint32_t port0_pin2_config = (/* Pin is configured as FC0_RXD_SDA_MOSI_DATA */
			IOPCTL_PIO_FUNC1 |
			/* Disable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_DI |
			/* Enable pull-down function */
			IOPCTL_PIO_PULLDOWN_EN |
			/* Enables input buffer function */
			IOPCTL_PIO_INBUF_EN |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Normal drive */
			IOPCTL_PIO_FULLDRIVE_DI |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is disabled */
			IOPCTL_PIO_PSEDRAIN_DI |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT0 PIN2 (coords: G4) is configured as FC0_RXD_SDA_MOSI_DATA */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 2U, port0_pin2_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(sw0), gpios, pin)
	uint32_t port0_pin25_config = (/* Pin is configured as PIO0_25 */
			IOPCTL_PIO_FUNC0 |
			/* Disable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_DI |
			/* Enable pull-down function */
			IOPCTL_PIO_PULLDOWN_EN |
			/* Enables input buffer function */
			IOPCTL_PIO_INBUF_EN |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Normal drive */
			IOPCTL_PIO_FULLDRIVE_DI |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is disabled */
			IOPCTL_PIO_PSEDRAIN_DI |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT0 PIN25 (coords: C12) is configured as PIO0_25 */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 25U, port0_pin25_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(sw1), gpios, pin)
	uint32_t port0_pin10_config = (/* Pin is configured as PIO0_10 */
			IOPCTL_PIO_FUNC0 |
			/* Disable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_DI |
			/* Enable pull-down function */
			IOPCTL_PIO_PULLDOWN_EN |
			/* Enables input buffer function */
			IOPCTL_PIO_INBUF_EN |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Normal drive */
			IOPCTL_PIO_FULLDRIVE_DI |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is disabled */
			IOPCTL_PIO_PSEDRAIN_DI |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT0 PIN10 (coords: J3) is configured as PIO0_10 */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 10U, port0_pin10_config);
#endif

#ifdef DT_GPIO_LEDS_LED_3_GPIOS_CONTROLLER
	uint32_t port0_pin14_config = (/* Pin is configured as PIO0_14 */
			IOPCTL_PIO_FUNC0 |
			/* Disable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_DI |
			/* Enable pull-down function */
			IOPCTL_PIO_PULLDOWN_EN |
			/* Disable input buffer function */
			IOPCTL_PIO_INBUF_DI |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Normal drive */
			IOPCTL_PIO_FULLDRIVE_DI |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is disabled */
			IOPCTL_PIO_PSEDRAIN_DI |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT0 PIN14 (coords: B12) is configured as PIO0_14 */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 14U, port0_pin14_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm12), nxp_lpc_usart, okay) && CONFIG_SERIAL
	/* USART12 RX, TX */
	const uint32_t port4_pin30_config = (/* Pin is configured as FC12_TXD_SCL_MISO */
			IOPCTL_PIO_FUNC6 |
			/* Disable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_DI |
			/* Enable pull-down function */
			IOPCTL_PIO_PULLDOWN_EN |
			/* Disable input buffer function */
			IOPCTL_PIO_INBUF_DI |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Normal drive */
			IOPCTL_PIO_FULLDRIVE_DI |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is disabled */
			IOPCTL_PIO_PSEDRAIN_DI |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT4 PIN30 (coords: N8) is configured as FC12_TXD_SCL_MISO */
	IOPCTL_PinMuxSet(IOPCTL, 4U, 30U, port4_pin30_config);

	const uint32_t port4_pin31_config = (/* Pin is configured as FC12_RXD_SDA_MOSI */
			IOPCTL_PIO_FUNC6 |
			/* Disable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_DI |
			/* Enable pull-down function */
			IOPCTL_PIO_PULLDOWN_EN |
			/* Disable input buffer function */
			IOPCTL_PIO_INBUF_DI |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Normal drive */
			IOPCTL_PIO_FULLDRIVE_DI |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is disabled */
			IOPCTL_PIO_PSEDRAIN_DI |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT4 PIN31 (coords: N10) is configured as FC12_RXD_SDA_MOSI */
	IOPCTL_PinMuxSet(IOPCTL, 4U, 31U, port4_pin31_config);

#endif

	return 0;
}

/* priority set to CONFIG_PINMUX_INIT_PRIORITY value */
SYS_INIT(mimxrt595_evk_pinmux_init, PRE_KERNEL_1, 45);
