/*
 * Copyright (c) 2018 Makaio GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/generic_uart/generic_uart_drv.h>
#include <device.h>
#include <logging/sys_log.h>
#include <gpio.h>
#include <string.h>
#include <net/net_if.h>
#include <net/net_context.h>
#include <net/tcp.h>
#include <net/net_offload.h>
/* TODO
#if (defined(CONFIG_LORA_UART0) && CONFIG_LORA_UART0 == 1) || (defined(CONFIG_LORA_UARTE0) && CONFIG_LORA_UARTE0 == 1)
#define LORA_DEV_UART_NAME CONFIG_UART_0_NAME
#elif defined(CONFIG_LORA_UARTE1) && CONFIG_LORA_UARTE1 == 1
#define LORA_DEV_UART_NAME CONFIG_UART_1_NAME
#endif
 */
#define MDM_DEV_UART_NAME CONFIG_UART_1_NAME
#define MDM_DEV_NAME "MODEM"
#define QUECTELMDM_CMD_TIMEOUT			K_SECONDS(5)
#define CONFIG_QUECTELMDM_INIT_PRIORITY 20
#define CONFIG_MODEM_APN_NAME "wm"
#ifdef CONFIG_NET_OFFLOAD
#undef CONFIG_NET_OFFLOAD
#endif


/* RX thread structures */
K_THREAD_STACK_DEFINE(quectelmdm_rx_stack,
                      CONFIG_MODEM_uart_dev_RX_STACK_SIZE);
struct k_thread quectelmdm_rx_thread;

K_THREAD_STACK_DEFINE(quectelmdm_workq_stack,
                      CONFIG_MODEM_uart_dev_RX_WORKQ_STACK_SIZE);
static struct k_work_q quectelmdm_workq;


struct modem_device_data {
    struct device *uart_device;
    struct uart_dev_ctx dev_ctx;


    struct net_if *iface;
    u8_t mac_addr[6];
};

static struct modem_device_data quectelmdm_data;

static bool mdm_comm_active = false;

static int on_initial_at_resp(uint8_t *buf, u16_t len)
{
    if(!strcmp("OK", buf))
    {
        mdm_comm_active = true;
        return 0;
    }
    return 1;
}

static int quectelmdm_check_regstatus(void)
{
    int ret = uart_dev_send_cmd(&quectelmdm_data.dev_ctx, "AT+CREG?",QUECTELMDM_CMD_TIMEOUT, NULL);

    return ret;
}

static int quectelmdm_check_gprsstatus(void)
{
    int ret = uart_dev_send_cmd(&quectelmdm_data.dev_ctx, "AT+CGATT?",QUECTELMDM_CMD_TIMEOUT, NULL);

    return ret;
}

static int quectelmdm_set_apn()
{
    int ret = uart_dev_send_cmd(&quectelmdm_data.dev_ctx, "AT+QIREGAPP=\"" CONFIG_MODEM_APN_NAME
    "\",\"\",\"\"",QUECTELMDM_CMD_TIMEOUT, NULL);

    return ret;
}

static int quectelmdm_attach_gprs(void)
{
    int ret = uart_dev_send_cmd(&quectelmdm_data.dev_ctx, "AT+CGATT=0",QUECTELMDM_CMD_TIMEOUT, NULL);

    return ret;
}

static int quectelmdm_enable_uplink(void)
{
    int ret = uart_dev_send_cmd(&quectelmdm_data.dev_ctx, "AT+CGATT=1",QUECTELMDM_CMD_TIMEOUT, NULL);

    return ret;
}

static int on_cmd_gprsstatus(uint8_t *buf, u16_t len)
{
    SYS_LOG_DBG("CGATT responded %s", buf);
    if(len > 0)
    {
        char reg_status = buf[0];

        if(reg_status == '1')
        {
            SYS_LOG_DBG("GPRS attached");
            quectelmdm_enable_uplink();
        } else if(reg_status == '0')
        {
            SYS_LOG_DBG("Attach to GPRS");
            quectelmdm_set_apn();
            quectelmdm_attach_gprs();
            quectelmdm_enable_uplink();
        }
    }
    return 0;
    //printk("cmd buf %s (%u)", buf, len);
}

static int on_cmd_regstatus(uint8_t *buf, u16_t len)
{
    if(len == 4)
    {
        char reg_status = buf[2];

        if(reg_status == '1' || reg_status == '5')
        {
            SYS_LOG_DBG("connected to carrier");

            quectelmdm_check_gprsstatus();

        }
    }
    return 0;
    //printk("cmd buf %s (%u)", buf, len);
}


static void quectelmdm_init_work(struct k_work *work)
{

    return;
    u32_t statValue = 0;

    /* Check if modem is already powered on - if not, trigger PWRKEY pin */
    struct device *gpio0_dev = device_get_binding(NORDIC_NRF_GPIO_50000000_LABEL);
    struct device *gpio1_dev = device_get_binding(NORDIC_NRF_GPIO_50000300_LABEL);
    gpio_pin_configure(gpio1_dev, 14, GPIO_DIR_IN);
    gpio_pin_read(gpio1_dev, 14, &statValue);

    gpio_pin_configure(gpio0_dev, 12, GPIO_DIR_OUT);

    SYS_LOG_DBG("pin val %u", statValue);
    if(!statValue)
    {
        SYS_LOG_DBG("MDM_RESET_PIN #%u -> ASSERTED",14);
        gpio_pin_write(gpio0_dev,
                       12, 1);
        k_sleep(K_SECONDS(1));
        SYS_LOG_DBG("MDM_RESET_PIN -> NOT_ASSERTED");
        gpio_pin_write(gpio0_dev,
                       12,0);
    }
    mdm_comm_active = false;
    uart_dev_send_cmd(&quectelmdm_data.dev_ctx, "AT",QUECTELMDM_CMD_TIMEOUT, on_initial_at_resp);


    if(mdm_comm_active)
    {
        /* communication looks ok */

        /* query registration status, handle in command handler */
        if(quectelmdm_check_regstatus() == 0)
        {

        }
    }
}


static void quectelmdm_init()
{
    int i, ret = 0;
SYS_LOG_DBG("quectelmdm_init");
    static struct k_delayed_work init_work;
    k_delayed_work_init(&init_work, quectelmdm_init_work);
    ret = k_delayed_work_submit_to_queue(&quectelmdm_workq,
                                        &init_work, K_MSEC(10));


}



const static struct cmd_handler handlers[] = {
        CMD_HANDLER("+CREG:", regstatus),
        CMD_HANDLER("+CGATT:", gprsstatus),
};

static int modem_device_init(struct device *dev)
{
    struct device* uart_device = device_get_binding(MDM_DEV_UART_NAME);
    //struct device* uart_device = dev;

    struct uart_dev_ctx dev_ctx;

    memset(&dev_ctx, 0, sizeof(struct uart_dev_ctx));

    dev_ctx.command_handlers = handlers;
    dev_ctx.command_handler_cnt = (uint8_t) ARRAY_SIZE(handlers);
    dev_ctx.generic_resp_handler = NULL;

    dev_ctx.workq = quectelmdm_workq;
    dev_ctx.workq_stack = quectelmdm_workq_stack;

    dev_ctx.rx_thread = quectelmdm_rx_thread;
    dev_ctx.rx_thread_stack = quectelmdm_rx_stack;

    quectelmdm_data.dev_ctx = dev_ctx;




/* RX thread work queue */
/*
    K_THREAD_STACK_DEFINE(uart_dev_workq_stack,
                          CONFIG_MODEM_uart_dev_RX_WORKQ_STACK_SIZE);
    static struct k_work_q uart_dev_workq;
  */

    uart_dev_init(&quectelmdm_data.dev_ctx, uart_device);
SYS_LOG_DBG("After uart_dev_init");
    SYS_LOG_DBG("Modem running at %s", uart_device->config->name);
    quectelmdm_data.uart_device = uart_device;
    //quectelmdm_init();

#ifdef CONFIG_NET_OFFLOAD
    net_if_up(quectelmdm_data.iface); // TODO
#else
    quectelmdm_init();
#endif
    return 0;
}



static inline u8_t *wncm14a2a_get_mac(struct device *dev)
{
    struct modem_device_data* ctx = &quectelmdm_data;

    ctx->mac_addr[0] = 0x00;
    ctx->mac_addr[1] = 0x10;

    UNALIGNED_PUT(sys_cpu_to_be32(sys_rand32_get()),
                  (u32_t *) ((void *)ctx->mac_addr+2));

    return ctx->mac_addr;
}



/*** OFFLOAD FUNCTIONS ***/
#if defined(CONFIG_NET_OFFLOAD)
static int offload_get(sa_family_t family,
                       enum net_sock_type type,
                       enum net_ip_protocol ip_proto,
                       struct net_context **context)
{
    int ret = 0;

    return ret;
}

static int offload_bind(struct net_context *context,
                        const struct sockaddr *addr,
                        socklen_t addrlen)
{

        return -EPFNOSUPPORT;

}

static int offload_listen(struct net_context *context, int backlog)
{
    /* NOT IMPLEMENTED */
    return -ENOTSUP;
}

static int offload_connect(struct net_context *context,
                           const struct sockaddr *addr,
                           socklen_t addrlen,
                           net_context_connect_cb_t cb,
                           s32_t timeout,
                           void *user_data)
{

        return -EINVAL;

}

static int offload_accept(struct net_context *context,
                          net_tcp_accept_cb_t cb,
                          s32_t timeout,
                          void *user_data)
{
    /* NOT IMPLEMENTED */
    return -ENOTSUP;
}

static int offload_sendto(struct net_pkt *pkt,
                          const struct sockaddr *dst_addr,
                          socklen_t addrlen,
                          net_context_send_cb_t cb,
                          s32_t timeout,
                          void *token,
                          void *user_data)
{
    int ret = 0;


    return ret;
}

static int offload_send(struct net_pkt *pkt,
                        net_context_send_cb_t cb,
                        s32_t timeout,
                        void *token,
                        void *user_data)
{

        return -EPFNOSUPPORT;

}

static int offload_recv(struct net_context *context,
                        net_context_recv_cb_t cb,
                        s32_t timeout,
                        void *user_data)
{

    return 0;
}

static int offload_put(struct net_context *context)
{

    return 0;
}

static struct net_offload offload_funcs = {
        .get = offload_get,
        .bind = offload_bind,
        .listen = offload_listen,	/* TODO */
        .connect = offload_connect,
        .accept = offload_accept,	/* TODO */
        .send = offload_send,
        .sendto = offload_sendto,
        .recv = offload_recv,
        .put = offload_put,
};


static void offload_iface_init(struct net_if *iface)
{
    struct device *dev = net_if_get_device(iface);
    SYS_LOG_DBG("offload_iface_init");

    if(!dev)
    {
        SYS_LOG_DBG("net dev not found");
    } else {
        //struct wncm14a2a_iface_ctx *ctx = dev->driver_data;
        struct modem_device_data ctx = quectelmdm_data;
        iface->if_dev->offload = &offload_funcs;
        net_if_set_link_addr(iface, wncm14a2a_get_mac(dev),
                             sizeof(ctx.mac_addr),
                             NET_LINK_ETHERNET);
        ctx.iface = iface;
    }


}

static struct net_if_api api_funcs = {
        .init	= offload_iface_init,
        .send	= NULL,
};



#define  MDM_MAX_DATA_LENGTH 1200 // TODO
#define CONFIG_QUECTELMDM_IFACE_INIT_PRIORITY 80 // TODO
NET_DEVICE_OFFLOAD_INIT(modem_quectel, "MODEM_QUECTEL", // TODO MODEM_QUECTEL = MDM_DEV_NAME
                        modem_device_init, &quectelmdm_data,
NULL, CONFIG_QUECTELMDM_IFACE_INIT_PRIORITY, &api_funcs,
MDM_MAX_DATA_LENGTH);
#else

DEVICE_AND_API_INIT(quectelmdm, MDM_DEV_NAME,
                    &modem_device_init, &quectelmdm_data,
                    NULL, POST_KERNEL, CONFIG_QUECTELMDM_INIT_PRIORITY,
                    NULL);
                    // TODO &rn2483_lora_api);

#endif


