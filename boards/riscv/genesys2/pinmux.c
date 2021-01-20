/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int genesys2_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	//const struct device *p = device_get_binding(CONFIG_PINMUX_ARIANE_NAME);

#ifdef CONFIG_UART_ARIANE
	/* UART0 RX */
	/* UART0 TX */
#endif

#ifdef CONFIG_SPI_ARIANE
	/* SPI1 */
#endif /* CONFIG_SPI_SIFIVE */

#ifdef CONFIG_I2C_ARIANE
	/* I2C 0 */
#endif

	return 0;
}

SYS_INIT(genesys2_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);

