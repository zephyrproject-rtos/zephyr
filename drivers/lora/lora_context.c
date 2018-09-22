/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/lora/lora_context.h>
#define SYS_LOG_DOMAIN "lora"

static struct lora_context_cb *callback_list;

void lora_context_init(struct lora_context_cb *cb)
{
    callback_list = cb;

    //lora_device_init();
    /*
#ifdef CONFIG_LORA_DEVICE_USES_UARTE
    lora_context_uart_init();
#else
#error No LoRa device implementation found
#endif*/
}

