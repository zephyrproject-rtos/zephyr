/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int hifive1_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

	struct device *p = device_get_binding(CONFIG_PINMUX_SIFIVE_0_NAME);

	/* UART0 RX */
	pinmux_pin_set(p, 16, SIFIVE_PINMUX_IOF0);

	/* UART0 TX */
	pinmux_pin_set(p, 17, SIFIVE_PINMUX_IOF0);

	/* SPI1 */
	pinmux_pin_set(p, 2, SIFIVE_PINMUX_IOF0);  /* CS0 */
	pinmux_pin_set(p, 3, SIFIVE_PINMUX_IOF0);  /* MOSI */
	pinmux_pin_set(p, 4, SIFIVE_PINMUX_IOF0);  /* MISO */
	pinmux_pin_set(p, 5, SIFIVE_PINMUX_IOF0);  /* SCK */
	pinmux_pin_set(p, 9, SIFIVE_PINMUX_IOF0);  /* CS2 */
	pinmux_pin_set(p, 10, SIFIVE_PINMUX_IOF0); /* CS3 */

#if defined(CONFIG_PWM)

	/* PWM 0 is not enabled because it conflicts with SPI 1 */

	/* PWM 1 */
	/* PWM1_0 is not enabled because the driver cannot use it */
	pinmux_pin_set(p, 19, SIFIVE_PINMUX_IOF1); /* PWM1_1 */
	pinmux_pin_set(p, 21, SIFIVE_PINMUX_IOF1); /* PWM1_2 */
	pinmux_pin_set(p, 22, SIFIVE_PINMUX_IOF1); /* PWM1_3 */

	/* PWM 2 */
	/* PWM2_0 is not enabled because the driver cannot use it */
	pinmux_pin_set(p, 11, SIFIVE_PINMUX_IOF1); /* PWM2_1 */
	pinmux_pin_set(p, 12, SIFIVE_PINMUX_IOF1); /* PWM2_2 */
	pinmux_pin_set(p, 13, SIFIVE_PINMUX_IOF1); /* PWM2_3 */

#endif /* CONFIG_PWM */

	return 0;
}

SYS_INIT(hifive1_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
