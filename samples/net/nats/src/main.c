/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_nats_sample, LOG_LEVEL_DBG);

#include <drivers/gpio.h>
#include <net/net_context.h>
#include <net/net_core.h>
#include <net/net_if.h>
#include <zephyr.h>

#include "nats.h"

/* LED */
#ifndef DT_ALIAS_LED0_GPIOS_CONTROLLER
#ifdef LED0_GPIO_PORT
#define DT_ALIAS_LED0_GPIOS_CONTROLLER 	LED0_GPIO_PORT
#else
#define DT_ALIAS_LED0_GPIOS_CONTROLLER "(fail)"
#define DT_ALIAS_LED0_GPIOS_PIN 0
#endif
#endif

#define LED_GPIO_NAME DT_ALIAS_LED0_GPIOS_CONTROLLER
#define LED_PIN DT_ALIAS_LED0_GPIOS_PIN

static struct device *led0;
static bool fake_led;

/* Network Config */

#if defined(CONFIG_NET_IPV6)

#define NATS_AF_INET		AF_INET6
#define NATS_SOCKADDR_IN	sockaddr_in6

#if defined(CONFIG_NET_CONFIG_MY_IPV6_ADDR)
#define NATS_LOCAL_IP_ADDR	CONFIG_NET_CONFIG_MY_IPV6_ADDR
#else
#define NATS_LOCAL_IP_ADDR	"2001:db8::1"
#endif /* CONFIG_NET_CONFIG_MY_IPV6_ADDR */

#if defined(CONFIG_NET_CONFIG_PEER_IPV6_ADDR)
#define NATS_PEER_IP_ADDR	CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#else
#define NATS_PEER_IP_ADDR	"2001:db8::2"
#endif /* CONFIG_NET_CONFIG_PEER_IPV6_ADDR */

#else /* CONFIG_NET_IPV4 */

#define NATS_AF_INET		AF_INET
#define NATS_SOCKADDR_IN	sockaddr_in

#if defined(CONFIG_NET_CONFIG_MY_IPV4_ADDR)
#define NATS_LOCAL_IP_ADDR	CONFIG_NET_CONFIG_MY_IPV4_ADDR
#else
#define NATS_LOCAL_IP_ADDR	"192.168.0.1"
#endif /* CONFIG_NET_CONFIG_MY_IPV4_ADDR */

#if defined(CONFIG_NET_CONFIG_PEER_IPV4_ADDR)
#define NATS_PEER_IP_ADDR	CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#else
#define NATS_PEER_IP_ADDR	"192.168.0.2"
#endif /* CONFIG_NET_CONFIG_PEER_IPV4_ADDR */

#endif

/* DNS API */
#define DNS_PORT		53
#define DNS_SLEEP_MSECS		400

/* Default server */
#define DEFAULT_PORT		4222

static void panic(const char *msg)
{
	LOG_ERR("Panic: %s", msg);
	for (;;) {
		k_sleep(K_FOREVER);
	}
}

static int in_addr_set(sa_family_t family,
		       const char *ip_addr,
		       int port,
		       struct sockaddr *_sockaddr)
{
	int rc = 0;

	_sockaddr->sa_family = family;

	if (ip_addr) {
		if (family == AF_INET6) {
			rc = net_addr_pton(family,
					   ip_addr,
					   &net_sin6(_sockaddr)->sin6_addr);
		} else {
			rc = net_addr_pton(family,
					   ip_addr,
					   &net_sin(_sockaddr)->sin_addr);
		}

		if (rc < 0) {
			LOG_ERR("Invalid IP address: %s", log_strdup(ip_addr));
			return -EINVAL;
		}
	}

	if (port >= 0) {
		if (family == AF_INET6) {
			net_sin6(_sockaddr)->sin6_port = htons(port);
		} else {
			net_sin(_sockaddr)->sin_port = htons(port);
		}
	}

	return rc;
}

static void initialize_network(void)
{
	struct net_if *iface;

	LOG_INF("Initializing network");

	iface = net_if_get_default();
	if (!iface) {
		panic("No default network interface");
	}

	/* TODO: IPV6 DHCP */
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_DHCPV4)
	net_dhcpv4_start(iface);

	/* delay so DHCPv4 can assign IP */
	/* TODO: add a timeout/retry */
	LOG_INF("Waiting for DHCP ...");
	do {
		k_sleep(K_SECONDS(1));
	} while (net_ipv4_is_addr_unspecified(&iface->dhcpv4.requested_ip));

	LOG_INF("Done!");

	/* TODO: add a timeout */
	LOG_INF("Waiting for IP assginment ...");
	do {
		k_sleep(K_SECONDS(1));
	} while (!net_ipv4_is_my_addr(&iface->dhcpv4.requested_ip));

	LOG_INF("Done!");
#else
	struct sockaddr addr;

	if (in_addr_set(NATS_AF_INET, NATS_LOCAL_IP_ADDR, 0,
			  &addr) < 0) {
		LOG_ERR("Invalid IP address: %s",
			NATS_LOCAL_IP_ADDR);
	}

#if defined(CONFIG_NET_IPV6)
	net_if_ipv6_addr_add(iface,
			     &net_sin6(&addr)->sin6_addr,
			     NET_ADDR_MANUAL, 0);
#else
	net_if_ipv4_addr_add(iface,
			     &net_sin(&addr)->sin_addr,
			     NET_ADDR_MANUAL, 0);
#endif
#endif /* CONFIG_NET_IPV4 && CONFIG_NET_DHCPV4 */
}

static bool read_led(void)
{
	u32_t led = 0U;
	int r;

	if (!led0) {
		return fake_led;
	}

	r = gpio_pin_read(led0, LED_PIN, &led);
	if (r < 0) {
		return false;
	}

	return !led;
}

static void write_led(const struct nats *nats,
		      const struct nats_msg *msg,
		      bool state)
{
	char *pubstate;
	int ret;

	if (!led0) {
		fake_led = state;
	} else {
		gpio_pin_write(led0, LED_PIN, !state);
	}

	pubstate = state ? "on" : "off";
	ret = nats_publish(nats, "led0", 0, msg->reply_to, 0,
			   pubstate, strlen(pubstate));
	if (ret < 0) {
		printk("Failed to publish: %d\n", ret);
	} else {
		printk("*** Turning LED %s\n", pubstate);
	}
}

static int on_msg_received(const struct nats *nats,
			   const struct nats_msg *msg)
{
	if (!strcmp(msg->subject, "led0")) {
		if (msg->payload_len == 2 && !strcmp(msg->payload, "on")) {
			write_led(nats, msg, true);
			return 0;
		}

		if (msg->payload_len == 3 && !strcmp(msg->payload, "off")) {
			write_led(nats, msg, false);
			return 0;
		}

		if (msg->payload_len == 6 && !strcmp(msg->payload, "toggle")) {
			write_led(nats, msg, !read_led());
			return 0;
		}

		return -EINVAL;
	}

	return -ENOENT;
}

static void initialize_hardware(void)
{
	LOG_INF("Initializing hardware");

	led0 = device_get_binding(LED_GPIO_NAME);
	if (led0) {
		gpio_pin_configure(led0, LED_PIN, GPIO_DIR_OUT);
	}
}

static int connect(struct nats *nats, u16_t port)
{
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_DHCPV4)
	struct net_if *iface;
#endif
	struct sockaddr dst_addr, src_addr;
	int ret;

	LOG_INF("Connecting...");

	ret = net_context_get(NATS_AF_INET, SOCK_STREAM, IPPROTO_TCP,
			      &nats->conn);
	if (ret < 0) {
		LOG_DBG("Could not get new context: %d", ret);
		return ret;
	}

	/* TODO: IPV6 DHCP */
#if defined(CONFIG_NET_IPV4) && defined(CONFIG_NET_DHCPV4)
	iface = net_if_get_default();

	net_ipaddr_copy(&net_sin(&src_addr)->sin_addr,
			&iface->dhcpv4.requested_ip);
	ret = in_addr_set(NATS_AF_INET, NULL, 0, &src_addr);
#else
	ret = in_addr_set(NATS_AF_INET, NATS_LOCAL_IP_ADDR,
			  0, &src_addr);
	if (ret < 0) {
		goto connect_exit;
	}
#endif

	ret = in_addr_set(NATS_AF_INET, NATS_PEER_IP_ADDR,
			  port, &dst_addr);
	if (ret < 0) {
		goto connect_exit;
	}

	ret = net_context_bind(nats->conn, &src_addr,
			       sizeof(struct NATS_SOCKADDR_IN));
	if (ret < 0) {
		LOG_DBG("Could not bind to local address: %d", -ret);
		goto connect_exit;
	}

	ret = nats_connect(nats, &dst_addr, sizeof(struct NATS_SOCKADDR_IN));
	if (!ret) {
		return 0;
	}

connect_exit:
	net_context_put(nats->conn);
	return ret;
}

static void nats_client(void)
{
	struct nats nats = {
		.on_message = on_msg_received
	};

	LOG_INF("NATS Client Sample");

	initialize_network();
	initialize_hardware();

	if (connect(&nats, DEFAULT_PORT) < 0) {
		panic("Could not connect to NATS server");
	}

	if (nats_subscribe(&nats, "led0", 0, NULL, 0,
			   "sub132984012384098", 0) < 0) {
		panic("Could not subscribe to `led0` topic");
	}
}

void main(void)
{
	nats_client();
}
