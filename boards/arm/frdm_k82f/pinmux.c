/*
 * Copyright (c) 2019 SEAL AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int frdm_k82f_pinmux_init(const struct device *dev)
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
#if DT_NODE_HAS_STATUS(DT_NODELABEL(portd), okay)
	__unused const struct device *portd =
		DEVICE_DT_GET(DT_NODELABEL(portd));
	__ASSERT_NO_MSG(device_is_ready(portd));
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(porte), okay)
	__unused const struct device *porte =
		DEVICE_DT_GET(DT_NODELABEL(porte));
	__ASSERT_NO_MSG(device_is_ready(porte));
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ftm3), nxp_kinetis_ftm_pwm, okay) && CONFIG_PWM
	/* Red, green, blue LEDs as PWM channels */
	pinmux_pin_set(portc,  8, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc,  9, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAlt4));
#else
	/* Red, green, blue LEDs as GPIOs */
	pinmux_pin_set(portc,  8, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc,  9, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

	/* Buttons */
	pinmux_pin_set(porta, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 6, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* FXOS8700 INT1 */
	pinmux_pin_set(portc, 13, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c3), okay) && CONFIG_I2C
	/* I2C3 SDA, SCL */
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAlt4)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAlt4)
					| PORT_PCR_ODE_MASK);
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay) && CONFIG_SPI
	/* SPI1 SCK, SOUT, SIN, PCS0 */
	pinmux_pin_set(porte, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porte, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porte, 4, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porte, 5, PORT_PCR_MUX(kPORT_MuxAlt2));
	/* SPI1 NOR RESET, WP */
	pinmux_pin_set(porte, 0, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 3, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart4), okay) && CONFIG_SERIAL
	/* LPUART4 RX, TX */
	pinmux_pin_set(portc, 14, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 15, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc0), okay) && CONFIG_ADC
	/* ADC0_SE15 */
	pinmux_pin_set(portc,  1, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
#endif

	return 0;
}

SYS_INIT(frdm_k82f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
