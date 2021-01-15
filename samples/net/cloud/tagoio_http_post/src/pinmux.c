/*
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>

#if defined(CONFIG_BOARD_FRDM_K64F)
#include <fsl_port.h>
#endif

static int tagoio_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if defined(CONFIG_BOARD_FRDM_K64F)
	const struct device *portc = DEVICE_DT_GET(DT_NODELABEL(portc));

	__ASSERT_NO_MSG(device_is_ready(portc));

	pinmux_pin_set(portc, 2, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 3, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

	return 0;
}

SYS_INIT(tagoio_pinmux_init, POST_KERNEL, CONFIG_PINMUX_INIT_PRIORITY);
