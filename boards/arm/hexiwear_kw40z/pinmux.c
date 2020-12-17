/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int hexiwear_kw40z_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(portb), okay)
	__unused const struct device *portb =
		device_get_binding(DT_LABEL(DT_NODELABEL(portb)));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portc), okay)
	__unused const struct device *portc =
		device_get_binding(DT_LABEL(DT_NODELABEL(portc)));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart0), okay) && CONFIG_SERIAL
	/* UART0 RX, TX */
	pinmux_pin_set(portc,  6, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(portc,  7, PORT_PCR_MUX(kPORT_MuxAlt4));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay) && CONFIG_I2C
	/* I2C1 SCL, SDA */
	pinmux_pin_set(portc,  6, PORT_PCR_MUX(kPORT_MuxAlt3)
					| PORT_PCR_PS_MASK);
	pinmux_pin_set(portc,  6, PORT_PCR_MUX(kPORT_MuxAlt3)
					| PORT_PCR_PS_MASK);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc0), okay) && CONFIG_ADC
	/* ADC0_SE1 */
	pinmux_pin_set(portb,  1, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
#endif

	return 0;
}

SYS_INIT(hexiwear_kw40z_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
