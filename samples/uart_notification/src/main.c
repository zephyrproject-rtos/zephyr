/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

// static const struct 
#define INT_ID 32+47+1
char ptr_string[100];
const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(uart1));
uint8_t read_uart_string()
{
    uint32_t i = 0;
    char temp ;

    while(1)
    {
        uart_poll_in(dev,&temp);
        // printk("%c",temp);
        ptr_string[i++] = temp;
        // putchar(temp);

        if (temp == 0x0D)
        {
            ptr_string[i++] = 0x0A;
            ptr_string[i++] = 0x00;
            printk("\ns",ptr_string);
            return(i);
            
        }
    }
}
int main()
{
    unsigned char p_char;
    
        IRQ_CONNECT(INT_ID, 1, read_uart_string, NULL, 0);
        irq_enable(INT_ID);
        plic_irq_enable(INT_ID);
        uart_irq_rx_enable(dev);
    printk("UART testing begin...\n");
    while (1)
    {
        printk("1\n");
        /* code */
    }
    
    // while (1)
    // {
    //    printk("\n hi\n");
    //    read_uart_string(dev,ptr_string);
    //    printk("%s",ptr_string);
    //     // printk("\n");
    // }
    
    

    
}