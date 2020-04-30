/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int twr_ke18f_pinmux_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_PORTA
	__unused const struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTB
	__unused const struct device *portb =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTB_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTC
	__unused const struct device *portc =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTD
	__unused const struct device *portd =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTD_NAME);
#endif

#ifdef CONFIG_PINMUX_MCUX_PORTE
	__unused const struct device *porte =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTE_NAME);
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ftm0), nxp_kinetis_ftm_pwm, okay) && CONFIG_PWM
	/* Tri-color LED as PWM */
	pinmux_pin_set(portb, 5, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 15, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 16, PORT_PCR_MUX(kPORT_MuxAlt2));
#else
	/* Tri-color LED as GPIO */
	pinmux_pin_set(portb, 5, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd, 15, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd, 16, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#if DT_NODE_HAS_COMPAT_STATUS(DT_NODELABEL(ftm3), nxp_kinetis_ftm_pwm, okay) && CONFIG_PWM
	/* User LEDs as PWM */
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 11, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 12, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portc, 13, PORT_PCR_MUX(kPORT_MuxAlt2));
#else
	/* User LEDs as GPIO */
	pinmux_pin_set(portc, 10, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 11, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 12, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portc, 13, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

	/* Buttons */
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portd, 6, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpuart0), okay) && CONFIG_SERIAL
	/* UART0 RX, TX */
	pinmux_pin_set(portb, 0, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi0), okay) && CONFIG_SPI
	/* SPI0 SCK, SIN, SOUT */
	pinmux_pin_set(porte, 0, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porte, 1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(porte, 2, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif
#ifdef CONFIG_BOARD_TWR_KE18F_SPI_0_PCS2
	/* SPI0 PCS2 */
	pinmux_pin_set(porte, 6, PORT_PCR_MUX(kPORT_MuxAlt2));
#else
	pinmux_pin_set(porte, 6, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpspi1), okay) && CONFIG_SPI
	/* SPI1 SCK, SIN, SOUT */
	pinmux_pin_set(portd, 0, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd, 2, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif
#ifdef CONFIG_BOARD_TWR_KE18F_SPI_1_PCS0
	/* SPI1 PCS0 */
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAlt3));
#else
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif
#ifdef CONFIG_BOARD_TWR_KE18F_SPI_1_PCS2
	/* SPI1 PCS2 */
	pinmux_pin_set(porta, 16, PORT_PCR_MUX(kPORT_MuxAlt3));
#else
	pinmux_pin_set(porta, 16, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#ifdef CONFIG_BOARD_TWR_KE18F_FLEXIO_CLKOUT
	/* CLKOUT */
	pinmux_pin_set(porte, 10, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c0), okay) && CONFIG_I2C
	/* LPI2C0 SCL, SDA - FXOS8700 */
	pinmux_pin_set(porta, 3, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(porta, 2, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(lpi2c1), okay) && CONFIG_I2C
	/* LPI2C1 SCL, SDA - Elevator connector */
	pinmux_pin_set(portd, 9, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd, 8, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(flexcan0), okay) && CONFIG_CAN
	/* FlexCAN0 RX, TX */
	pinmux_pin_set(porte, 4, PORT_PCR_MUX(kPORT_MuxAlt5));
	pinmux_pin_set(porte, 5, PORT_PCR_MUX(kPORT_MuxAlt5));
#endif

	/* FXOS8700 INT1, INT2, RST */
#if DT_NODE_HAS_PROP(DT_INST(0, nxp_fxos8700), int1_gpios)
	pinmux_pin_set(porta, 14, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif
#if DT_NODE_HAS_PROP(DT_INST(0, nxp_fxos8700), int2_gpios)
	pinmux_pin_set(portc, 17, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif
	pinmux_pin_set(portc, 15, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if DT_NODE_HAS_STATUS(DT_NODELABEL(adc0), okay) && CONFIG_ADC
	/* Thermistor A, B */
	pinmux_pin_set(porta, 0, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
	pinmux_pin_set(porta, 1, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
	/* Potentiometer */
	pinmux_pin_set(portc, 14, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(dac0), okay) && CONFIG_DAC
	pinmux_pin_set(porte, 9, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
#endif

	return 0;
}

SYS_INIT(twr_ke18f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
