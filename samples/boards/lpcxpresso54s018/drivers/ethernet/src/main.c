/*
 * Ethernet Sample for LPC54S018
 * 
 * This sample demonstrates Ethernet networking using the built-in
 * ENET controller with LAN8720A PHY in RMII mode.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(eth_sample, LOG_LEVEL_INF);

/* Status LED */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* PHY Power Control GPIO */
#define PHY_POWER_NODE DT_NODELABEL(gpio3)
#define PHY_POWER_PIN 3
static const struct gpio_dt_spec phy_power = {
    .port = DEVICE_DT_GET(PHY_POWER_NODE),
    .pin = PHY_POWER_PIN,
    .dt_flags = GPIO_ACTIVE_HIGH
};

/* Network management callback */
static struct net_mgmt_event_callback mgmt_cb;

/* TCP echo server port */
#define ECHO_PORT 7

static void network_event_handler(struct net_mgmt_event_callback *cb,
                                 uint32_t mgmt_event, struct net_if *iface)
{
    if ((mgmt_event & NET_EVENT_IPV4_ADDR_ADD) ||
        (mgmt_event & NET_EVENT_IPV4_DHCP_BOUND)) {
        char buf[NET_IPV4_ADDR_LEN];
        struct net_if_config *cfg = net_if_get_config(iface);
        
        if (cfg->ip.ipv4) {
            struct net_if_addr *unicast = &cfg->ip.ipv4->unicast[0];
            net_addr_ntop(AF_INET, &unicast->address.in_addr, buf, sizeof(buf));
            LOG_INF("Ethernet ready, IP address: %s", buf);
            
            /* Turn on LED to indicate network is ready */
            if (gpio_is_ready_dt(&led)) {
                gpio_pin_set_dt(&led, 1);
            }
        }
    }
}

static void tcp_echo_server(void)
{
    int serv_sock, client_sock;
    struct sockaddr_in addr;
    char buffer[256];
    int ret;

    /* Create socket */
    serv_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock < 0) {
        LOG_ERR("Failed to create socket: %d", errno);
        return;
    }

    /* Bind to echo port */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(ECHO_PORT);

    ret = bind(serv_sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret < 0) {
        LOG_ERR("Failed to bind: %d", errno);
        close(serv_sock);
        return;
    }

    /* Listen */
    ret = listen(serv_sock, 1);
    if (ret < 0) {
        LOG_ERR("Failed to listen: %d", errno);
        close(serv_sock);
        return;
    }

    LOG_INF("TCP echo server listening on port %d", ECHO_PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        /* Accept connection */
        client_sock = accept(serv_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            LOG_ERR("Failed to accept: %d", errno);
            continue;
        }

        LOG_INF("Client connected");

        /* Echo loop */
        while (1) {
            int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
            if (len <= 0) {
                break;
            }

            buffer[len] = '\0';
            LOG_DBG("Received %d bytes", len);

            /* Echo back */
            send(client_sock, buffer, len, 0);
        }

        LOG_INF("Client disconnected");
        close(client_sock);
    }
}

static void enable_phy_power(void)
{
    int ret;

    if (!device_is_ready(phy_power.port)) {
        LOG_ERR("PHY power GPIO not ready");
        return;
    }

    ret = gpio_pin_configure_dt(&phy_power, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure PHY power pin: %d", ret);
        return;
    }

    /* Enable PHY power */
    gpio_pin_set_dt(&phy_power, 1);
    LOG_INF("PHY power enabled");

    /* Wait for PHY to power up */
    k_sleep(K_MSEC(100));
}

int main(void)
{
    int ret;

    LOG_INF("Ethernet Sample for LPC54S018");

    /* Configure LED */
    if (device_is_ready(led.port)) {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED: %d", ret);
        }
    }

    /* Enable PHY power */
    enable_phy_power();

    /* Register network event handler */
    net_mgmt_init_event_callback(&mgmt_cb, network_event_handler,
                                 NET_EVENT_IPV4_ADDR_ADD |
                                 NET_EVENT_IPV4_DHCP_BOUND);
    net_mgmt_add_event_callback(&mgmt_cb);

    /* Wait for network to be ready */
    LOG_INF("Waiting for network configuration...");
    net_config_init_app(NULL, "Ethernet Sample");

    /* Start TCP echo server */
    tcp_echo_server();

    return 0;
}