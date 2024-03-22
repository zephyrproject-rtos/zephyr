/*
 * Copyright (c) 2023 ENE Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ENE_KB1200_GPIO_H
#define ENE_KB1200_GPIO_H

/**
 *  Structure type to access General Purpose Input/Output (GPIO).
 */
struct gpio_regs {
	volatile uint32_t GPIOFS; /*Function Selection Register */
	volatile uint32_t Reserved1[3];
	volatile uint32_t GPIOOE; /*Output Enable Register */
	volatile uint32_t Reserved2[3];
	volatile uint32_t GPIOD; /*Output Data Register */
	volatile uint32_t Reserved3[3];
	volatile uint32_t GPIOIN; /*Input Data Register */
	volatile uint32_t Reserved4[3];
	volatile uint32_t GPIOPU; /*Pull Up Register */
	volatile uint32_t Reserved5[3];
	volatile uint32_t GPIOOD; /*Open Drain Register */
	volatile uint32_t Reserved6[3];
	volatile uint32_t GPIOIE; /*Input Enable Register */
	volatile uint32_t Reserved7[3];
	volatile uint32_t GPIODC; /*Driving Control Register */
	volatile uint32_t Reserved8[3];
	volatile uint32_t GPIOLV; /*Low Voltage Mode Enable Register */
	volatile uint32_t Reserved9[3];
	volatile uint32_t GPIOPD; /*Pull Down Register */
	volatile uint32_t Reserved10[3];
	volatile uint32_t GPIOFL; /*Function Lock Register */
	volatile uint32_t Reserved11[3];
};

#define NUM_KB1200_GPIO_PORTS 4

/*-- Constant Define --------------------------------------------*/
#define GPIO00_PWMLED0_PWM8     0x00
#define GPIO01_SERRXD1_UARTSIN  0x01
#define GPIO03_SERTXD1_UARTSOUT 0x03
#define GPIO22_ESBDAT_PWM9      0x22
#define GPIO28_32KOUT_SERCLK2   0x28
#define GPIO36_UARTSOUT_SERTXD2 0x36
#define GPIO5C_KSO6_P80DAT      0x5C
#define GPIO5D_KSO7_P80CLK      0x5D
#define GPIO5E_KSO8_SERRXD1     0x5E
#define GPIO5F_KSO9_SERTXD1     0x5F
#define GPIO71_SDA8_UARTRTS     0x71
#define GPIO38_SCL4_PWM1        0x38


#endif /* ENE_KB1200_GPIO_H */
