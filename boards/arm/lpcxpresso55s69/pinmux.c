/*
 * Copyright (c) 2019,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_common.h>
#include <fsl_iocon.h>
#include <soc.h>

static int lpcxpresso_55s69_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_BOARD_LPCXPRESSO55S69_CPU0
/* Only CPU0 configures GPIO port inputs. */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(pio0), okay)
	const struct device *port0 = DEVICE_DT_GET(DT_NODELABEL(pio0));

	__ASSERT_NO_MSG(device_is_ready(port0));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(pio1), okay)
	const struct device *port1 = DEVICE_DT_GET(DT_NODELABEL(pio1));

	__ASSERT_NO_MSG(device_is_ready(port1));
#endif
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm0), nxp_lpc_usart, okay) && CONFIG_SERIAL
	/* USART0 RX,  TX */
	uint32_t port0_pin29_config = (
			IOCON_PIO_FUNC1 |
			IOCON_PIO_MODE_INACT |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);

	uint32_t port0_pin30_config = (
			IOCON_PIO_FUNC1 |
			IOCON_PIO_MODE_INACT |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);

	pinmux_pin_set(port0, 29, port0_pin29_config);
	pinmux_pin_set(port0, 30, port0_pin30_config);

#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm2), nxp_lpc_usart, okay) && CONFIG_SERIAL
	/* USART2 RX,  TX */
	uint32_t port1_pin24_config = (
			IOCON_PIO_FUNC1 |
			IOCON_PIO_MODE_INACT |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);

	uint32_t port0_pin27_config = (
			IOCON_PIO_FUNC1 |
			IOCON_PIO_MODE_INACT |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);

	pinmux_pin_set(port1, 24, port1_pin24_config);
	pinmux_pin_set(port0, 27, port0_pin27_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(sw0), gpios, pin)
	uint32_t sw0_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);
	pinmux_pin_set(port0, DT_GPIO_PIN(DT_ALIAS(sw0), gpios), sw0_config);
#endif


#if DT_PHA_HAS_CELL(DT_ALIAS(sw1), gpios, pin)
	uint32_t sw1_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);
	pinmux_pin_set(port1, DT_GPIO_PIN(DT_ALIAS(sw1), gpios), sw1_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(sw2), gpios, pin)
	uint32_t sw2_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);
	pinmux_pin_set(port1, DT_GPIO_PIN(DT_ALIAS(sw2), gpios), sw2_config);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm4), nxp_lpc_i2c, okay) && CONFIG_I2C
	/* PORT1 PIN20 is configured as FC4_TXD_SCL_MISO_WS */
	pinmux_pin_set(port1, 20, IOCON_PIO_FUNC5  |
				  IOCON_PIO_MODE_INACT |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_OPENDRAIN_DI);

	/* PORT1 PIN21 is configured as FC4_RXD_SDA_MOSI_DATA */
	pinmux_pin_set(port1, 21, IOCON_PIO_FUNC5  |
				  IOCON_PIO_MODE_INACT |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_OPENDRAIN_DI);
#endif

#ifdef CONFIG_FXOS8700_TRIGGER
	pinmux_pin_set(port1, 19, IOCON_PIO_FUNC0 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_INPFILT_OFF |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_OPENDRAIN_DI);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(hs_lspi), okay) && CONFIG_SPI
	/* PORT0 PIN26 is configured as HS_SPI_MOSI */
	pinmux_pin_set(port0, 26, IOCON_PIO_FUNC9 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_OPENDRAIN_DI);

	uint32_t pio_func = IOCON_PIO_FUNC5; /* Flexcomm controlled CS*/
#if DT_NODE_HAS_PROP(DT_NODELABEL(hs_lspi), cs_gpios)
	pio_func = IOCON_PIO_FUNC0; /* GPIO controlled CS*/
#endif
	/* PORT1 PIN1 is configured as HS_SPI_SSEL1 */
	pinmux_pin_set(port1,  1, pio_func |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_OPENDRAIN_DI);

	/* PORT1 PIN2 is configured as HS_SPI_SCK */
	pinmux_pin_set(port1,  2, IOCON_PIO_FUNC6 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_OPENDRAIN_DI);

	/* PORT1 PIN3 is configured as HS_SPI_MISO */
	pinmux_pin_set(port1,  3, IOCON_PIO_FUNC6 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_OPENDRAIN_DI);
#endif

#if (DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm6), nxp_lpc_i2s, okay)) && \
		(DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(flexcomm7), nxp_lpc_i2s, okay)) && \
		CONFIG_I2S
	CLOCK_EnableClock(kCLOCK_Sysctl);
	/* Set shared signal set 0 SCK, WS from Transmit I2S - Flexcomm 7 */
	SYSCTL->SHAREDCTRLSET[0] = SYSCTL_SHAREDCTRLSET_SHAREDSCKSEL(7) |
				SYSCTL_SHAREDCTRLSET_SHAREDWSSEL(7);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Data in from Transmit I2S - Flexcomm 7 */
	SYSCTL->SHAREDCTRLSET[0] |= SYSCTL_SHAREDCTRLSET_SHAREDDATASEL(7);
	/* Enable Transmit I2S - Flexcomm 7 for Shared Data Out */
	SYSCTL->SHAREDCTRLSET[0] |= SYSCTL_SHAREDCTRLSET_FC7DATAOUTEN(1);
#endif

	/* Set Receive I2S - Flexcomm 6 SCK, WS from shared signal set 0 */
	SYSCTL->FCCTRLSEL[6] = SYSCTL_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL_FCCTRLSEL_WSINSEL(1);

	/* Set Transmit I2S - Flexcomm 7 SCK, WS from shared signal set 0 */
	SYSCTL->FCCTRLSEL[7] = SYSCTL_FCCTRLSEL_SCKINSEL(1) |
				SYSCTL_FCCTRLSEL_WSINSEL(1);

#ifdef CONFIG_I2S_TEST_SEPARATE_DEVICES
	/* Select Receive I2S - Flexcomm 6 Data in from shared signal set 0 */
	SYSCTL->FCCTRLSEL[6] |= SYSCTL_FCCTRLSEL_DATAINSEL(1);
	/* Select Transmit I2S - Flexcomm 7 Data out to shared signal set 0 */
	SYSCTL->FCCTRLSEL[7] |= SYSCTL_FCCTRLSEL_DATAOUTSEL(1);
#endif

	/* Pin is configured as FC7_TXD_SCL_MISO_WS */
	pinmux_pin_set(port0, 19, IOCON_PIO_FUNC7 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_SLEW_FAST |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_OPENDRAIN_DI);

	/* Pin is configured as FC7_RXD_SDA_MOSI_DATA */
	pinmux_pin_set(port0, 20, IOCON_PIO_FUNC7 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_SLEW_FAST |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_OPENDRAIN_DI);

	/* Pin is configured as FC7_SCK */
	pinmux_pin_set(port0, 21, IOCON_PIO_FUNC7 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_SLEW_FAST |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_OPENDRAIN_DI);

	/* Pin is configured as FC6_RXD_SDA_MOSI_DATA */
	pinmux_pin_set(port1, 13, IOCON_PIO_FUNC2 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_SLEW_FAST |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_OPENDRAIN_DI);

#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(sc_timer), nxp_sctimer_pwm, okay) && CONFIG_PWM
	/* Pin is configured as SCT0_OUT2 */
	pinmux_pin_set(port0, 15, IOCON_PIO_FUNC4 |
				  IOCON_PIO_MODE_INACT |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_OPENDRAIN_DI |
				  IOCON_PIO_ASW_EN);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(red_pwm_led), okay)
	/* Pin is configured as SCT0_OUT0 */
	pinmux_pin_set(port1, 4, IOCON_PIO_FUNC4 |
				  IOCON_PIO_MODE_INACT |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_OPENDRAIN_DI);
#endif

#endif

	return 0;
}

SYS_INIT(lpcxpresso_55s69_pinmux_init,  PRE_KERNEL_1,
	 CONFIG_PINMUX_INIT_PRIORITY);
