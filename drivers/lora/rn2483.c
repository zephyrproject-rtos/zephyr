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
#include <string.h>
#include <stdio.h>


#define LORA_CMD_TIMEOUT			K_SECONDS(5)
#define RN2483_GENERIC_ERROR        "invalid_param"
#define RN2483_GENERIC_SUCCESS      "ok"
static struct lora_context_cb *lora_callbacks;

static bool rn2483_key_set_failed = true;

/* RX thread structures */
K_THREAD_STACK_DEFINE(rn2483_rx_stack,
                      CONFIG_MODEM_uart_dev_RX_STACK_SIZE);
struct k_thread rn2483_rx_thread;

K_THREAD_STACK_DEFINE(rn2483_workq_stack,
                      CONFIG_MODEM_uart_dev_RX_WORKQ_STACK_SIZE);
static struct k_work_q rn2483_workq;


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

static int rn2483_on_key_set(uint8_t *buf, u16_t len)
{
   // SYS_LOG_DBG("rn2483_on_key_set: %s (%u bytes)",buf, len);
    if(strcmp(buf, RN2483_GENERIC_ERROR) == 0)
    {
        rn2483_key_set_failed = true;
    } else if(strcmp(buf, RN2483_GENERIC_SUCCESS) == 0) {
        rn2483_key_set_failed = false;
    }
    return 0;
}

const static struct cmd_handler handlers[] = {
CMD_HANDLER("RN2483", uartcmd_init),
};



struct rn2483_data {
    struct device *uart_device;
    struct uart_dev_ctx dev_ctx;
};

static struct rn2483_data rn2483_lora_data;

void rn2483_init()
{
    uart_dev_send_cmd(&rn2483_lora_data.dev_ctx, "sys get ver",LORA_CMD_TIMEOUT, NULL);

  //  uart_dev_send_cmd(&rn2483_lora_data.dev_ctx, "sys get hweui", LORA_CMD_TIMEOUT, on_get_hweui);

}

int rn2483_key_set(const char* key_name, uint8_t* key_value)
{
    char base_cmd[] = "mac set ";

    size_t key_hex_size = (sizeof(key_value)*2)+1;

    char* key_in_hex = k_malloc(key_hex_size);
    memset(key_in_hex,0, key_hex_size);
    key_in_hex[key_hex_size-1] = 0;
    for(uint8_t j = 0; j < sizeof(key_value); j++)
        sprintf(&key_in_hex[2*j], "%02X", key_value[j]);

    size_t cmd_size = strlen(base_cmd) + strlen(key_name) + 1 + strlen(key_in_hex);
    char* cmd = k_malloc(cmd_size);

    strcpy(cmd, base_cmd);
    strcat(cmd, key_name);
    strcat(cmd, " ");
    strcat(cmd, key_in_hex);
    rn2483_key_set_failed = true;
    uart_dev_send_cmd(&rn2483_lora_data.dev_ctx, cmd, LORA_CMD_TIMEOUT, rn2483_on_key_set);

    k_free(cmd);
    k_free(key_in_hex);

    if(rn2483_key_set_failed)
    {
        return -1;
    } else {
        return 0;
    }
   // rn2483_on_key_set
}

int rn2483_key_set_devaddr (uint8_t devaddr[4])
{
    return rn2483_key_set("devaddr", devaddr);
}

int rn2483_key_set_deveui (uint8_t deveui[8])
{
    return rn2483_key_set("deveui", deveui);
}
int rn2483_key_set_appeui (uint8_t appeui[8])
{
    return rn2483_key_set("appeui", appeui);
}

int rn2483_key_set_nwkskey (uint8_t nwkskey[16])
{
    return rn2483_key_set("nwkskey", nwkskey);
}
int rn2483_key_set_appskey (uint8_t appskey[16])
{
    return rn2483_key_set("appskey", appskey);
}
int rn2483_key_set_appkey (uint8_t appkey[16])
{
    return rn2483_key_set("appkey", appkey);
}

static int rn2483_join(struct device *dev)
{
    uint8_t devaddr[4];
    devaddr[0] = 0xaa;
    devaddr[1] = 0xbb;
    devaddr[2] = 0xcc;
    devaddr[3] = 0xdd;

    int key_ret = rn2483_key_set_devaddr(devaddr);
    SYS_LOG_DBG("Key set ret: %u", key_ret);
    lora_callbacks->joined();
    return 0;

}


static void rn2483_cb_register(struct device *dev, struct lora_context_cb *cb)
{
    lora_callbacks = cb;
}


static int rn2483_send(struct device *dev)
{
    return -1;
}

static int rn2483_pm_control(struct device *dev, u32_t unused_ctrl_command, void *unused_context)
{
    SYS_LOG_DBG("pm control");
    return 0;
}


static const struct lora_driver_api rn2483_lora_api = {
        .join = rn2483_join,
        .send = rn2483_send,
        .callback_register = rn2483_cb_register,

#ifdef LORA_LORAWAN
        .key_set_devaddr = NULL,
        .key_set_deveui = NULL,
        .key_set_appeui = NULL,
        .key_set_nwkskey = NULL,
        .key_set_appskey = NULL,
        .key_set_appkrey = NULL
#endif
};





static int lora_device_init(struct device *dev)
{

    struct device* uart_device = device_get_binding(LORA_DEV_UART_NAME);
    struct uart_dev_ctx dev_ctx = {0};

    dev_ctx.command_handlers = handlers;
    dev_ctx.command_handler_cnt = (uint8_t) ARRAY_SIZE(handlers);
    dev_ctx.generic_resp_handler = on_cmd_response;

    dev_ctx.workq = rn2483_workq;
    dev_ctx.workq_stack = rn2483_workq_stack;

    dev_ctx.rx_thread = rn2483_rx_thread;
    dev_ctx.rx_thread_stack = rn2483_rx_stack;


    rn2483_lora_data.dev_ctx = dev_ctx;

    uart_dev_init(&rn2483_lora_data.dev_ctx, uart_device);

    //dev->driver_api = &rn2483_lora_api;
    rn2483_lora_data.uart_device = uart_device;
    rn2483_init();

    return 0;
}



void lora_device_reset(void)
{

}

//#define LORA_DEV_NAME "RN2483" //todo
#define CONFIG_LORA_INIT_PRIORITY 20
/*
SYS_DEVICE_DEFINE(LORA_DEV_NAME,  lora_device_init, rn2483_pm_control, POST_KERNEL,
                  CONFIG_LORA_INIT_PRIORITY);
*/

DEVICE_AND_API_INIT(rn2483, LORA_DEV_NAME,
                    &lora_device_init, &rn2483_lora_data,
                    NULL, POST_KERNEL, CONFIG_LORA_INIT_PRIORITY,
                    &rn2483_lora_api);