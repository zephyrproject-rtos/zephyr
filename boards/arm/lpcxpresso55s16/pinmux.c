/*
 * Copyright (c) 2020 Henrik Brix Andersen <henrik@brixandersen.dk>
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_common.h>
#include <fsl_iocon.h>
#include <soc.h>

static int lpcxpresso_55s16_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_LPC_PORT0
	__unused struct device *port0 =
		device_get_binding(CONFIG_PINMUX_MCUX_LPC_PORT0_NAME);
#endif

#ifdef CONFIG_PINMUX_MCUX_LPC_PORT1
	__unused struct device *port1 =
		device_get_binding(CONFIG_PINMUX_MCUX_LPC_PORT1_NAME);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(sw0), gpios, pin)
	/* Wakeup button */
	const u32_t sw0_config = (
		IOCON_PIO_FUNC0 |
		IOCON_PIO_INV_DI |
		IOCON_PIO_DIGITAL_EN |
		IOCON_PIO_INPFILT_OFF |
		IOCON_PIO_OPENDRAIN_DI
		);
	pinmux_pin_set(port1, DT_GPIO_PIN(DT_ALIAS(sw0), gpios), sw0_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(sw1), gpios, pin)
	/* USR button */
	const u32_t sw1_config = (
		IOCON_PIO_FUNC0 |
		IOCON_PIO_INV_DI |
		IOCON_PIO_DIGITAL_EN |
		IOCON_PIO_INPFILT_OFF |
		IOCON_PIO_OPENDRAIN_DI
		);
	pinmux_pin_set(port1, DT_GPIO_PIN(DT_ALIAS(sw1), gpios), sw1_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(sw2), gpios, pin)
	/* ISP button */
	const u32_t sw2_config = (
		IOCON_PIO_FUNC0 |
		IOCON_PIO_INV_DI |
		IOCON_PIO_DIGITAL_EN |
		IOCON_PIO_INPFILT_OFF |
		IOCON_PIO_OPENDRAIN_DI
		);
	pinmux_pin_set(port0, DT_GPIO_PIN(DT_ALIAS(sw2), gpios), sw2_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(led0), gpios, pin)
	/* Red LED */
	const u32_t led0_config = (
		IOCON_PIO_FUNC0 |
		IOCON_PIO_INV_DI |
		IOCON_PIO_DIGITAL_EN |
		IOCON_PIO_INPFILT_OFF |
		IOCON_PIO_OPENDRAIN_DI
		);
	pinmux_pin_set(port1, DT_GPIO_PIN(DT_ALIAS(led0), gpios), led0_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(led1), gpios, pin)
	/* Green LED */
	const u32_t led1_config = (
		IOCON_PIO_FUNC0 |
		IOCON_PIO_INV_DI |
		IOCON_PIO_DIGITAL_EN |
		IOCON_PIO_INPFILT_OFF |
		IOCON_PIO_OPENDRAIN_DI
		);
	pinmux_pin_set(port1, DT_GPIO_PIN(DT_ALIAS(led1), gpios), led1_config);
#endif

#if DT_PHA_HAS_CELL(DT_ALIAS(led2), gpios, pin)
	/* Blue LED */
	const u32_t led2_config = (
		IOCON_PIO_FUNC0 |
		IOCON_PIO_INV_DI |
		IOCON_PIO_DIGITAL_EN |
		IOCON_PIO_INPFILT_OFF |
		IOCON_PIO_OPENDRAIN_DI
		);
	pinmux_pin_set(port1, DT_GPIO_PIN(DT_ALIAS(led2), gpios), led2_config);
#endif

#if DT_HAS_NODE_STATUS_OKAY(DT_NODELABEL(flexcomm0)) && \
    DT_NODE_HAS_COMPAT(DT_NODELABEL(flexcomm0), nxp_lpc_usart)
	/* USART0 RX, TX */
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

	return 0;
}

SYS_INIT(lpcxpresso_55s16_pinmux_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_INIT_PRIORITY);
