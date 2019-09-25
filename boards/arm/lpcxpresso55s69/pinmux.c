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

#ifdef CONFIG_USART_MCUX_LPC_0
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

#ifdef CONFIG_GPIO_MCUX_LPC_PORT0
	const u32_t port0_pin5_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);

	pinmux_pin_set(port0, 5, port0_pin5_config);
#endif

#ifdef CONFIG_GPIO_MCUX_LPC_PORT0
	const u32_t port1_pin18_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);

	pinmux_pin_set(port1, 18, port1_pin18_config);

	const u32_t port1_pin9_config = (
			IOCON_PIO_FUNC0 |
			IOCON_PIO_MODE_PULLUP |
			IOCON_PIO_INV_DI |
			IOCON_PIO_DIGITAL_EN |
			IOCON_PIO_INPFILT_OFF |
			IOCON_PIO_SLEW_STANDARD |
			IOCON_PIO_OPENDRAIN_DI
			);

	pinmux_pin_set(port1, 9, port1_pin9_config);
#endif

	return 0;
}

SYS_INIT(lpcxpresso_55s69_pinmux_init,  PRE_KERNEL_1,
	 CONFIG_PINMUX_INIT_PRIORITY);
