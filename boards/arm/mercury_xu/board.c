/*
 * Copyright (c) 2020, Antmicro
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/init.h>
#define MIO_PIN_18	0xff180048
#define MIO_PIN_19	0xff18004c
#define MIO_PIN_38	0xff180098
#define MIO_PIN_39	0xff18009c

#define MIO_DEFAULT	0x0
#define MIO_UART0	0xc0

static int mercury_xu_init(void)
{
	/* pinmux settings for uart */
	sys_write32(MIO_UART0, MIO_PIN_38);
	sys_write32(MIO_UART0, MIO_PIN_39);

	/* disable misleading pinmux */
	sys_write32(MIO_DEFAULT, MIO_PIN_18);
	sys_write32(MIO_DEFAULT, MIO_PIN_19);

	return 0;
}

SYS_INIT(mercury_xu_init, PRE_KERNEL_2,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
