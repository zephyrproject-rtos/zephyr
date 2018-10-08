/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <pinmux.h>
#include <board.h>

static int sifive_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

	struct device *p = device_get_binding(CONFIG_PINMUX_SIFIVE_0_NAME);

	/* UART0 RX */
	pinmux_pin_set(p, 16, SIFIVE_PINMUX_IOF0);

	/* UART0 TX */
	pinmux_pin_set(p, 17, SIFIVE_PINMUX_IOF0);

	return 0;
}

SYS_INIT(sifive_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
