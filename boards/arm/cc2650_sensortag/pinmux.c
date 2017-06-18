/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Board-specific pin multiplexing for Texas Instruments'
 * SensorTag device.
 *
 * For now, this only setups a default configuration
 * at initialization (not a real pinmux driver).
 */

#include <toolchain/gcc.h>
#include <init.h>
#include <pinmux.h>
#include <soc.h>

#include "board.h"


static int sensortag_pinmux_init(struct device *dev)
{
	dev = device_get_binding(CONFIG_PINMUX_NAME);

	/* DIO10 is LED 1 */
	pinmux_pin_set(dev, SENSORTAG_LED1, CC2650_IOC_GPIO);
	pinmux_pin_input_enable(dev, SENSORTAG_LED1, PINMUX_OUTPUT_ENABLED);

	/* DIO15 is LED 2 */
	pinmux_pin_set(dev, SENSORTAG_LED2, CC2650_IOC_GPIO);
	pinmux_pin_input_enable(dev, SENSORTAG_LED2, PINMUX_OUTPUT_ENABLED);

	/* UART RX */
	pinmux_pin_set(dev, SENSORTAG_UART_RX, CC2650_IOC_MCU_UART0_RX);
	pinmux_pin_input_enable(dev, SENSORTAG_UART_RX, PINMUX_INPUT_ENABLED);

	/* UART TX */
	pinmux_pin_set(dev, SENSORTAG_UART_TX, CC2650_IOC_MCU_UART0_TX);
	pinmux_pin_input_enable(dev, SENSORTAG_UART_TX, PINMUX_OUTPUT_ENABLED);

	return 0;
}

SYS_INIT(sensortag_pinmux_init, PRE_KERNEL_1,
	 CONFIG_PINMUX_INIT_PRIORITY);
