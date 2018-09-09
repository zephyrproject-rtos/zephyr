/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/lora/lora_context.h>
#include <drivers/generic_uart/generic_uart_drv.h>
#include <init.h>
#include <logging/sys_log.h>

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
#define ASSIGNED_UART_PERIPHERAL CONFIG_UART_0_NAME
#elif defined(CONFIG_LORA_UARTE1) && CONFIG_LORA_UARTE1 == 1
#define ASSIGNED_UART_PERIPHERAL CONFIG_UART_1_NAME
#endif

#define LORA_CMD_TIMEOUT			K_SECONDS(5)

static int on_cmd_uartcmd_init(uint8_t *buf, u16_t len)
{
    SYS_LOG_DBG("%s",buf);
    return 0;
    //printk("cmd buf %s (%u)", buf, len);
}

static int on_cmd_response(uint8_t *buf, u16_t len)
{
    SYS_LOG_DBG("resp: %s",buf);
    /* don't handle any response */
    return 1;
}

static int on_get_hweui(uint8_t *buf, u16_t len)
{
    SYS_LOG_DBG("hweui: %s (%u bytes)",buf, len);
    return (len != 16);
}

const static struct cmd_handler handlers[] = {
CMD_HANDLER("RN2483", uartcmd_init),
};

void rn2483_init()
{
    uart_dev_send_cmd("sys get ver",LORA_CMD_TIMEOUT, NULL);

    uart_dev_send_cmd("sys get hweui", LORA_CMD_TIMEOUT, on_get_hweui);


}

void lora_device_init(void)
{
    struct device* uart_device = device_get_binding(ASSIGNED_UART_PERIPHERAL);
    uart_dev_init(uart_device, handlers, (uint8_t) ARRAY_SIZE(handlers), on_cmd_response);

    rn2483_init();
}

void lora_device_reset(void)
{

}