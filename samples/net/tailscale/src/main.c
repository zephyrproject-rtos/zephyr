/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/net/tailscale.h>
#include <zephyr/kernel.h>
#include <zephyr/net/dhcpv4.h>
#include <zephyr/net/dns_resolve.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/sntp.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/sys/clock.h>

static int load_trust_anchor(sec_tag_t tag)
{
#ifdef TAILSCALE_SAMPLE_EMBEDDED_CA
	static const uint8_t ca[] = {
#include "tailscale_ca.inc"
	};

	return tls_credential_add(tag, TLS_CREDENTIAL_CA_CERTIFICATE, ca, sizeof(ca));
#else
	ARG_UNUSED(tag);
	printk("Build with TAILSCALE_CA_FILE to provision the control server's trust anchor\n");
	return -ENOENT;
#endif
}

static void show_login(const char *url, void *context)
{
	ARG_UNUSED(context);
	if (IS_ENABLED(CONFIG_SAMPLE_TAILSCALE_PRIVATE_CONSOLE)) {
		printk("Authorize this node on the private console: %s\n", url);
	}
}

static int start_underlay(struct net_if *iface)
{
	int rc = 0;

	net_dhcpv4_start(iface);
#ifdef CONFIG_WIFI_CREDENTIALS_CONNECT_STORED
	rc = net_mgmt(NET_REQUEST_WIFI_CONNECT_STORED, iface, NULL, 0);
#endif
	if (rc != 0) {
		return rc;
	}
	for (int i = 0; i < 120; i++) {
		if (net_if_ipv4_get_global_addr(iface, NET_ADDR_PREFERRED) != NULL) {
			return 0;
		}
		k_sleep(K_MSEC(250));
	}
	return -ETIMEDOUT;
}

static int exchange_echo(struct zsock_addrinfo *address, int type, uint16_t port, uint8_t request)
{
	const struct zsock_timeval timeout = {.tv_sec = 5};
	struct net_sockaddr *peer = address->ai_addr;
	bool ipv6 = address->ai_family == NET_AF_INET6;
	const char *protocol = type == NET_SOCK_DGRAM ? "UDP" : "TCP";
	uint8_t reply;
	ssize_t count;
	int rc;

	if (ipv6) {
		net_sin6(peer)->sin6_port = net_htons(port);
	} else {
		net_sin(peer)->sin_port = net_htons(port);
	}
	int fd = zsock_socket(address->ai_family, type, 0);

	if (fd < 0) {
		return -errno;
	}

	rc = zsock_setsockopt(fd, ZSOCK_SOL_SOCKET, ZSOCK_SO_RCVTIMEO, &timeout, sizeof(timeout));
	if (rc < 0) {
		rc = -errno;
		printk("Tailscale %s echo timeout setup failed (%d)\n", protocol, rc);
		goto out;
	}
	if (zsock_connect(fd, peer, address->ai_addrlen) < 0) {
		rc = -errno;
		printk("Tailscale %s echo connect failed (%d)\n", protocol, rc);
		goto out;
	}
	count = zsock_send(fd, &request, sizeof(request), 0);
	if (count != sizeof(request)) {
		rc = count < 0 ? -errno : -EIO;
		printk("Tailscale %s echo send failed (%d)\n", protocol, rc);
		goto out;
	}
	count = zsock_recv(fd, &reply, sizeof(reply), 0);
	if (count != sizeof(reply) || reply != request) {
		rc = count < 0 ? -errno : -EBADMSG;
		printk("Tailscale %s echo receive failed (%d)\n", protocol, rc);
	} else {
		printk("Tailscale IPv%d %s echo passed\n", ipv6 ? 6 : 4, protocol);
	}
out:
	zsock_close(fd);
	return rc;
}

static int example_echo(int family)
{
	const struct zsock_addrinfo hints = {.ai_family = family};
	struct zsock_addrinfo *result;
	int rc = zsock_getaddrinfo(CONFIG_SAMPLE_TAILSCALE_PEER, NULL, &hints, &result);

	if (family == NET_AF_INET6 && (rc == DNS_EAI_NONAME || rc == DNS_EAI_NODATA)) {
		return 0;
	}
	if (rc != 0) {
		printk("Tailscale peer name lookup failed (%d)\n", rc);
		return -EHOSTUNREACH;
	}
	/* The first datagram can be lost while WireGuard establishes the path. */
	for (int attempt = 0; attempt < 3; attempt++) {
		rc = exchange_echo(result, NET_SOCK_DGRAM, 9000, 2);
		if (rc != -EAGAIN && rc != -ETIMEDOUT) {
			break;
		}
	}
	if (rc == 0) {
		rc = exchange_echo(result, NET_SOCK_STREAM, 9001, 1);
	}
	zsock_freeaddrinfo(result);
	return rc;
}

static int start_client(struct tailscale_client_config *config, struct net_if **overlay)
{
	int rc = tailscale_start(config, overlay);

	for (int i = 0; rc == -EINPROGRESS && i < 12; i++) {
		k_sleep(K_SECONDS(5));
		config->login = NULL;
		rc = tailscale_start(config, overlay);
	}
	return rc;
}

static int wait_for_interface(struct net_if *iface)
{
	for (int i = 0; i < 120; i++) {
		if (net_if_is_up(iface)) {
			printk("Tailscale interface ready\n");
			return 0;
		}
		k_sleep(K_MSEC(500));
	}
	return -ETIMEDOUT;
}

int main(void)
{
#ifdef CONFIG_WIFI
	struct net_if *iface = net_if_get_wifi_sta();
#else
	struct net_if *iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
#endif
	struct tailscale_client_config config = {
		.underlay = iface,
		.hostname = CONFIG_SAMPLE_TAILSCALE_HOSTNAME,
		.ca_tag = 1,
		.identity_mode = TAILSCALE_IDENTITY_PERSISTENT,
		.timeout_ms = 45000,
		.login = IS_ENABLED(CONFIG_SAMPLE_TAILSCALE_PRIVATE_CONSOLE) ? show_login : NULL,
	};
	struct net_if *overlay;
	int rc = iface != NULL ? load_trust_anchor(config.ca_tag) : -ENODEV;

	if (rc == 0) {
		rc = start_underlay(iface);
	}
	if (rc == 0 && IS_ENABLED(CONFIG_SAMPLE_TAILSCALE_SNTP_BOOTSTRAP)) {
		struct sntp_time utc;

		rc = sntp_simple("time.cloudflare.com", 5000, &utc);
		if (rc == 0) {
			struct timespec ts = {.tv_sec = utc.seconds};

			rc = sys_clock_settime(SYS_CLOCK_REALTIME, &ts);
		}
	}
	if (rc == 0) {
		rc = start_client(&config, &overlay);
	}
	if (rc == 0) {
		rc = wait_for_interface(overlay);
		bool echo = CONFIG_SAMPLE_TAILSCALE_PEER[0] != '\0';

		if (rc == 0 && echo) {
			rc = example_echo(NET_AF_INET);
			if (rc == 0) {
				rc = example_echo(NET_AF_INET6);
			}
		}
		if (rc != 0 || echo) {
			int stop_rc = tailscale_stop(TAILSCALE_STOP_KEEP_IDENTITY);

			if (rc == 0) {
				rc = stop_rc;
				if (rc == 0 && net_if_is_up(overlay)) {
					rc = -EIO;
				}
				if (rc == 0) {
					printk("Tailscale interface stopped\n");
				}
			}
		}
	}
	printk("Tailscale sample result %d\n", rc);
	return rc;
}
