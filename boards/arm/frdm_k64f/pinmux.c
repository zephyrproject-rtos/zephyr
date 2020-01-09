/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <init.h>
#include <drivers/pinmux.h>
#include <fsl_port.h>

static int frdm_k64f_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_MCUX_PORTA
	struct device *porta =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTA_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTB
	struct device *portb =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTB_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTC
	struct device *portc =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTD
	struct device *portd =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTD_NAME);
#endif
#ifdef CONFIG_PINMUX_MCUX_PORTE
	struct device *porte =
		device_get_binding(CONFIG_PINMUX_MCUX_PORTE_NAME);
#endif

#ifdef CONFIG_UART_MCUX_0
	/* UART0 RX, TX */
	pinmux_pin_set(portb, 16, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portb, 17, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#ifdef CONFIG_UART_MCUX_2
	/* UART2 RX, TX */
	pinmux_pin_set(portd, 0, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd, 1, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd, 2, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portd, 3, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#ifdef CONFIG_UART_MCUX_3
	/* UART3 RX, TX */
	pinmux_pin_set(portc, 16, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 17, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif
	/* SW2 / FXOS8700 INT1 */
	pinmux_pin_set(portc,  6, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* FXOS8700 INT2 */
	pinmux_pin_set(portc, 13, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* SW3 */
	pinmux_pin_set(porta,  4, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Red, green, blue LEDs */
	pinmux_pin_set(portb, 22, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 26, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, 21, PORT_PCR_MUX(kPORT_MuxAsGpio));

#ifdef CONFIG_MODEM_WNCM14A2A
	/* WNC-M14A2A Modem POWER_ON */
	pinmux_pin_set(portb,  9, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* WNC-M14A2A Shield PMOD_D1 */
	pinmux_pin_set(portb, 10, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* WNC-M14A2A Shield PMOD_D2 */
	pinmux_pin_set(portb, 11, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* WNC-M14A2A Modem WWAN_STATE */
	pinmux_pin_set(portb, 23, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* WNC-M14A2A Modem WAKEUP_ENABLE */
	pinmux_pin_set(portc,  2, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* WNC-M14A2A Modem HTS221_DRDY */
	pinmux_pin_set(portc,  3, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* WNC-M14A2A Shield LEVEL_TRANSFORM_ENABLE */
	pinmux_pin_set(portc,  4, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* WNC-M14A2A Modem RESET */
	pinmux_pin_set(portc, 12, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* WNC-M14A2A Modem BOOT_MODE_SELECT */
	pinmux_pin_set(portc, 17, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#ifdef CONFIG_MODEM_UBLOX_SARA_R4
	/* Modem RESET */
	pinmux_pin_set(portc,  2, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* Modem POWER_ON */
	pinmux_pin_set(porta,  2, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#ifdef CONFIG_IEEE802154_MCR20A
	/* FRDM-MCR20A Reset (D5) */
	pinmux_pin_set(porta,  2, PORT_PCR_MUX(kPORT_MuxAsGpio));
	/* FRDM-MCR20A IRQ_B (D2) */
	pinmux_pin_set(portb,  9, PORT_PCR_MUX(kPORT_MuxAsGpio));
#endif

#ifdef CONFIG_SPI_0
	/* SPI0 CS0, SCK, SOUT, SIN */
	pinmux_pin_set(portd,  0, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd,  1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd,  2, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portd,  3, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if CONFIG_I2C_0
	/* I2C0 SCL, SDA */
	pinmux_pin_set(porte, 24, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_ODE_MASK);
	pinmux_pin_set(porte, 25, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_ODE_MASK);
#endif

#if CONFIG_ADC_1
	/* ADC1_SE14 */
	pinmux_pin_set(portb, 10, PORT_PCR_MUX(kPORT_PinDisabledOrAnalog));
#endif

#ifdef CONFIG_PWM_3
	pinmux_pin_set(portc,  8, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc,  9, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

#if CONFIG_ETH_MCUX_0
	pinmux_pin_set(porta,  5, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(porta, 12, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(porta, 13, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(porta, 14, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(porta, 15, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(porta, 16, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(porta, 17, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(porta, 28, PORT_PCR_MUX(kPORT_MuxAlt4));

	pinmux_pin_set(portb,  0, PORT_PCR_MUX(kPORT_MuxAlt4)
		| PORT_PCR_ODE_MASK | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);

	pinmux_pin_set(portb,  1, PORT_PCR_MUX(kPORT_MuxAlt4));

	pinmux_pin_set(portc, 16, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(portc, 17, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(portc, 18, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_pin_set(portc, 19, PORT_PCR_MUX(kPORT_MuxAlt4));
#endif

#if CONFIG_CAN_0
	/* FlexCAN0 RX, TX */
	pinmux_pin_set(portb, 18, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_pin_set(portb, 19, PORT_PCR_MUX(kPORT_MuxAlt2) |
		       PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);
#endif

	return 0;
}

SYS_INIT(frdm_k64f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
