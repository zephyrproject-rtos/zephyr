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

#define INT_ID 			(uint32_t)(GPIO_PIN + 32)


void arch_system_halt(unsigned int reason)
{
	struct _isr_table_entry *plic_int_t;
	plic_int_t = (struct _isr_table_entry *)(&_sw_isr_table[11]);
    plic_int_t->isr(plic_int_t->arg);
}

int gpio_application_isr(const void*)
{
    printk("Entered ISR from application side\n");
    return 1;
}

static void isr_installer(void)
{
	IRQ_CONNECT(INT_ID, 1, gpio_application_isr, NULL, 0);
	irq_enable(INT_ID);
}

int main(void)
{

	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
	printf("Entered main.c\n");

	gpio_pin_configure(dev, GPIO_PIN, 0);
    gpio_pin_interrupt_configure(dev, GPIO_PIN, 1);

	plic_irq_enable(INT_ID);
		
	isr_installer();
	//bp
      // Enable Global (PLIC) interrupts.
    __asm__ volatile("li      t0, 8\t\n" 
				"csrrs   zero, mstatus, t0\t\n"
				);

    // Enable Local (PLIC) interrupts.
    __asm__ volatile("li      t0, 0x800\t\n"
                 "csrrs   zero, mie, t0\t\n"
                );


	while(1)
	{
		printf("GPIO Pin Status : %x\n", gpio_port_get_raw(dev, GPIO_PIN));
	}

	return 0;
}
