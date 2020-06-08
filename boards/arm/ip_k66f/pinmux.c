/*
 * Copyright (c) 2020 DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int ip_k66f_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_PORTA
	struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);
#endif

	/* Red0, Red2 LEDs */
	pinmux_pin_set(porta, 8, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, 10, PORT_PCR_MUX(kPORT_MuxAsGpio));

	return 0;
}

SYS_INIT(ip_k66f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
