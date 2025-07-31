/*
 * USB CDC ECM Network Sample for LPC54S018
 * 
 * This sample demonstrates USB networking using Zephyr's built-in
 * CDC ECM support. The device will enumerate as a USB Ethernet adapter
 * and run a simple TCP echo server on port 13400 (VCI port).
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/net/net_config.h>
#include <zephyr/net/socket.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(usb_net, LOG_LEVEL_INF);

/* Status LED */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* VCI TCP Port */
#define VCI_PORT 13400

static void setup_usb(void)
{
    int ret;

    /* Enable USB device */
    ret = usb_enable(NULL);
    if (ret != 0) {
        LOG_ERR("Failed to enable USB device: %d", ret);
        return;
    }

    LOG_INF("USB device enabled");
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

    /* Bind to port */
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(VCI_PORT);

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

    LOG_INF("TCP echo server listening on port %d", VCI_PORT);

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
            LOG_DBG("Received %d bytes: %s", len, buffer);

            /* Echo back */
            send(client_sock, buffer, len, 0);
        }

        LOG_INF("Client disconnected");
        close(client_sock);
    }
}

int main(void)
{
    int ret;

    LOG_INF("USB CDC ECM Network Sample");
    LOG_INF("Device IP: 192.168.7.1");
    LOG_INF("Connect USB cable and configure host PC:");
    LOG_INF("  Host IP: 192.168.7.2");
    LOG_INF("  Netmask: 255.255.255.0");

    /* Configure LED */
    if (device_is_ready(led.port)) {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED: %d", ret);
        }
    }

    /* Setup USB - Zephyr handles enumeration automatically */
    setup_usb();

    /* Wait for network to be ready */
    LOG_INF("Waiting for network configuration...");
    net_config_init_app(NULL, "USB CDC ECM");

    /* Turn on LED when network is ready */
    if (device_is_ready(led.port)) {
        gpio_pin_set_dt(&led, 1);
    }

    /* Start TCP echo server */
    tcp_echo_server();

    return 0;
}