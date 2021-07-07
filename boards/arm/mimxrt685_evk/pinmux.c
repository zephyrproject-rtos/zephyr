/*
 * Copyright (c) 2020,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <fsl_iopctl.h>
#include <soc.h>

static int mimxrt685_evk_pinmux_init(const struct device *dev)
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
	uint32_t port1_pin1_config = (/* Pin is configured as PIO1_1 */
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
	/* PORT1 PIN1 (coords: G15) is configured as PIO1_1 */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 1U, port1_pin1_config);
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

#ifdef DT_GPIO_LEDS_LED_1_GPIOS_CONTROLLER
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
	/* PORT0 PIN14 (coords: A3) is configured as PIO0_14 */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 14U, port0_pin14_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_i2c, okay) && CONFIG_I2C
	uint32_t port0_pin17_config = (/* Pin is configured as FC2_CTS_SDA_SSEL0 */
			IOPCTL_PIO_FUNC1 |
			/* Enable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_EN |
			/* Enable pull-up function */
			IOPCTL_PIO_PULLUP_EN |
			/* Enables input buffer function */
			IOPCTL_PIO_INBUF_EN |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Full drive enable */
			IOPCTL_PIO_FULLDRIVE_EN |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is enabled */
			IOPCTL_PIO_PSEDRAIN_EN |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT0 PIN17 (coords: D7) is configured as FC2_CTS_SDA_SSEL0 */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 17U, port0_pin17_config);

	uint32_t port0_pin18_config = (/* Pin is configured as FC2_RTS_SCL_SSEL1 */
			IOPCTL_PIO_FUNC1 |
			/* Enable pull-up / pull-down function */
			IOPCTL_PIO_PUPD_EN |
			/* Enable pull-up function */
			IOPCTL_PIO_PULLUP_EN |
			/* Enables input buffer function */
			IOPCTL_PIO_INBUF_EN |
			/* Normal mode */
			IOPCTL_PIO_SLEW_RATE_NORMAL |
			/* Full drive enable */
			IOPCTL_PIO_FULLDRIVE_EN |
			/* Analog mux is disabled */
			IOPCTL_PIO_ANAMUX_DI |
			/* Pseudo Output Drain is enabled */
			IOPCTL_PIO_PSEDRAIN_EN |
			/* Input function is not inverted */
			IOPCTL_PIO_INV_DI);
	/* PORT0 PIN18 (coords: B7) is configured as FC2_RTS_SCL_SSEL1 */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 18U, port0_pin18_config);
#endif

#ifdef CONFIG_FXOS8700_TRIGGER
	uint32_t port1_pin5_config = (/* Pin is configured as PIO1_5 */
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
	/* PORT1 PIN5 (coords: J16) is configured as PIO1_5 */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 5U, port1_pin5_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_usart, okay) && CONFIG_SERIAL
	/* USART4 RX,  TX */
	uint32_t port0_pin29_config = (/* Pin is configured as FC4_TXD_SCL_MISO_WS */
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
	/* PORT0 PIN1 (coords: G2) is configured as FC4_TXD_SCL_MISO_WS  */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 29U, port0_pin29_config);

	uint32_t port0_pin30_config = (/* Pin is configured as FC4_RXD_SDA_MOSI_DATA  */
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
	/* PORT0 PIN2 (coords: G4) is configured as FC4_RXD_SDA_MOSI_DATA */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 30U, port0_pin30_config);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcomm5), okay) && CONFIG_SPI
	uint32_t port1_pin3_config = (/* Pin is configured as FC5_SCK */
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
	/* PORT1 PIN3 (coords: G16) is configured as FC5_SCK */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 3U, port1_pin3_config);

	uint32_t port1_pin4_config = (/* Pin is configured as FC5_MISO */
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
	/* PORT1 PIN4 (coords: G17) is configured as FC5_TXD_SCL_MISO_WS */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 4U, port1_pin4_config);

	uint32_t port1_pin5_config = (/* Pin is configured as FC5_MOSI */
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
	/* PORT1 PIN5 (coords: J16) is configured as FC5_RXD_SDA_MOSI_DATA */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 5U, port1_pin5_config);

	uint32_t port1_pin6_config = (/* Pin is configured as FC5_SSEL0 */
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
	/* PORT1 PIN6 (coords: J17) is configured as FC5_CTS_SDA_SSEL0 */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 6U, port1_pin6_config);
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm1), nxp_lpc_i2s, okay)) && \
	(DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm3), nxp_lpc_i2s, okay)) && \
	CONFIG_I2S

	/* Set shared signal set 0 SCK, WS from Transmit I2S - Flexcomm3 */
	SYSCTL1->SHAREDCTRLSET[0] = SYSCTL1_SHAREDCTRLSET_SHAREDSCKSEL(3) |
				SYSCTL1_SHAREDCTRLSET_SHAREDWSSEL(3);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Data in from Transmit I2S - Flexcomm 3 */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_SHAREDDATASEL(3);
	/* Enable Transmit I2S - Flexcomm 3 for Shared Data Out */
	SYSCTL1->SHAREDCTRLSET[0] |= SYSCTL1_SHAREDCTRLSET_FC3DATAOUTEN(1);
#endif

	/* Set Receive I2S - Flexcomm 1 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

	/* Set Transmit I2S - Flexcomm 3 SCK, WS from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[3] = SYSCTL1_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL1_FCCTRLSEL_WSINSEL(1);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Receive I2S - Flexcomm 1 Data in from shared signal set 0 */
	SYSCTL1->FCCTRLSEL[1] |= SYSCTL1_FCCTRLSEL_DATAINSEL(1);
	/* Select Transmit I2S - Flexcomm 3 Data out to shared signal set 0 */
	SYSCTL1->FCCTRLSEL[3] |= SYSCTL1_FCCTRLSEL_DATAOUTSEL(1);
#endif

	/* Pin is configured as FC3_RXD_SDA_MOSI_DATA */
	uint32_t port0_pin23_config = (
		IOPCTL_PIO_FUNC1 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT0 PIN23 (coords: C9) is configured as FC3_RXD_SDA_MOSI_DATA */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 23U, port0_pin23_config);

	/* Pin is configured as FC3_TXD_SCL_MISO_WS */
	uint32_t port0_pin22_config = (
		IOPCTL_PIO_FUNC1 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT0 PIN22 (coords: D8) is configured as FC3_TXD_SCL_MISO_WS */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 22U, port0_pin22_config);

	/* Pin is configured as FC3_SCK */
	uint32_t port0_pin21_config = (
		IOPCTL_PIO_FUNC1 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT0 PIN21 (coords: C7) is configured as FC3_SCK */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 21U, port0_pin21_config);

	/* Pin is configured as FC1_RXD_SDA_MOSI_DATA */
	uint32_t port0_pin9_config = (
		IOPCTL_PIO_FUNC1 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT0 PIN9 (coords: L3) is configured as FC1_RXD_SDA_MOSI_DATA */
	IOPCTL_PinMuxSet(IOPCTL, 0U, 9U, port0_pin9_config);

#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexspi), okay) && CONFIG_FLASH
	uint32_t port1_pin11_config = (/* Pin is configured as FLEXSPI0B_DATA0 */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT1 PIN11 (coords: L2) is configured as FLEXSPI0B_DATA0 */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 11U, port1_pin11_config);

	uint32_t port1_pin12_config = (/* Pin is configured as FLEXSPI0B_DATA1 */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT1 PIN12 (coords: M2) is configured as FLEXSPI0B_DATA1 */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 12U, port1_pin12_config);

	uint32_t port1_pin13_config = (/* Pin is configured as FLEXSPI0B_DATA2 */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT1 PIN13 (coords: N1) is configured as FLEXSPI0B_DATA2 */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 13U, port1_pin13_config);

	uint32_t port1_pin14_config = (/* Pin is configured as FLEXSPI0B_DATA3 */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT1 PIN14 (coords: N2) is configured as FLEXSPI0B_DATA3 */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 14U, port1_pin14_config);

	uint32_t port1_pin29_config = (/* Pin is configured as FLEXSPI0B_SCLK */
		IOPCTL_PIO_FUNC5 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT1 PIN29 (coords: U3) is configured as FLEXSPI0B_SCLK */
	IOPCTL_PinMuxSet(IOPCTL, 1U, 29U, port1_pin29_config);

	uint32_t port2_pin12_config = (/* Pin is configured as PIO2_12 */
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
	/* PORT2 PIN12 (coords: T3) is configured as PIO2_12 */
	IOPCTL_PinMuxSet(IOPCTL, 2U, 12U, port2_pin12_config);

	uint32_t port2_pin17_config = (/* Pin is configured as FLEXSPI0B_DATA4 */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT2 PIN17 (coords: U1) is configured as FLEXSPI0B_DATA4 */
	IOPCTL_PinMuxSet(IOPCTL, 2U, 17U, port2_pin17_config);

	uint32_t port2_pin18_config = (/* Pin is configured as FLEXSPI0B_DATA5 */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT2 PIN18 (coords: R2) is configured as FLEXSPI0B_DATA5 */
	IOPCTL_PinMuxSet(IOPCTL, 2U, 18U, port2_pin18_config);

	uint32_t port2_pin19_config = (/* Pin is configured as FLEXSPI0B_SS0_N */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT2 PIN19 (coords: T2) is configured as FLEXSPI0B_SS0_N */
	IOPCTL_PinMuxSet(IOPCTL, 2U, 19U, port2_pin19_config);

	uint32_t port2_pin22_config = (/* Pin is configured as FLEXSPI0B_DATA6 */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT2 PIN22 (coords: P3) is configured as FLEXSPI0B_DATA6 */
	IOPCTL_PinMuxSet(IOPCTL, 2U, 22U, port2_pin22_config);

	uint32_t port2_pin23_config = (/* Pin is configured as FLEXSPI0B_DATA7 */
		IOPCTL_PIO_FUNC6 |
		/* Disable pull-up / pull-down function */
		IOPCTL_PIO_PUPD_DI |
		/* Enable pull-down function */
		IOPCTL_PIO_PULLDOWN_EN |
		/* Enables input buffer function */
		IOPCTL_PIO_INBUF_EN |
		/* Normal mode */
		IOPCTL_PIO_SLEW_RATE_NORMAL |
		/* Full drive enable */
		IOPCTL_PIO_FULLDRIVE_EN |
		/* Analog mux is disabled */
		IOPCTL_PIO_ANAMUX_DI |
		/* Pseudo Output Drain is disabled */
		IOPCTL_PIO_PSEDRAIN_DI |
		/* Input function is not inverted */
		IOPCTL_PIO_INV_DI);
	/* PORT2 PIN23 (coords: P5) is configured as FLEXSPI0B_DATA7 */
	IOPCTL_PinMuxSet(IOPCTL, 2U, 23U, port2_pin23_config);
#endif

	return 0;
}

/* priority set to CONFIG_PINMUX_INIT_PRIORITY value */
SYS_INIT(mimxrt685_evk_pinmux_init, PRE_KERNEL_1, 45);
