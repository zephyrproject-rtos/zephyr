/*
 * Copyright (c) 2020 Google LLC.
 * Copyright (c) 2021 Szymon Duchniewicz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <soc.h>

static int board_pinmux_init(const struct device *dev)
{
	__unused const struct device *muxa = DEVICE_DT_GET(DT_NODELABEL(pinmux_a));
	__unused const struct device *muxb = DEVICE_DT_GET(DT_NODELABEL(pinmux_b));

	__ASSERT_NO_MSG(device_is_ready(muxa));
	__ASSERT_NO_MSG(device_is_ready(muxb));

	ARG_UNUSED(dev);
	ARG_UNUSED(muxa);
	ARG_UNUSED(muxb);

	if (IS_ENABLED(CONFIG_USB_DC_SAM0)) {
		/* USB DP on PA25, USB DM on PA24 */
		pinmux_pin_set(muxa, 25, PINMUX_FUNC_H);
		pinmux_pin_set(muxa, 24, PINMUX_FUNC_H);
	}

	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
