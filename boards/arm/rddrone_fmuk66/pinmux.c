/*
 * Copyright 2021 Electromaticus LLC, 2021 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int rddrone_fmuk66_pinmux_init(const struct device *dev)
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



#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart0), okay) && CONFIG_SERIAL
	/* LPUART0 RX, TX */
	pinmux_pin_set(portd,  8, PORT_PCR_MUX(kPORT_MuxAlt5));
	pinmux_pin_set(portd,  9, PORT_PCR_MUX(kPORT_MuxAlt5));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart0), okay) && CONFIG_SERIAL
	/* UART0 RX, TX */
	pinmux_pin_set(porta,  1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porta,  2, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart1), okay) && CONFIG_SERIAL
	/* UART1 RX, TX */
	pinmux_pin_set(portc,  3, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc,  4, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart2), okay) && CONFIG_SERIAL
	/* UART1 RX, TX */
	pinmux_pin_set(portd,  2, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd,  3, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(uart4), okay) && CONFIG_SERIAL
	/* UART1 RTS, CTS, RX, TX */
	pinmux_pin_set(porte, 27, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 13, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 14, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 15, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcan0), okay) && CONFIG_CAN
	/* CAN0 TX, RX */
	pinmux_pin_set(portb, 18, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 19, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcan1), okay) && CONFIG_CAN
	/* CAN1 TX, RX */
	pinmux_pin_set(portc, 17, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 16, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay) && CONFIG_I2C
	/* I2C0 SCL, SDA */
	pinmux_pin_set(porte, 24, PORT_PCR_MUX(kPORT_MuxAlt5));
	pinmux_pin_set(porte, 25, PORT_PCR_MUX(kPORT_MuxAlt5));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay) && CONFIG_I2C
	/* I2C0 SCL, SDA */
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 11, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi0), okay) && CONFIG_SPI
	/* SPI0 CS2, SCK, SIN, SOUT */
	pinmux_pin_set(portc, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 5, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 6, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 7, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi1), okay) && CONFIG_SPI
	/* SPI1 CS0, CS1, SCK, SIN, SOUT */
	pinmux_pin_set(portb, 10, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb,  9, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 11, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 16, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 17, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(spi2), okay) && CONFIG_SPI
	/* SPI2 CS0, SCK, SIN, SOUT */
	pinmux_pin_set(portb, 20, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 21, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 22, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 23, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif


#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ftm0), nxp_kinetis_ftm_pwm, okay) && CONFIG_PWM
	/* FlexTimer0 Channels for FMU (servo control) */
	/* fmu ch1 */
	pinmux_pin_set(portc,  1, PORT_PCR_MUX(kPORT_MuxAlt4));
	/* fmu ch2 */
	pinmux_pin_set(porta,  4, PORT_PCR_MUX(kPORT_MuxAlt3));
	/* fmu ch3 */
	pinmux_pin_set(portd,  4, PORT_PCR_MUX(kPORT_MuxAlt4));
	/* fmu ch4 */
	pinmux_pin_set(portd,  5, PORT_PCR_MUX(kPORT_MuxAlt4));
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ftm3), nxp_kinetis_ftm_pwm, okay) && CONFIG_PWM
	/* FlexTimer3 Channels for PWM controlled RGB light and FMU (servo control) */
	/* red */
	pinmux_pin_set(portd,  1, PORT_PCR_MUX(kPORT_MuxAlt4));
	/* green */
	pinmux_pin_set(portc,  9, PORT_PCR_MUX(kPORT_MuxAlt3));
	/* blue */
	pinmux_pin_set(portc,  8, PORT_PCR_MUX(kPORT_MuxAlt3));
	/* fmu ch5 */
	pinmux_pin_set(porte, 11, PORT_PCR_MUX(kPORT_MuxAlt6));
	/* fmu ch6 */
	pinmux_pin_set(porte, 12, PORT_PCR_MUX(kPORT_MuxAlt6));
#endif

	return 0;
}

SYS_INIT(rddrone_fmuk66_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
