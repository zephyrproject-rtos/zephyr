/*
 * Copyright (c) 2019,  NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_common.h>
#include <fsl_iocon.h>
#include <soc.h>

static int lpcxpresso_55s69_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_LPC_PORT0
	struct device *port0 =
		device_get_binding(CONFIG_PINMUX_MCUX_LPC_PORT0_NAME);
#endif

#ifdef CONFIG_PINMUX_MCUX_LPC_PORT1
	struct device *port1 =
		device_get_binding(CONFIG_PINMUX_MCUX_LPC_PORT1_NAME);
#endif

#ifdef CONFIG_UART_MCUX_FLEXCOMM_0
	/* USART0 RX,  TX */
	const u32_t port0_pin29_config = (
			IOCON_PIO_FUNC1 |
			IOCON_PIO_MODE_INACT |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);

	const u32_t port0_pin30_config = (
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

#ifdef DT_GPIO_KEYS_SW0_GPIOS_CONTROLLER
	const u32_t sw0_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);
	pinmux_pin_set(port0, DT_ALIAS_SW0_GPIOS_PIN, sw0_config);
#endif

#ifdef DT_GPIO_KEYS_SW1_GPIOS_CONTROLLER
	const u32_t sw1_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);
	pinmux_pin_set(port1, DT_ALIAS_SW1_GPIOS_PIN, sw1_config);
#endif

#ifdef DT_GPIO_KEYS_SW2_GPIOS_CONTROLLER
	const u32_t sw2_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);
	pinmux_pin_set(port1, DT_ALIAS_SW2_GPIOS_PIN, sw2_config);
#endif

#ifdef CONFIG_I2C_4
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
#endif /* CONFIG_I2C_4 */

#ifdef CONFIG_FXOS8700_TRIGGER
	pinmux_pin_set(port1, 19, IOCON_PIO_FUNC0 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_INPFILT_OFF |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_OPENDRAIN_DI);
#endif

#ifdef CONFIG_SPI_8
	/* PORT0 PIN26 is configured as HS_SPI_MOSI */
	pinmux_pin_set(port0, 26, IOCON_PIO_FUNC9 |
				  IOCON_PIO_MODE_PULLUP |
				  IOCON_PIO_INV_DI |
				  IOCON_PIO_DIGITAL_EN |
				  IOCON_PIO_SLEW_STANDARD |
				  IOCON_PIO_OPENDRAIN_DI);

	/* PORT1 PIN1 is configured as HS_SPI_SSEL1 */
	pinmux_pin_set(port1,  1, IOCON_PIO_FUNC5 |
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
#endif /* CONFIG_SPI_8 */

	return 0;
}

SYS_INIT(lpcxpresso_55s69_pinmux_init,  PRE_KERNEL_1,
	 CONFIG_PINMUX_INIT_PRIORITY);
