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

static int hexiwear_k64_pinmux_init(struct device *dev)
{
	ARG_UNUSED(dev);

	pinmux_ksdk_init();

	/* Red, green, blue LEDs */
	pinmux_ksdk_set(PORTC,  8, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_ksdk_set(PORTC,  9, PORT_PCR_MUX(kPORT_MuxAsGpio));
	pinmux_ksdk_set(PORTD,  0, PORT_PCR_MUX(kPORT_MuxAsGpio));

#if CONFIG_I2C_1
	/* I2C1 SCL, SDA - accel/mag, gyro, pressure */
	pinmux_ksdk_set(PORTC, 10, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_ODE_MASK);
	pinmux_ksdk_set(PORTC, 11, PORT_PCR_MUX(kPORT_MuxAlt5)
					| PORT_PCR_ODE_MASK);
#endif
	/* FXOS8700 INT1 */
	pinmux_ksdk_set(PORTC,  1, PORT_PCR_MUX(kPORT_MuxAsGpio));

#ifdef CONFIG_UART_K20_PORT_4
	/* UART4 RX, TX - BLE */
	pinmux_ksdk_set(PORTE, 24, PORT_PCR_MUX(kPORT_MuxAlt3));
	pinmux_ksdk_set(PORTE, 25, PORT_PCR_MUX(kPORT_MuxAlt3));
#endif

	return 0;
}

SYS_INIT(hexiwear_k64_pinmux_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_INIT_PRIORITY);
