/*
 * Copyright (c) 2020 Mark Olsson <mark@markolsson.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "stm32f7xx.h"
#include "kernel.h"

// Size of early_init_vector_table at the bottom of this file
#define VECTOR_TABLE_OFFSET 0x80;
extern void __start(void);

void configure_gpio_fmc(char port, uint8_t pin)
{
	/*
	 * Alternate function: 12
	 * Alternate function mode
	 * Speed: 50 MHz
	 * Output type: push-pull
	 * No pull-up, pull-down
	 */
	volatile GPIO_TypeDef *GPIOx_base =
		(void *)(GPIOA_BASE + (port - 'A') * 0x400);

	if (pin >= 8) {
		GPIOx_base->AFR[1] |= 0xC << ((pin - 8) * 4);
	} else {
		GPIOx_base->AFR[0] |= 0xC << (pin * 4);
	}
	GPIOx_base->MODER |= 0x2 << (pin * 2);
	GPIOx_base->OSPEEDR |= 0x2 << (pin * 2);
	GPIOx_base->OTYPER |= 0x0 << (pin * 2);
	GPIOx_base->PUPDR |= 0x1 << (pin * 2);
}

void early_init_handler(void)
{
	register u32_t temp_register = 0, timeout = 0xFFFF;
	register __IO u32_t index;

	/* Reset the RCC clock configuration */
	RCC->CR = (u32_t)0x00000083;
	RCC->CFGR = (u32_t)0x00000000;
	RCC->PLLCFGR = (u32_t)0x24003010;
	RCC->CIR = (u32_t)0x00000000;

	/* Enable GPIO clocks for FMC pins */
	RCC->AHB1ENR |= (
		RCC_AHB1ENR_GPIOCEN |
		RCC_AHB1ENR_GPIODEN |
		RCC_AHB1ENR_GPIOEEN |
		RCC_AHB1ENR_GPIOFEN |
		RCC_AHB1ENR_GPIOGEN |
		RCC_AHB1ENR_GPIOHEN
	);

	/* Cofigure the GPIO Pins for FMC */
	configure_gpio_fmc('C', 3);
	configure_gpio_fmc('D', 0);
	configure_gpio_fmc('D', 1);
	configure_gpio_fmc('D', 8);
	configure_gpio_fmc('D', 9);
	configure_gpio_fmc('D', 10);
	configure_gpio_fmc('D', 14);
	configure_gpio_fmc('D', 15);
	configure_gpio_fmc('E', 0);
	configure_gpio_fmc('E', 1);
	configure_gpio_fmc('E', 7);
	configure_gpio_fmc('E', 8);
	configure_gpio_fmc('E', 9);
	configure_gpio_fmc('E', 10);
	configure_gpio_fmc('E', 11);
	configure_gpio_fmc('E', 12);
	configure_gpio_fmc('E', 13);
	configure_gpio_fmc('E', 14);
	configure_gpio_fmc('E', 15);
	configure_gpio_fmc('F', 0);
	configure_gpio_fmc('F', 1);
	configure_gpio_fmc('F', 2);
	configure_gpio_fmc('F', 3);
	configure_gpio_fmc('F', 4);
	configure_gpio_fmc('F', 5);
	configure_gpio_fmc('F', 11);
	configure_gpio_fmc('F', 12);
	configure_gpio_fmc('F', 13);
	configure_gpio_fmc('F', 14);
	configure_gpio_fmc('F', 15);
	configure_gpio_fmc('G', 0);
	configure_gpio_fmc('G', 1);
	configure_gpio_fmc('G', 4);
	configure_gpio_fmc('G', 5);
	configure_gpio_fmc('G', 8);
	configure_gpio_fmc('G', 15);
	configure_gpio_fmc('H', 3);
	configure_gpio_fmc('H', 5);

	/* Enable the FMC interface clock */
	RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;

	/* Configure and enable SDRAM bank1 */
	FMC_Bank5_6->SDCR[0] = 0x00001954;
	FMC_Bank5_6->SDTR[0] = 0x01115351;

	/* SDRAM initialization sequence */
	/* Clock enable command */
	FMC_Bank5_6->SDCMR = 0x00000011;
	temp_register = FMC_Bank5_6->SDSR & 0x00000020;
	while ((temp_register != 0) && (timeout-- > 0)) {
		temp_register = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Delay */
	for (index = 0; index < 1000; index++) {
	}

	/* PALL command */
	FMC_Bank5_6->SDCMR = 0x00000012;
	timeout = 0xFFFF;
	while ((temp_register != 0) && (timeout-- > 0)) {
		temp_register = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Auto refresh command */
	FMC_Bank5_6->SDCMR = 0x000000F3;
	timeout = 0xFFFF;
	while ((temp_register != 0) && (timeout-- > 0)) {
		temp_register = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* MRD register program */
	FMC_Bank5_6->SDCMR = 0x00044014;
	timeout = 0xFFFF;
	while ((temp_register != 0) && (timeout-- > 0)) {
		temp_register = FMC_Bank5_6->SDSR & 0x00000020;
	}

	/* Set refresh count */
	temp_register = FMC_Bank5_6->SDRTR;
	FMC_Bank5_6->SDRTR = (temp_register | (0x0000050C << 1));

	/* Disable write protection */
	temp_register = FMC_Bank5_6->SDCR[0];
	FMC_Bank5_6->SDCR[0] = (temp_register & 0xFFFFFDFF);

	/* Set vector table to the Zephyr one */
	SCB->VTOR = FLASHAXI_BASE | VECTOR_TABLE_OFFSET;

	/* Jump to Zephyr __start */
	__start();
}

/* Vector table for early_init */
unsigned long *early_init_vector_table[0x80] __attribute__((section(".early_init"))) = {
	(unsigned long *)0x20000400,
	(unsigned long *)early_init_handler,
};
