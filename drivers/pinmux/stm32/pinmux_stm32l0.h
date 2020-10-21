/*
 * Copyright (c) 2018 Ilya Tagunov
 * Copyright (c) 2019 Centaur Analytics, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32L0_H_
#define ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32L0_H_

/**
 * @file Header for STM32L0 pin multiplexing helper
 */

/*
 * Note:
 * The SPIx_SCK pin speed must be set to VERY_HIGH to avoid last data bit
 * corruption which is a known issue of STM32L0 SPI peripheral (see errata
 * sheets).
 */

#define STM32L0_PINMUX_FUNC_PB6_USART1_TX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PB7_USART1_RX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_NO_PULL)

#define STM32L0_PINMUX_FUNC_PA9_USART1_TX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PA10_USART1_RX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_PUPDR_NO_PULL)

#define STM32L0_PINMUX_FUNC_PA2_USART2_TX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PA3_USART2_RX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_PUPDR_NO_PULL)

#define STM32L0_PINMUX_FUNC_PA14_USART2_TX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PA15_USART2_RX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_PUPDR_NO_PULL)

#define STM32L0_PINMUX_FUNC_PA2_LPUART1_TX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_6 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PA3_LPUART1_RX __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_6 | STM32_PUPDR_NO_PULL)

/* I2C1 */
#define STM32L0_PINMUX_FUNC_PA9_I2C1_SCL __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_6 | STM32_OPENDRAIN_PULLUP)
#define STM32L0_PINMUX_FUNC_PA10_I2C1_SDA __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_6 | STM32_OPENDRAIN_PULLUP)

#define STM32L0_PINMUX_FUNC_PB6_I2C1_SCL __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_OPENDRAIN_PULLUP)
#define STM32L0_PINMUX_FUNC_PB7_I2C1_SDA __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_1 | STM32_OPENDRAIN_PULLUP)

#define STM32L0_PINMUX_FUNC_PB8_I2C1_SCL __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_OPENDRAIN_PULLUP)
#define STM32L0_PINMUX_FUNC_PB9_I2C1_SDA __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_4 | STM32_OPENDRAIN_PULLUP)

/* I2C2 */
#define STM32L0_PINMUX_FUNC_PB10_I2C2_SCL __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_6 | STM32_OPENDRAIN_PULLUP)
#define STM32L0_PINMUX_FUNC_PB11_I2C2_SDA __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_6 | STM32_OPENDRAIN_PULLUP)

#define STM32L0_PINMUX_FUNC_PB13_I2C2_SCL __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_5 | STM32_OPENDRAIN_PULLUP)
#define STM32L0_PINMUX_FUNC_PB14_I2C2_SDA __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_5 | STM32_OPENDRAIN_PULLUP)

/* I2C3 */
#define STM32L0_PINMUX_FUNC_PA8_I2C3_SCL __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_7 | STM32_OPENDRAIN_PULLUP)
#define STM32L0_PINMUX_FUNC_PB4_I2C3_SDA __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_7 | STM32_OPENDRAIN_PULLUP)

#define STM32L0_PINMUX_FUNC_PC0_I2C3_SCL __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_7 | STM32_OPENDRAIN_PULLUP)
#define STM32L0_PINMUX_FUNC_PC1_I2C3_SDA __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_7 | STM32_OPENDRAIN_PULLUP)

#define STM32L0_PINMUX_FUNC_PC9_I2C3_SDA __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_7 | STM32_OPENDRAIN_PULLUP)

/*
 * Increase SCK pin speed to avoid last data bit corruption which is
 * a known issue of STM32L0 SPI peripheral (see errata sheets).
 */

#define STM32L0_PINMUX_FUNC_PA4_SPI1_NSS __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PA5_SPI1_SCK __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL | \
	 STM32_OSPEEDR_VERY_HIGH_SPEED)
#define STM32L0_PINMUX_FUNC_PA6_SPI1_MISO __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_PULL_DOWN)
#define STM32L0_PINMUX_FUNC_PA7_SPI1_MOSI __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_PULL_DOWN)

#define STM32L0_PINMUX_FUNC_PA15_SPI1_NSS __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PB3_SPI1_SCK __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL | \
	 STM32_OSPEEDR_VERY_HIGH_SPEED)
#define STM32L0_PINMUX_FUNC_PB4_SPI1_MISO __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_PULL_DOWN)
#define STM32L0_PINMUX_FUNC_PB5_SPI1_MOSI __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_PULL_DOWN)

#define STM32L0_PINMUX_FUNC_PB12_SPI2_NSS __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PB13_SPI2_SCK __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUSHPULL_NOPULL | \
	 STM32_OSPEEDR_VERY_HIGH_SPEED)
#define STM32L0_PINMUX_FUNC_PB14_SPI2_MISO __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_PULL_DOWN)
#define STM32L0_PINMUX_FUNC_PB15_SPI2_MOSI __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_0 | STM32_PUPDR_PULL_DOWN)

#define STM32L0_PINMUX_FUNC_PB9_SPI2_NSS __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_5 | STM32_PUSHPULL_PULLUP)
#define STM32L0_PINMUX_FUNC_PB10_SPI2_SCK __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_5 | STM32_PUSHPULL_NOPULL | \
	 STM32_OSPEEDR_VERY_HIGH_SPEED)
#define STM32L0_PINMUX_FUNC_PC2_SPI2_MISO __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_2 | STM32_PUPDR_PULL_DOWN)
#define STM32L0_PINMUX_FUNC_PC3_SPI2_MOSI __DEPRECATED_MACRO \
	(STM32_PINMUX_ALT_FUNC_2 | STM32_PUPDR_PULL_DOWN)

/* USB */
#define STM32L0_PINMUX_FUNC_PA11_USB_DM  \
	STM32_MODER_INPUT_MODE
#define STM32L0_PINMUX_FUNC_PA12_USB_DP  \
	STM32_MODER_INPUT_MODE

#define STM32L0_PINMUX_FUNC_PC0_ADC_IN10 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PC1_ADC_IN11 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PC2_ADC_IN12 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PC3_ADC_IN13 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA0_ADC_IN0 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA1_ADC_IN1 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA2_ADC_IN2 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA3_ADC_IN3 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA4_ADC_IN4 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA5_ADC_IN5 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA6_ADC_IN6 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA7_ADC_IN7 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PC4_ADC_IN14 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PC5_ADC_IN15 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PB0_ADC_IN8 \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PB1_ADC_IN9 \
	STM32_MODER_ANALOG_MODE

#define STM32L0_PINMUX_FUNC_PA4_DAC_OUT1 __DEPRECATED_MACRO \
	STM32_MODER_ANALOG_MODE
#define STM32L0_PINMUX_FUNC_PA5_DAC_OUT2 __DEPRECATED_MACRO \
	STM32_MODER_ANALOG_MODE

#endif /* ZEPHYR_DRIVERS_PINMUX_STM32_PINMUX_STM32L0_H_ */
