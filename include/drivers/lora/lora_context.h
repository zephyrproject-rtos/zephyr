/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LORA_CONTEXT_H
#define LORA_CONTEXT_H
#include <zephyr/types.h>
#include <stddef.h>
#include <kernel.h>
#ifdef __cplusplus
extern "C" {
#endif

struct lora_context_cb {
    /* device is ready to accept commands */
    //void (*ready)(u8_t err);
    void (*ready)(bool success);
};

struct k_sem lora_uart_rx_sem;

typedef void (*lora_device_get_version_cb)(u8_t err);

void lora_context_init(struct lora_context_cb *cb);

void lora_device_init(void);

void lora_device_uart_init(const char* newLine);

void lora_device_get_version(lora_device_get_version_cb *cb);

#define LORA_RX_STACK_SIZE 1028 // TODO
#define LORA_RX_WORKQ_STACK_SIZE 2048 // TODO



#ifdef __cplusplus
}
#endif
#endif
