/*
 * Copyright (c) 2018 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int frdm_k22f_pinmux_init(const struct device *dev)
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

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) && CONFIG_SERIAL
#error "No UART0 is used"
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) && CONFIG_SERIAL
	/* UART1 RX, TX */
	pinmux_pin_set(porte, 0, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(porte, 1, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) && CONFIG_SERIAL
	/* UART2 RX, TX */
	pinmux_pin_set(portd, 2, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

	/* SW2 */
	pinmux_pin_set(portc, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* SW3 */
	pinmux_pin_set(portb, 17, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* FXOS8700 INT1 */
	pinmux_pin_set(portd, 0, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* FXOS8700 INT2 */
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ftm0), nxp_kinetis_ftm_pwm, okay) && CONFIG_PWM
	/* Red, green, blue LEDs as PWM channels*/
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd, 5, PORT_PCR_MUX(kPORT_MuxAlt4));
#else
	/* Red, green, blue LEDs as GPIO channels*/
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd, 5, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi0), okay) && CONFIG_SPI
	/* SPI0 CS0, SCK, SOUT, SIN */
	pinmux_pin_set(portd, 4, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 11, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay) && CONFIG_I2C
	/* I2C0 SCL, SDA */
	pinmux_pin_set(portb, 2, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(portb, 3, PORT_PCR_MUX(kPORT_MuxAlt2)
					| PORT_PCR_ODE_MASK);
#endif

#if CONFIG_SHIELD_FRDM_STBC_AGM01
	/* FXOS8700 INT1 */
	pinmux_pin_set(portb, 16, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* FXOS8700 INT2 */
	pinmux_pin_set(porta, 4, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* FXAS21002 INT1 */
	pinmux_pin_set(portb, 18, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* FXAS21002 INT2 */
	pinmux_pin_set(portb, 19, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

	return 0;
}

SYS_INIT(frdm_k22f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
