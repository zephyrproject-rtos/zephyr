/* This sample describes the step by step procedure to be followed for using PLIC in zephyr*/


/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/sw_isr_table.h>


#define GPIO_PIN 		6
#define GPIO_PIN_FLAG 	1

/* 
	First 32 indices in isr_tables.c are reserved for system level exceptions. 
	Always add 32 to the IRQ_LINE number (refer platform.h in bare-metal for IRQ_LINE number).
	IRQ_LINE_NUM = 0 -> No interrupt 
*/

#define INT_ID 			(uint32_t)(GPIO_PIN + 32)

/**
 * @fn void gpio_application_isr(const void*)
 * @brief The function is an example for user-defined ISR() from application side.
 * @param void* The parameter \a void* is a null pointer to array of arguments that can be passed to isr().
 */

int gpio_application_isr(const void*)
{
    printk("Entered ISR from application side\n");
    return 1;
}

/**
 * @fn void isr_installer(const void*)
 * @brief This function installs the irq service for a specific peripheral and lined.
 * @param void 
 */


static void isr_installer(void)
{
	IRQ_CONNECT(INT_ID, 1, gpio_application_isr, NULL, 0);
	irq_enable(INT_ID);
}

/**
 * @fn int main(void)
 * @brief The main function describes the steps to initialize interrupts
 * @param void 
 */

int main(void)
{

	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	printf("Entered main.c\n");

	// Initialize interrupt from peripheral(GPIO) side.
	gpio_pin_configure(dev, GPIO_PIN, 0);
    gpio_pin_interrupt_configure(dev, GPIO_PIN, 1);

	// Initialize interrupt from PLIC side
	plic_irq_enable(INT_ID);
		
	// Install user-defined ISR() into zephyr's isr_table
	isr_installer();


	while(1)
	{
		printf("GPIO Pin Status : %x\n", gpio_port_get_raw(dev, GPIO_PIN));
	}

	return 0;
}
