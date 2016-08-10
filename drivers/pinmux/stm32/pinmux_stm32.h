/*
 * Copyright (c) 2016 Open-RnD Sp. z o.o.
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
 * @file header for STM32 pin multiplexing
 */

#ifndef _STM32_PINMUX_H_
#define _STM32_PINMUX_H_

#include <stdint.h>
#include <stddef.h>
#include <clock_control.h>
#include "pinmux/pinmux.h"

/**
 * @brief numerical IDs for IO ports
 */
enum stm32_pin_port {
	STM32_PORTA = 0,	/* IO port A */
	STM32_PORTB,		/* .. */
	STM32_PORTC,
	STM32_PORTD,
	STM32_PORTE,
	STM32_PORTF,
	STM32_PORTG,
	STM32_PORTH,		/* IO port H */
};

/* override this at soc level */
#ifndef STM32_PORTS_MAX
#define STM32_PORTS_MAX (STM32_PORTH + 1)
#endif

/**
 * @brief helper macro to encode an IO port pin in a numerical format
 */
#define STM32PIN(_port, _pin) \
	(_port << 4 | _pin)

/*
 */
enum stm32_pin_alt_func {
	STM32_PINMUX_FUNC_ALT_0 = 0, /* GPIO */
	STM32_PINMUX_FUNC_ALT_1,
	STM32_PINMUX_FUNC_ALT_2,
	STM32_PINMUX_FUNC_ALT_3,
	STM32_PINMUX_FUNC_ALT_4,
	STM32_PINMUX_FUNC_ALT_5,
	STM32_PINMUX_FUNC_ALT_6,
	STM32_PINMUX_FUNC_ALT_7,
	STM32_PINMUX_FUNC_ALT_8,
	STM32_PINMUX_FUNC_ALT_9,
	STM32_PINMUX_FUNC_ALT_10,
	STM32_PINMUX_FUNC_ALT_11,
	STM32_PINMUX_FUNC_ALT_12,
	STM32_PINMUX_FUNC_ALT_13,
	STM32_PINMUX_FUNC_ALT_14,
	STM32_PINMUX_FUNC_ALT_15,
	STM32_PINMUX_FUNC_ALT_MAX
};

#define STM32_PINMUX_FUNC_GPIO 0
#define STM32_PINMUX_FUNC_ANALOG (STM32_PINMUX_FUNC_ALT_MAX)

#define STM32_PIN_PA0	STM32PIN(STM32_PORTA, 0)
#define STM32_PIN_PA1	STM32PIN(STM32_PORTA, 1)
#define STM32_PIN_PA2	STM32PIN(STM32_PORTA, 2)
#define STM32_PIN_PA3	STM32PIN(STM32_PORTA, 3)
#define STM32_PIN_PA4	STM32PIN(STM32_PORTA, 4)
#define STM32_PIN_PA5	STM32PIN(STM32_PORTA, 5)
#define STM32_PIN_PA6	STM32PIN(STM32_PORTA, 6)
#define STM32_PIN_PA7	STM32PIN(STM32_PORTA, 7)
#define STM32_PIN_PA8	STM32PIN(STM32_PORTA, 8)
#define STM32_PIN_PA9	STM32PIN(STM32_PORTA, 9)
#define STM32_PIN_PA10  STM32PIN(STM32_PORTA, 10)
#define STM32_PIN_PA11  STM32PIN(STM32_PORTA, 11)
#define STM32_PIN_PA12  STM32PIN(STM32_PORTA, 12)
#define STM32_PIN_PA13  STM32PIN(STM32_PORTA, 13)
#define STM32_PIN_PA14  STM32PIN(STM32_PORTA, 14)
#define STM32_PIN_PA15  STM32PIN(STM32_PORTA, 15)

#define STM32_PIN_PB0	STM32PIN(STM32_PORTB, 0)
#define STM32_PIN_PB1	STM32PIN(STM32_PORTB, 1)
#define STM32_PIN_PB2	STM32PIN(STM32_PORTB, 2)
#define STM32_PIN_PB3	STM32PIN(STM32_PORTB, 3)
#define STM32_PIN_PB4	STM32PIN(STM32_PORTB, 4)
#define STM32_PIN_PB5	STM32PIN(STM32_PORTB, 5)
#define STM32_PIN_PB6	STM32PIN(STM32_PORTB, 6)
#define STM32_PIN_PB7	STM32PIN(STM32_PORTB, 7)
#define STM32_PIN_PB8	STM32PIN(STM32_PORTB, 8)
#define STM32_PIN_PB9	STM32PIN(STM32_PORTB, 9)
#define STM32_PIN_PB10  STM32PIN(STM32_PORTB, 10)
#define STM32_PIN_PB11  STM32PIN(STM32_PORTB, 11)
#define STM32_PIN_PB12  STM32PIN(STM32_PORTB, 12)
#define STM32_PIN_PB13  STM32PIN(STM32_PORTB, 13)
#define STM32_PIN_PB14  STM32PIN(STM32_PORTB, 14)
#define STM32_PIN_PB15  STM32PIN(STM32_PORTB, 15)

#define STM32_PIN_PC0	STM32PIN(STM32_PORTC, 0)
#define STM32_PIN_PC1	STM32PIN(STM32_PORTC, 1)
#define STM32_PIN_PC2	STM32PIN(STM32_PORTC, 2)
#define STM32_PIN_PC3	STM32PIN(STM32_PORTC, 3)
#define STM32_PIN_PC4	STM32PIN(STM32_PORTC, 4)
#define STM32_PIN_PC5	STM32PIN(STM32_PORTC, 5)
#define STM32_PIN_PC6	STM32PIN(STM32_PORTC, 6)
#define STM32_PIN_PC7	STM32PIN(STM32_PORTC, 7)
#define STM32_PIN_PC8	STM32PIN(STM32_PORTC, 8)
#define STM32_PIN_PC9	STM32PIN(STM32_PORTC, 9)
#define STM32_PIN_PC10  STM32PIN(STM32_PORTC, 10)
#define STM32_PIN_PC11  STM32PIN(STM32_PORTC, 11)
#define STM32_PIN_PC12  STM32PIN(STM32_PORTC, 12)
#define STM32_PIN_PC13  STM32PIN(STM32_PORTC, 13)
#define STM32_PIN_PC14  STM32PIN(STM32_PORTC, 14)
#define STM32_PIN_PC15  STM32PIN(STM32_PORTC, 15)

#define STM32_PIN_PD0	STM32PIN(STM32_PORTD, 0)
#define STM32_PIN_PD1	STM32PIN(STM32_PORTD, 1)
#define STM32_PIN_PD2	STM32PIN(STM32_PORTD, 2)
#define STM32_PIN_PD3	STM32PIN(STM32_PORTD, 3)
#define STM32_PIN_PD4	STM32PIN(STM32_PORTD, 4)
#define STM32_PIN_PD5	STM32PIN(STM32_PORTD, 5)
#define STM32_PIN_PD6	STM32PIN(STM32_PORTD, 6)
#define STM32_PIN_PD7	STM32PIN(STM32_PORTD, 7)
#define STM32_PIN_PD8	STM32PIN(STM32_PORTD, 8)
#define STM32_PIN_PD9	STM32PIN(STM32_PORTD, 9)
#define STM32_PIN_PD10  STM32PIN(STM32_PORTD, 10)
#define STM32_PIN_PD11  STM32PIN(STM32_PORTD, 11)
#define STM32_PIN_PD12  STM32PIN(STM32_PORTD, 12)
#define STM32_PIN_PD13  STM32PIN(STM32_PORTD, 13)
#define STM32_PIN_PD14  STM32PIN(STM32_PORTD, 14)
#define STM32_PIN_PD15  STM32PIN(STM32_PORTD, 15)

#define STM32_PIN_PE0	STM32PIN(STM32_PORTE, 0)
#define STM32_PIN_PE1	STM32PIN(STM32_PORTE, 1)
#define STM32_PIN_PE2	STM32PIN(STM32_PORTE, 2)
#define STM32_PIN_PE3	STM32PIN(STM32_PORTE, 3)
#define STM32_PIN_PE4	STM32PIN(STM32_PORTE, 4)
#define STM32_PIN_PE5	STM32PIN(STM32_PORTE, 5)
#define STM32_PIN_PE6	STM32PIN(STM32_PORTE, 6)
#define STM32_PIN_PE7	STM32PIN(STM32_PORTE, 7)
#define STM32_PIN_PE8	STM32PIN(STM32_PORTE, 8)
#define STM32_PIN_PE9	STM32PIN(STM32_PORTE, 9)
#define STM32_PIN_PE10  STM32PIN(STM32_PORTE, 10)
#define STM32_PIN_PE11  STM32PIN(STM32_PORTE, 11)
#define STM32_PIN_PE12  STM32PIN(STM32_PORTE, 12)
#define STM32_PIN_PE13  STM32PIN(STM32_PORTE, 13)
#define STM32_PIN_PE14  STM32PIN(STM32_PORTE, 14)
#define STM32_PIN_PE15  STM32PIN(STM32_PORTE, 15)

#define STM32_PIN_PF0	STM32PIN(STM32_PORTF, 0)
#define STM32_PIN_PF1	STM32PIN(STM32_PORTF, 1)
#define STM32_PIN_PF2	STM32PIN(STM32_PORTF, 2)
#define STM32_PIN_PF3	STM32PIN(STM32_PORTF, 3)
#define STM32_PIN_PF4	STM32PIN(STM32_PORTF, 4)
#define STM32_PIN_PF5	STM32PIN(STM32_PORTF, 5)
#define STM32_PIN_PF6	STM32PIN(STM32_PORTF, 6)
#define STM32_PIN_PF7	STM32PIN(STM32_PORTF, 7)
#define STM32_PIN_PF8	STM32PIN(STM32_PORTF, 8)
#define STM32_PIN_PF9	STM32PIN(STM32_PORTF, 9)
#define STM32_PIN_PF10  STM32PIN(STM32_PORTF, 10)
#define STM32_PIN_PF11  STM32PIN(STM32_PORTF, 11)
#define STM32_PIN_PF12  STM32PIN(STM32_PORTF, 12)
#define STM32_PIN_PF13  STM32PIN(STM32_PORTF, 13)
#define STM32_PIN_PF14  STM32PIN(STM32_PORTF, 14)
#define STM32_PIN_PF15  STM32PIN(STM32_PORTF, 15)

#define STM32_PIN_PG0	STM32PIN(STM32_PORTG, 0)
#define STM32_PIN_PG1	STM32PIN(STM32_PORTG, 1)
#define STM32_PIN_PG2	STM32PIN(STM32_PORTG, 2)
#define STM32_PIN_PG3	STM32PIN(STM32_PORTG, 3)
#define STM32_PIN_PG4	STM32PIN(STM32_PORTG, 4)
#define STM32_PIN_PG5	STM32PIN(STM32_PORTG, 5)
#define STM32_PIN_PG6	STM32PIN(STM32_PORTG, 6)
#define STM32_PIN_PG7	STM32PIN(STM32_PORTG, 7)
#define STM32_PIN_PG8	STM32PIN(STM32_PORTG, 8)
#define STM32_PIN_PG9	STM32PIN(STM32_PORTG, 9)
#define STM32_PIN_PG10  STM32PIN(STM32_PORTG, 10)
#define STM32_PIN_PG11  STM32PIN(STM32_PORTG, 11)
#define STM32_PIN_PG12  STM32PIN(STM32_PORTG, 12)
#define STM32_PIN_PG13  STM32PIN(STM32_PORTG, 13)
#define STM32_PIN_PG14  STM32PIN(STM32_PORTG, 14)
#define STM32_PIN_PG15  STM32PIN(STM32_PORTG, 15)

/* pretend that array will cover pin functions */
typedef int stm32_pin_func_t;

/**
 * @brief pinmux config wrapper
 *
 * GPIO function is assumed to be always available, as such it's not listed
 * in @funcs array
 */
struct stm32_pinmux_conf {
	uint32_t pin;		 /* pin ID */
	const stm32_pin_func_t *funcs; /* functions array, indexed with
					* (stm32_pin_alt_func - 1)
					*/
	const size_t nfuncs;	 /* number of alternate functions, not
				  * counting GPIO
				  */
};

/**
 * @brief helper to define pins
 */
#define STM32_PIN_CONF(__pin, __funcs) \
	{__pin, __funcs, ARRAY_SIZE(__funcs)}

/**
 * @brief helper to extract IO port number from STM32PIN() encoded
 * value
 */
#define STM32_PORT(__pin) \
	(__pin >> 4)

/**
 * @brief helper to extract IO pin number from STM32PIN() encoded
 * value
 */
#define STM32_PIN(__pin) \
	(__pin & 0xf)

/**
 * @brief helper for mapping pin function to its configuration
 *
 * @param pin   pin ID encoded with STM32PIN()
 * @param func  alternate function ID
 *
 * Helper function for mapping alternate function for given pin to its
 * configuration. This function must be implemented by SoC integartion
 * code.
 *
 * @return SoC specific pin configuration
 */
int stm32_get_pin_config(int pin, int func);

/**
 * @brief helper for mapping IO port to its clock subsystem
 *
 * @param port  IO port
 *
 * Map given IO @port to corresponding clock subsystem. The returned
 * clock subsystemd ID must suitable for passing as parameter to
 * clock_control_on(). Implement this function at the SoC level.
 *
 * @return clock subsystem ID
 */
clock_control_subsys_t stm32_get_port_clock(int port);

/**
 * @brief helper for configuration of IO pin
 *
 * @param pin IO pin, STM32PIN() encoded
 * @param func IO function encoded
 * @param clk clock control device, for enabling/disabling clock gate
 * for the port
 */
int _pinmux_stm32_set(uint32_t pin, uint32_t func,
		      struct device *clk);

/**
 * @brief helper for obtaining pin configuration for the board
 *
 * @param[out] pins  set to the number of pins in the array
 *
 * Obtain pin assignment/configuration for current board. This call
 * needs to be implemented at the board integration level. After
 * restart all pins are already configured as GPIO and can be skipped
 * in the configuration arrray. Pin numbers in @pin_num field are
 * STM32PIN() encoded.
 *
 * @return array of pin assignments
 */
void stm32_setup_pins(const struct pin_config *pinconf,
		      size_t pins);

/* common pinmux device name for all STM32 chips */
#define STM32_PINMUX_NAME "stm32-pinmux"

#ifdef CONFIG_SOC_SERIES_STM32F1X
#include "pinmux_stm32f1.h"
#elif CONFIG_SOC_SERIES_STM32F4X
#include "pinmux_stm32f4.h"
#endif

#endif	/* _STM32_PINMUX_H_ */
