/*
 * Copyright 2017, 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int frdm_kw41z_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(porta), okay)
	__unused const struct device *porta =
		DEVICE_DT_GET(DT_NODELABEL(porta));
	__ASSERT_NO_MSG(device_is_ready(porta));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portb), okay)
	__unused const struct device *portb =
		DEVICE_DT_GET(DT_NODELABEL(portb));
	__ASSERT_NO_MSG(device_is_ready(portb));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portc), okay)
	__unused const struct device *portc =
		DEVICE_DT_GET(DT_NODELABEL(portc));
	__ASSERT_NO_MSG(device_is_ready(portc));
#endif

	/* Red, green, blue LEDs. Note the red LED and accel INT1 are both
	 * wired to PTC1.
	 */
#if defined(CONFIG_PWM) && DT_NODE_HAS_STATUS(DT_NODELABEL(tpm0), okay)
	pinmux_pin_set(portc,  1, PORT_PCR_MUX(kPORT_MuxAlt5));
#endif

#if defined(CONFIG_PWM) && DT_NODE_HAS_STATUS(DT_NODELABEL(tpm2), okay)
	pinmux_pin_set(porta, 19, PORT_PCR_MUX(kPORT_MuxAlt5));
	pinmux_pin_set(porta, 18, PORT_PCR_MUX(kPORT_MuxAlt5));
#endif

	return 0;
}

SYS_INIT(frdm_kw41z_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
