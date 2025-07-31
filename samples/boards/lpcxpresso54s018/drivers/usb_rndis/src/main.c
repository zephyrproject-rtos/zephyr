/*
 * USB RNDIS Virtual Network Interface Sample
 * Based on LPC54S018M VCI implementation
 *
 * This creates a USB composite device with:
 * - CDC RNDIS for virtual network interface
 * - CDC ACM for virtual COM port
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usb_rndis, LOG_LEVEL_INF);

/* LED for status indication */
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});

/* Network management callback */
static struct net_mgmt_event_callback mgmt_cb;

/* VCI TCP server port (matching baremetal) */
#define VCI_TCP_PORT 13400

static void network_event_handler(struct net_mgmt_event_callback *cb,
                                 uint32_t mgmt_event, struct net_if *iface)
{
    if (mgmt_event == NET_EVENT_IPV4_ADDR_ADD) {
        char buf[NET_IPV4_ADDR_LEN];
        struct net_if_config *cfg = net_if_get_config(iface);
        
        if (cfg->ip.ipv4) {
            struct net_if_addr *unicast = &cfg->ip.ipv4->unicast[0];
            net_addr_ntop(AF_INET, &unicast->address.in_addr, buf, sizeof(buf));
            LOG_INF("Network interface ready, IP address: %s", buf);
            
            /* Turn on LED to indicate network is ready */
            if (gpio_is_ready_dt(&led)) {
                gpio_pin_set_dt(&led, 1);
            }
        }
    }
}

/* Simple echo server for testing */
static void tcp_server_thread(void *p1, void *p2, void *p3)
{
    ARG_UNUSED(p1);
    ARG_UNUSED(p2);
    ARG_UNUSED(p3);
    
    int ret;
    struct sockaddr_in addr;
    int serv_sock, client_sock;
    char buffer[256];
    
    /* Wait for network to be ready */
    k_sleep(K_SECONDS(2));
    
    /* Create socket */
    serv_sock = zsock_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock < 0) {
        LOG_ERR("Failed to create socket: %d", errno);
        return;
    }
    
    /* Bind to VCI port */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(VCI_TCP_PORT);
    
    ret = zsock_bind(serv_sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERR("Failed to bind socket: %d", errno);
        zsock_close(serv_sock);
        return;
    }
    
    /* Listen for connections */
    ret = zsock_listen(serv_sock, 1);
    if (ret < 0) {
        LOG_ERR("Failed to listen: %d", errno);
        zsock_close(serv_sock);
        return;
    }
    
    LOG_INF("VCI TCP server listening on port %d", VCI_TCP_PORT);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        
        /* Accept connection */
        client_sock = zsock_accept(serv_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0) {
            LOG_ERR("Failed to accept connection: %d", errno);
            continue;
        }
        
        LOG_INF("Client connected");
        
        /* Handle client */
        while (1) {
            int len = zsock_recv(client_sock, buffer, sizeof(buffer) - 1, 0);
            if (len <= 0) {
                break;
            }
            
            buffer[len] = '\0';
            LOG_DBG("Received: %s", buffer);
            
            /* Echo back */
            zsock_send(client_sock, buffer, len, 0);
        }
        
        LOG_INF("Client disconnected");
        zsock_close(client_sock);
    }
}

K_THREAD_DEFINE(tcp_server, 2048, tcp_server_thread, NULL, NULL, NULL, 5, 0, 0);

int main(void)
{
    int ret;
    
    LOG_INF("USB RNDIS VCI starting...");
    
    /* Configure LED */
    if (gpio_is_ready_dt(&led)) {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED: %d", ret);
        }
    }
    
    /* Register network event handler */
    net_mgmt_init_event_callback(&mgmt_cb, network_event_handler,
                                 NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&mgmt_cb);
    
    /* Enable USB */
    ret = usb_enable(NULL);
    if (ret != 0) {
        LOG_ERR("Failed to enable USB: %d", ret);
        return ret;
    }
    
    LOG_INF("USB RNDIS device enabled");
    LOG_INF("Connect to PC and configure network:");
    LOG_INF("  - Device IP: 192.168.7.1");
    LOG_INF("  - Host IP: 192.168.7.2");
    LOG_INF("  - VCI port: %d", VCI_TCP_PORT);
    
    /* Main loop - blink LED slowly when USB not configured */
    while (1) {
        if (gpio_is_ready_dt(&led)) {
            struct net_if *iface = net_if_get_default();
            if (iface && net_if_is_up(iface)) {
                /* Network is up, LED stays on */
                gpio_pin_set_dt(&led, 1);
            } else {
                /* Blink slowly */
                gpio_pin_toggle_dt(&led);
            }
        }
        k_sleep(K_MSEC(500));
    }
    
    return 0;
}