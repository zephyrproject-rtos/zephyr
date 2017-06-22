/*
 * Copyright (c) 2017 RDA
 * Copyright (c) 2016-2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _PINMUX_RDA5981A_H_
#define _PINMUX_RDA5981A_H_

#include <stdint.h>
#include <stddef.h>

#include "pinmux/pinmux.h"
#include "soc.h"

enum rda5981a_pin_port {
	PIN_PORTA = 0,	/* IO port A */
	PIN_PORTB,
	PIN_PORTC = 4,
	PIN_PORTD,
};

#define RDA5981A_PIN(_port, _pin) \
	((_port << 5) | _pin)

#define PIN_TO_PORT(__pin) \
		(__pin >> 5)

#define GET_PIN(__pin) \
	(__pin & 0x1f)

//iomux ctrl reg 0
#define PA0	RDA5981A_PIN(PIN_PORTA, 26)
#define PA1	RDA5981A_PIN(PIN_PORTA, 27)
#define PA2	RDA5981A_PIN(PIN_PORTA, 14)
#define PA3	RDA5981A_PIN(PIN_PORTA, 15)
#define PA4	RDA5981A_PIN(PIN_PORTA, 16)
#define PA5	RDA5981A_PIN(PIN_PORTA, 17)
#define PA6	RDA5981A_PIN(PIN_PORTA, 18)
#define PA7	RDA5981A_PIN(PIN_PORTA, 19)
#define PA8	RDA5981A_PIN(PIN_PORTA, 10)
#define PA9	RDA5981A_PIN(PIN_PORTA, 11)

//iomux ctrl reg 1
#define PB0	RDA5981A_PIN(PIN_PORTB, 0)
#define PB1	RDA5981A_PIN(PIN_PORTB, 1)
#define PB2	RDA5981A_PIN(PIN_PORTB, 2)
#define PB3	RDA5981A_PIN(PIN_PORTB, 3)
#define PB4	RDA5981A_PIN(PIN_PORTB, 4)
#define PB5	RDA5981A_PIN(PIN_PORTB, 5)
#define PB6	RDA5981A_PIN(PIN_PORTB, 6)
#define PB7	RDA5981A_PIN(PIN_PORTB, 7)
#define PB8	RDA5981A_PIN(PIN_PORTB, 8)
#define PB9	RDA5981A_PIN(PIN_PORTB, 9)

//iomux ctrl reg 4
#define PC0	RDA5981A_PIN(PIN_PORTC, 12)
#define PC1	RDA5981A_PIN(PIN_PORTC, 13)
#define PC2	RDA5981A_PIN(PIN_PORTC, 14)
#define PC3	RDA5981A_PIN(PIN_PORTC, 15)
#define PC4	RDA5981A_PIN(PIN_PORTC, 16)
#define PC5	RDA5981A_PIN(PIN_PORTC, 17)
#define PC6	RDA5981A_PIN(PIN_PORTC, 18)
#define PC7	RDA5981A_PIN(PIN_PORTC, 19)
#define PC8	RDA5981A_PIN(PIN_PORTC, 20)
#define PC9	RDA5981A_PIN(PIN_PORTC, 21)

//iomux ctrl reg 4
#define PD0	RDA5981A_PIN(PIN_PORTD, 22)
#define PD1	RDA5981A_PIN(PIN_PORTD, 23)
#define PD2	RDA5981A_PIN(PIN_PORTD, 24)
#define PD3	RDA5981A_PIN(PIN_PORTD, 25)

#define NC	0xff

enum rda5981a_gpio_pin {
	GPIO_PIN0  = PB0,
	GPIO_PIN1  = PB1,
	GPIO_PIN2  = PB2,
	GPIO_PIN3  = PB3,
	GPIO_PIN4  = PB4,
	GPIO_PIN5  = PB5,
	GPIO_PIN6  = PB6,
	GPIO_PIN7  = PB7,
	GPIO_PIN8  = PB8,
	GPIO_PIN9  = PB9,
	GPIO_PIN10 = PA8,
	GPIO_PIN11 = PA9,
	GPIO_PIN12 = PC0,
	GPIO_PIN13 = PC1,
	GPIO_PIN14 = PC2,
	GPIO_PIN15 = PC3,
	GPIO_PIN16 = PC4,
	GPIO_PIN17 = PC5,
	GPIO_PIN18 = PC6,
	GPIO_PIN19 = PC7,
	GPIO_PIN20 = PC8,
	GPIO_PIN21 = PC9,
	GPIO_PIN22 = PD0,
	GPIO_PIN23 = PD1,
	GPIO_PIN24 = PD2,
	GPIO_PIN25 = PD3,
	GPIO_PIN26 = PA0,
	GPIO_PIN27 = PA1,

	// Another pin names for GPIO 14 - 19
	GPIO_PIN14A = PA2,
	GPIO_PIN15A = PA3,
	GPIO_PIN16A = PA4,
	GPIO_PIN17A = PA5,
	GPIO_PIN18A = PA6,
	GPIO_PIN19A = PA7,

	UART0_RX = PA0,
	UART0_TX = PA1,
	UART1_RX = PB1,
	UART1_TX = PB2,

	I2C_SCL = PC0,
	I2C_SDA = PC1,

	I2S_TX_SD   = PB1,
	I2S_TX_WS   = PB2,
	I2S_TX_BCLK = PB3,
	I2S_RX_SD   = PB4,
	I2S_RX_WS   = PB5,
	I2S_RX_BCLK = PB8,
};

#define PIN_INDEX(pin)	(pin & 0xff)

typedef int rda5981a_pin_func_t;

struct rda5981a_pinmux_conf {
	uint32_t pin;
	const rda5981a_pin_func_t *funcs;
	const size_t nfuncs;
};

#define RDA5981A_PIN_CONF(__pin, __funcs) \
	{__pin, __funcs, ARRAY_SIZE(__funcs)}

enum {
	GPIO_0 = (uint32_t)RDA_GPIO_BASE
} ;

#define PER_BITBAND_ADDR(reg, bit)      \
	(uint32_t *)(RDA_PERBTBND_BASE + (((uint32_t)(reg)-RDA_PER_BASE)<<5U) + (((uint32_t)(bit))<<2U))

void pinmux_rda5981a_set(uint32_t pin, uint32_t func);
void rda5981a_setup_pins(const struct pin_config *pinconf,uint32_t pins);

#endif
