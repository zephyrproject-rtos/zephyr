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
#include <pinmux/pinmux_ksdk.h>

static int frdm_k64f_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

	pinmux_ksdk_init();

#ifdef CONFIG_UART_K20_PORT_3
	/* UART3 RX, TX */
	pinmux_ksdk_set(PORTC, 16, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_ksdk_set(PORTC, 17, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif
	/* SW2 / FXOS8700 INT1 */
	pinmux_ksdk_set(PORTC,  6, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* SW3 */
	pinmux_ksdk_set(PORTA,  4, PORT_PCR_MUX(kPORT_MuxAsGpio));

	/* Red, green, blue LEDs */
	pinmux_ksdk_set(PORTB, 22, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_ksdk_set(PORTE, 26, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_ksdk_set(PORTB, 21, PORT_PCR_MUX(kPORT_MuxAsGpio));

#ifdef CONFIG_SPI_0
	/* SPI0 CS0, SCK, SOUT, SIN */
	pinmux_ksdk_set(PORTD,  0, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_ksdk_set(PORTD,  1, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_ksdk_set(PORTD,  2, PORT_PCR_MUX(kPORT_MuxAlt2));
	pinmux_ksdk_set(PORTD,  3, PORT_PCR_MUX(kPORT_MuxAlt2));
#endif

#if CONFIG_I2C_0
	/* I2C0 SCL, SDA */
	pinmux_ksdk_set(PORTE, 24, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_ODE_MASK);
	pinmux_ksdk_set(PORTE, 25, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_ODE_MASK);
#endif

#if CONFIG_ETH_KSDK_0
	pinmux_ksdk_set(PORTA,  5, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTA, 12, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTA, 13, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTA, 14, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTA, 15, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTA, 16, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTA, 17, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTA, 28, PORT_PCR_MUX(kPORT_MuxAlt4));

	pinmux_ksdk_set(PORTB,  0, PORT_PCR_MUX(kPORT_MuxAlt4)
		| PORT_PCR_ODE_MASK | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK);

	pinmux_ksdk_set(PORTB,  1, PORT_PCR_MUX(kPORT_MuxAlt4));

	pinmux_ksdk_set(PORTC, 16, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTC, 17, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTC, 18, PORT_PCR_MUX(kPORT_MuxAlt4));
	pinmux_ksdk_set(PORTC, 19, PORT_PCR_MUX(kPORT_MuxAlt4));
#endif

	return 0;
}

SYS_INIT(frdm_k64f_pinmux_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_INIT_PRIORITY);
