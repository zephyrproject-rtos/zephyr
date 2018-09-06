/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#if defined(CONFIG_LORA_UART0) || defined(CONFIG_LORA_UARTE0)
    #ifndef CONFIG_UART_0_NAME
        #error UART_0 / UARTE_0 peripheral not found
    #endif
#elif defined(CONFIG_LORA_UARTE1)
    #ifndef CONFIG_UART_1_NAME
        #error UARTE_1 peripheral not found
    #endif
#endif

void lora_device_init(void)
{


}

void lora_device_reset(void)
{

}