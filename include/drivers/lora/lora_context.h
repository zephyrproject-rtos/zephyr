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
#include <device.h>
#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_LORA_UART0) || defined(CONFIG_LORA_UARTE0)
#ifndef CONFIG_UART_0_NAME
        #error UART_0 / UARTE_0 peripheral not found
    #endif
#elif defined(CONFIG_LORA_UARTE1)
#ifndef CONFIG_UART_1_NAME
        #error UARTE_1 peripheral not found
    #endif
#endif


#if (defined(CONFIG_LORA_UART0) && CONFIG_LORA_UART0 == 1) || (defined(CONFIG_LORA_UARTE0) && CONFIG_LORA_UARTE0 == 1)
#define LORA_DEV_UART_NAME CONFIG_UART_0_NAME
#elif defined(CONFIG_LORA_UARTE1) && CONFIG_LORA_UARTE1 == 1
#define LORA_DEV_UART_NAME CONFIG_UART_1_NAME
#endif

#define LORA_DEV_NAME "LORA"
#define LORA_LORAWAN 1


struct lora_context_cb {
    /* device is ready to accept commands */
    //void (*ready)(u8_t err);
    void (*ready)(bool success);

#ifdef LORA_LORAWAN
    void (*joined)(void);
#endif
};

typedef int (*lora_driver_api_join)(struct device *dev);
typedef int (*lora_driver_api_send)(struct device *dev);
typedef void (*lora_driver_cb_register)(struct device *dev, struct lora_context_cb *cb);
typedef int (*lora_drv_key_set_devaddr)(uint8_t devaddr[4]);
typedef int (*lora_drv_key_set_deveui)(uint8_t deveui[8]);
typedef int (*lora_drv_key_set_appeui)(uint8_t appeui[8]);
typedef int (*lora_drv_key_set_nwkskey)(uint8_t nwkskey[16]);
typedef int (*lora_drv_key_set_appskey)(uint8_t appskey[16]);
typedef int (*lora_drv_key_set_appkey)(uint8_t appkey[16]);

struct lora_driver_api {
    lora_driver_api_send send;
    lora_driver_cb_register callback_register;
#ifdef LORA_LORAWAN
    lora_driver_api_join join;

    lora_drv_key_set_devaddr key_set_devaddr;
    lora_drv_key_set_deveui key_set_deveui;
    lora_drv_key_set_appeui key_set_appeui;
    lora_drv_key_set_nwkskey key_set_nwkskey;
    lora_drv_key_set_appskey key_set_appskey;
    lora_drv_key_set_appkey key_set_appkrey;
#endif
};


struct k_sem lora_uart_rx_sem;

typedef void (*lora_device_get_version_cb)(u8_t err);

void lora_context_init(struct lora_context_cb *cb);

//void lora_device_init(void);

void lora_device_uart_init(const char* newLine);

void lora_device_get_version(lora_device_get_version_cb *cb);

#define LORA_RX_STACK_SIZE 1028 // TODO
#define LORA_RX_WORKQ_STACK_SIZE 2048 // TODO



#ifdef __cplusplus
}
#endif
#endif
