/*
 * Copyright (c) 2015 Intel Corporation.
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

/**
 * @file Board config file
 */

#include <device.h>
#include <init.h>

#include <nanokernel.h>

#include "soc.h"

#ifdef CONFIG_UART_K20
#include <uart.h>
#include <drivers/k20_pcr.h>
#include <drivers/k20_sim.h>
#include <console/uart_console.h>
#include <serial/uart_k20_priv.h>
#endif /* CONFIG_UART_K20 */

#ifdef CONFIG_PINMUX
#include <pinmux.h>
#include <pinmux/pinmux.h>
#include <pinmux/pinmux_k64.h>
#endif /* CONFIG_PINMUX */


/*
 * UART configuration
 */

#ifdef CONFIG_UART_K20

#if defined(CONFIG_UART_CONSOLE)
#if defined(CONFIG_PRINTK) || defined(CONFIG_STDOUT_CONSOLE)

/**
 * @brief Initialize K20 serial port as console
 *
 * Initialize the UART port for console I/O.
 *
 * @param dev The UART device struct
 *
 * @return DEV_OK if successful, otherwise failed.
 */
static int uart_k20_console_init(struct device *dev)
{
	uint32_t port;
	uint32_t rxPin;
	uint32_t txPin;
	union K20_PCR pcr = {0}; /* Pin Control Register */

	/* Port/pin ctrl module */
	volatile struct K20_PORT_PCR *port_pcr_p =
		(volatile struct K20_PORT_PCR *)PERIPH_ADDR_BASE_PCR;

	/* UART0 Rx and Tx pin assignments */
	port = CONFIG_UART_CONSOLE_PORT;
	rxPin = CONFIG_UART_CONSOLE_PORT_RX_PIN;
	txPin = CONFIG_UART_CONSOLE_PORT_TX_PIN;

	/* Enable the UART Rx and Tx Pins */
	pcr.field.mux = CONFIG_UART_CONSOLE_PORT_MUX_FUNC;

	port_pcr_p->port[port].pcr[rxPin] = pcr;
	port_pcr_p->port[port].pcr[txPin] = pcr;

	return DEV_OK;
}

SYS_INIT(uart_k20_console_init, PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif
#endif /* CONFIG_UART_CONSOLE */

static int uart_k20_init(struct device *dev)
{
	volatile struct K20_SIM *sim = /* sys integ. ctl */
		(volatile struct K20_SIM *)PERIPH_ADDR_BASE_SIM;
	SIM_SCGC4_t scgc4;

	ARG_UNUSED(dev);

	/* Although it is possible to modify the bits through
	 * *sim directly, the following code saves about 20 bytes
	 * of ROM space, compared to direct modification.
	 */
	scgc4.value = sim->scgc4.value;

#ifdef CONFIG_UART_K20_PORT_0
	scgc4.field.uart0_clk_en = 1;
#endif

#ifdef CONFIG_UART_K20_PORT_1
	scgc4.field.uart1_clk_en = 1;
#endif

#ifdef CONFIG_UART_K20_PORT_2
	scgc4.field.uart2_clk_en = 1;
#endif

#ifdef CONFIG_UART_K20_PORT_3
	scgc4.field.uart3_clk_en = 1;
#endif

	sim->scgc4.value = scgc4.value;

#ifdef CONFIG_UART_K20_PORT_4
	sim->scgc1.field.uart4_clk_en = 1;
#endif

	return DEV_OK;
}

DEVICE_INIT(_uart_k20_init, "", uart_k20_init,
				NULL, NULL,
				PRIMARY, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_UART_K20 */

/*
 * I/O pin configuration
 */

#ifdef CONFIG_PINMUX

/*
 * Number of default pin settings, used for Arduino Rev 3 pinout.
 *
 * NOTE: The FRDM-K64F board routes the PTA0/1/2 pins for JTAG/SWD signals that
 * are used for the OpenSDAv2 debug interface.  These pins are also routed to
 * the Arduino header pins as D8, D3 and D5, respectively.
 * Since the K64 MCU configures these pins for JTAG/SWD signaling at reset,
 * they should only be re-configured if the debug interface is not used.
 */

#ifndef CONFIG_PRESERVE_JTAG_IO_PINS
#define NUM_DFLT_PINS_SET   22
#else
#define NUM_DFLT_PINS_SET   (22 - 3)
#endif

/*
 * Alter this table to change the default Arduino pin settings on the Freescale
 * FRDM-K64F boards.  Specifically, change the PINMUX_* values to represent
 * the functionality desired.
 */
struct pin_config mux_config[NUM_DFLT_PINS_SET] = {
	/* pin,      		selected mode */
	{ K64_PIN_PTC16, (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
	{ K64_PIN_PTC17, (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
	{ K64_PIN_PTB9,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
#ifndef CONFIG_PRESERVE_JTAG_IO_PINS
	{ K64_PIN_PTA1,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
#endif
	{ K64_PIN_PTB23, (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
#ifndef CONFIG_PRESERVE_JTAG_IO_PINS
	{ K64_PIN_PTA2,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
#endif
	{ K64_PIN_PTC2,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
	{ K64_PIN_PTC3,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
#ifndef CONFIG_PRESERVE_JTAG_IO_PINS
	{ K64_PIN_PTA0,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
#endif
	{ K64_PIN_PTC4,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
	{ K64_PIN_PTD0,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
	{ K64_PIN_PTD2,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
	{ K64_PIN_PTD3,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
	{ K64_PIN_PTD1,  (K64_PINMUX_FUNC_GPIO | K64_PINMUX_GPIO_DIR_INPUT) },
	/* I2C0_SDA */
	{ K64_PIN_PTE25, (K64_PINMUX_ALT_5 | K64_PINMUX_OPEN_DRN_ENABLE) },
	/* I2C0_SCL */
	{ K64_PIN_PTE24, (K64_PINMUX_ALT_5 | K64_PINMUX_OPEN_DRN_ENABLE) },
	{ K64_PIN_PTB2,  K64_PINMUX_FUNC_ANALOG },  /* ADC0_SE12/Analog In 0 */
	{ K64_PIN_PTB3,  K64_PINMUX_FUNC_ANALOG },  /* ADC0_SE13/Analog In 1 */
	{ K64_PIN_PTB10, K64_PINMUX_FUNC_ANALOG },  /* ADC1_SE14/Analog In 2 */
	{ K64_PIN_PTB11, K64_PINMUX_FUNC_ANALOG },  /* ADC1_SE15/Analog In 3 */
	{ K64_PIN_PTC11, K64_PINMUX_FUNC_ANALOG },  /* ADC1_SE7b/Analog In 4 */
	{ K64_PIN_PTC10, K64_PINMUX_FUNC_ANALOG },  /* ADC1_SE6b/Analog In 5 */
};


int fsl_frdm_k64f_pin_init(struct device *arg)
{
	ARG_UNUSED(arg);
	struct device *pmux;
	int i;

	pmux = device_get_binding(PINMUX_NAME);

	if (!pmux) {
		return DEV_INVALID_CONF;
	}

	/* configure the pins from the default mapping above */

	for (i = 0; i < NUM_DFLT_PINS_SET; i++) {
		pinmux_pin_set(pmux, mux_config[i].pin_num, mux_config[i].mode);
	}

	return DEV_OK;
}

DEVICE_INIT(frdm_k64f_pmux, "", fsl_frdm_k64f_pin_init, NULL, NULL,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif /* CONFIG_PINMUX */
