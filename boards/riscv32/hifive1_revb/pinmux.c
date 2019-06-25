/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int hifive1_revb_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

	struct device *p = device_get_binding(CONFIG_PINMUX_SIFIVE_0_NAME);

#ifdef CONFIG_UART_SIFIVE
#ifdef CONFIG_UART_SIFIVE_PORT_0
	/* UART0 RX */
	pinmux_pin_set(p, 16, SIFIVE_PINMUX_IOF0);
	/* UART0 TX */
	pinmux_pin_set(p, 17, SIFIVE_PINMUX_IOF0);
#endif /* CONFIG_UART_SIFIVE_PORT_0 */
#endif /* CONFIG_UART_SIFIVE */

#ifdef CONFIG_SPI_SIFIVE
	/* SPI1 */
	pinmux_pin_set(p, 2, SIFIVE_PINMUX_IOF0); /* SS0 */
	pinmux_pin_set(p, 3, SIFIVE_PINMUX_IOF0); /* MOSI */
	pinmux_pin_set(p, 4, SIFIVE_PINMUX_IOF0); /* MISO */
	pinmux_pin_set(p, 5, SIFIVE_PINMUX_IOF0); /* SCK */
	pinmux_pin_set(p, 9, SIFIVE_PINMUX_IOF0); /* SS2 */
	pinmux_pin_set(p, 10, SIFIVE_PINMUX_IOF0); /* SS3 */
#endif /* CONFIG_SPI_SIFIVE */

#ifdef CONFIG_I2C_SIFIVE
	/* I2C 0 */
	pinmux_pin_set(p, 12, SIFIVE_PINMUX_IOF0);
	pinmux_pin_set(p, 13, SIFIVE_PINMUX_IOF0);
#endif /* CONFIG_I2C_SIFIVE */

	return 0;
}

SYS_INIT(hifive1_revb_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
