/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <init.h>
#include <pinmux.h>
#include <fsl_port.h>

static int frdm_k64f_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_PINMUX_KSDK_PORTA
	struct device *porta =
		device_get_binding(CONFIG_PINMUX_KSDK_PORTA_NAME);
#endif
#ifdef CONFIG_PINMUX_KSDK_PORTB
	struct device *portb =
		device_get_binding(CONFIG_PINMUX_KSDK_PORTB_NAME);
#endif
#ifdef CONFIG_PINMUX_KSDK_PORTC
	struct device *portc =
		device_get_binding(CONFIG_PINMUX_KSDK_PORTC_NAME);
#endif
#ifdef CONFIG_PINMUX_KSDK_PORTD
	struct device *portd =
		device_get_binding(CONFIG_PINMUX_KSDK_PORTD_NAME);
#endif
#ifdef CONFIG_PINMUX_KSDK_PORTE
	struct device *porte =
		device_get_binding(CONFIG_PINMUX_KSDK_PORTE_NAME);
#endif

#ifdef CONFIG_UART_K20_PORT_3
	/* UART3 RX, TX */
	pinmux_pin_set(portc, 16, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_pin_set(portc, 17, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif
	/* SW2 / FXOS8700 INT1 */
	pinmux_pin_set(portc,  6, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* SW3 */
	pinmux_pin_set(porta,  4, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Red, green, blue LEDs */
	pinmux_pin_set(portb, 22, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(porte, 26, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_pin_set(portb, 21, PORT_PCR_MUX(kPORT_MuxAsGpio));

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

#if CONFIG_ETH_KSDK_0
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

	return 0;
}

SYS_INIT(frdm_k64f_pinmux_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);
