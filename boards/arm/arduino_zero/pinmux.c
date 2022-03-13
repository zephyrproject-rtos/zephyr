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
	const struct device *muxb = DEVICE_DT_GET(DT_NODELABEL(pinmux_b));

	ARG_UNUSED(dev);

	if (!device_is_ready(muxa)) {
		return -ENXIO;
	}
	if (!device_is_ready(muxb)) {
		return -ENXIO;
	}

#if DT_NODE_HAS_STATUS(DT_NODELABEL(dac0), okay) && defined(CONFIG_DAC_SAM0)
	/* DAC on PA02 */
	pinmux_pin_set(muxa, 2, PINMUX_FUNC_B);
#endif
	return 0;
}

SYS_INIT(board_pinmux_init, PRE_KERNEL_2, CONFIG_PINMUX_INIT_PRIORITY);
