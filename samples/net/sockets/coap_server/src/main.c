/*
 * Copyright (c) 2023 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_coap_service_sample, LOG_LEVEL_DBG);

#include <zephyr/net/coap_service.h>

#include "net_sample_common.h"

#ifdef CONFIG_NET_IPV6
#include <zephyr/net/mld.h>

#include "net_private.h"
#include "ipv6.h"
#endif

static uint16_t coap_port = CONFIG_NET_SAMPLE_COAP_SERVER_SERVICE_PORT;

#ifndef CONFIG_NET_SAMPLE_COAPS_SERVICE

COAP_SERVICE_DEFINE(coap_server, NULL, &coap_port, COAP_SERVICE_AUTOSTART);

#else /* CONFIG_NET_SAMPLE_COAPS_SERVICE */

#include "certificate.h"

static const sec_tag_t sec_tag_list_verify_none[] = {
		SERVER_CERTIFICATE_TAG,
#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
		PSK_TAG,
#endif
};

COAPS_SERVICE_DEFINE(coap_server, NULL, &coap_port, 0,
		     sec_tag_list_verify_none, sizeof(sec_tag_list_verify_none));

#endif /* CONFIG_NET_SAMPLE_COAPS_SERVICE */

static int setup_dtls(void)
{
#if defined(CONFIG_NET_SAMPLE_COAPS_SERVICE)
#if defined(CONFIG_NET_SOCKETS_ENABLE_DTLS)
	int err;

#if defined(CONFIG_NET_SAMPLE_CERTS_WITH_SC)
	err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_CA_CERTIFICATE,
				 ca_certificate,
				 sizeof(ca_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register CA certificate: %d", err);
		return err;
	}
#endif /* defined(CONFIG_NET_SAMPLE_CERTS_WITH_SC) */

	err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_SERVER_CERTIFICATE,
				 server_certificate,
				 sizeof(server_certificate));
	if (err < 0) {
		LOG_ERR("Failed to register public certificate: %d", err);
		return err;
	}

	err = tls_credential_add(SERVER_CERTIFICATE_TAG,
				 TLS_CREDENTIAL_PRIVATE_KEY,
				 private_key, sizeof(private_key));
	if (err < 0) {
		LOG_ERR("Failed to register private key: %d", err);
		return err;
	}

#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
	err = tls_credential_add(PSK_TAG,
				 TLS_CREDENTIAL_PSK,
				 psk,
				 sizeof(psk));
	if (err < 0) {
		LOG_ERR("Failed to register PSK: %d", err);
		return err;
	}

	err = tls_credential_add(PSK_TAG,
				 TLS_CREDENTIAL_PSK_ID,
				 psk_id,
				 sizeof(psk_id) - 1);
	if (err < 0) {
		LOG_ERR("Failed to register PSK ID: %d", err);
		return err;
	}

#endif /* defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED) */
#endif /* defined(CONFIG_NET_SOCKETS_ENABLE_DTLS) */
#endif /* defined(CONFIG_NET_SAMPLE_COAPS_SERVICE) */
	return 0;
}

#if !defined(CONFIG_NET_SAMPLE_COAPS_SERVICE) && defined(CONFIG_NET_IPV6)

#define ALL_NODES_LOCAL_COAP_MCAST \
	{ { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfd } } }

#define MY_IP6ADDR \
	{ { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1 } } }

static int join_coap_multicast_group(uint16_t port)
{
	static struct in6_addr my_addr = MY_IP6ADDR;
	struct sockaddr_in6 mcast_addr = {
		.sin6_family = AF_INET6,
		.sin6_addr = ALL_NODES_LOCAL_COAP_MCAST,
		.sin6_port = htons(port) };
	struct net_if_addr *ifaddr;
	struct net_if *iface;
	int ret;

	iface = net_if_get_default();
	if (!iface) {
		LOG_ERR("Could not get the default interface");
		return -ENOENT;
	}

#if defined(CONFIG_NET_CONFIG_SETTINGS)
	if (net_addr_pton(AF_INET6,
			  CONFIG_NET_CONFIG_MY_IPV6_ADDR,
			  &my_addr) < 0) {
		LOG_ERR("Invalid IPv6 address %s",
			CONFIG_NET_CONFIG_MY_IPV6_ADDR);
	}
#endif

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		LOG_ERR("Could not add unicast address to interface");
		return -EINVAL;
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ret = net_ipv6_mld_join(iface, &mcast_addr.sin6_addr);
	if (ret < 0) {
		LOG_ERR("Cannot join %s IPv6 multicast group (%d)",
			net_sprint_ipv6_addr(&mcast_addr.sin6_addr), ret);
		return ret;
	}

	return 0;
}

#endif /* CONFIG_NET_IPV6 */

int main(void)
{
	int ret;

	wait_for_network();

	ret = setup_dtls();
	if (ret < 0) {
		LOG_ERR("Failed to setup DTLS (%d)", ret);
		return ret;
	}

#if !defined(CONFIG_NET_SAMPLE_COAPS_SERVICE) && defined(CONFIG_NET_IPV6)
	ret = join_coap_multicast_group(coap_port);
	if (ret < 0) {
		LOG_ERR("Failed to join CoAP all-nodes multicast (%d)", ret);
		return ret;
	}
#endif

#ifdef CONFIG_NET_SAMPLE_COAPS_SERVICE
	/* CoAP secure server has to be started manually after DTLS setup */
	ret = coap_service_start(&coap_server);
	if (ret < 0) {
		LOG_ERR("Failed to start CoAP secure server (%d)", ret);
		return ret;
	}
#endif

	return ret;
}
