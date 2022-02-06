/*
 * Copyright (c) 2018 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	const struct device *muxa = DEVICE_DT_GET(DT_NODELABEL(pinmux_a));

	ARG_UNUSED(dev);

	if (!device_is_ready(muxa)) {
		return -ENXIO;
	}

#if ATMEL_SAM0_DT_TCC_CHECK(0, atmel_sam0_tcc_pwm) && \
	defined(CONFIG_PWM_SAM0_TCC)
	/* LED0 on PA10/TCC0/WO[2] */
	pinmux_pin_set(muxa, 10, PINMUX_FUNC_F);
#endif

#ifdef CONFIG_USB_DC_SAM0
	/* USB DP on PA25, USB DM on PA24 */
	pinmux_pin_set(muxa, 25, PINMUX_FUNC_G);
	pinmux_pin_set(muxa, 24, PINMUX_FUNC_G);
#endif

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_2, CONFIG_PINMUX_INIT_PRIORITY);
