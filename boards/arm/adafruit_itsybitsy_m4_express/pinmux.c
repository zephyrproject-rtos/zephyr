/*
 * Copyright (c) 2020 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	const struct device *muxa = DEVICE_DT_GET(DT_NODELABEL(pinmux_a));
	const struct device *muxb = DEVICE_DT_GET(DT_NODELABEL(pinmux_b));

	ARG_UNUSED(dev);

	if (!device_is_ready(muxa)) {
		return -ENXIO;
	}
	if (!device_is_ready(muxb)) {
		return -ENXIO;
	}

#if defined(CONFIG_PWM_SAM0_TCC)
#if ATMEL_SAM0_DT_TCC_CHECK(0, atmel_sam0_tcc_pwm)
	/* TCC0/WO[2] on PA22 (LED) */
	pinmux_pin_set(muxa, 22, PINMUX_FUNC_G);
#endif
#if ATMEL_SAM0_DT_TCC_CHECK(1, atmel_sam0_tcc_pwm)
	/* TCC1/WO[2] on PA18 (D7) */
	pinmux_pin_set(muxa, 18, PINMUX_FUNC_F);
	/* TCC1/WO[3] on PA19 (D9) */
	pinmux_pin_set(muxa, 19, PINMUX_FUNC_F);
#endif
#endif

	if (IS_ENABLED(CONFIG_USB_DC_SAM0)) {
		/* USB DP on PA25, USB DM on PA24 */
		pinmux_pin_set(muxa, 25, PINMUX_FUNC_H);
		pinmux_pin_set(muxa, 24, PINMUX_FUNC_H);
	}

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_2, CONFIG_PINMUX_INIT_PRIORITY);
