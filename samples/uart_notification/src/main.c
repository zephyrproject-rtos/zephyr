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

const struct device * dev = DEVICE_DT_GET(DT_NODELABEL(uart1));

int main()
{
    unsigned char *p_char;
    printk("UART testing begin...\n");
    

    
}