/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>

#include "sl_net_ip_types.h"
#include "sl_si91x_types.h"
#include "sl_net_si91x.h"
#include "sl_net_dns.h"
#ifdef CONFIG_WIFI_SILABS_SIWX91X_PING
#include "sl_net_ping.h"
#endif
#include "sl_net.h"
#include "app_config.h"

volatile uint8_t state = 0;
static K_SEM_DEFINE(wlan_sem, 0, 1);
K_THREAD_STACK_DEFINE(wifi_stack, STACK_SIZE);

typedef enum siwx91x_app_state_e {
	SIWX91X_WIFI_CONNECT_STATE = 0,
	SIWX91X_IP_CONFIG_STATE,
	SIWX91X_PING_HOSTNAME_STATE
} siwx91x_app_state_t;

void application_start(void);
static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface);
static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb);
static sl_status_t network_event_handler(sl_net_event_t event, sl_status_t status, void *data,
					 uint32_t data_length);

static void dhcp_callback_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				  struct net_if *iface)
{
	uint8_t ipv4_addr[NET_IPV4_ADDR_LEN] = {0};
	uint8_t subnet[NET_IPV4_ADDR_LEN] = {0};
	uint8_t gateway[NET_IPV4_ADDR_LEN] = {0};
	int i = 0;

	if (mgmt_event != NET_EVENT_IPV4_ADDR_ADD) {
		return;
	}

	for (i = 0; i < ARRAY_SIZE(iface->config.ip.ipv4->unicast); i++) {

		if (iface->config.ip.ipv4->unicast[i].ipv4.addr_type != NET_ADDR_DHCP) {
			continue;
		}

		printf("Address[%d]: %s", net_if_get_by_iface(iface),
		       net_addr_ntop(AF_INET,
				     &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
				     ipv4_addr, sizeof(ipv4_addr)));
		printf("    Subnet[%d]: %s", net_if_get_by_iface(iface),
		       net_addr_ntop(AF_INET, &iface->config.ip.ipv4->unicast[i].netmask, subnet,
				     sizeof(subnet)));
		printf("    Router[%d]: %s", net_if_get_by_iface(iface),
		       net_addr_ntop(AF_INET, &iface->config.ip.ipv4->gw, gateway,
				     sizeof(gateway)));

		k_sem_give(&wlan_sem);
	}
}

void application_start(void)
{
	int32_t status = -1;
	uint8_t ping_count = 0;
	struct net_if *iface;

	printf("\r\nApplication started\r\n");
	state = SIWX91X_WIFI_CONNECT_STATE;

	while (1) {
		switch (state) {
		case SIWX91X_WIFI_CONNECT_STATE: {
			struct wifi_connect_req_params cnx_params;

			iface = net_if_get_first_wifi();
			memset(&cnx_params, 0, sizeof(struct wifi_connect_req_params));
			context.connecting = true;
			cnx_params.channel = CHANNEL_NO;
			cnx_params.band = 0;
			cnx_params.security = SECURITY_TYPE;
			cnx_params.psk_length = strlen(PSK);
			cnx_params.psk = PSK;
			cnx_params.ssid_length = strlen(SSID);
			cnx_params.ssid = SSID;

			status = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &cnx_params,
					  sizeof(struct wifi_connect_req_params));
			if (status != STATUS_OK) {
				printf("Connection request failed with error: %d\n", status);
				context.connecting = false;
				state = SIWX91X_WIFI_CONNECT_STATE;
				break;
			}
			printf("Connection requested\n");

			if (k_sem_take(&wlan_sem, K_MSEC(CMD_WAIT_TIME)) != STATUS_OK) {
				printf("\r\nWi-Fi connect failed\r\n");
				state = SIWX91X_WIFI_CONNECT_STATE;
			}
		} break;
		case SIWX91X_IP_CONFIG_STATE: {
			if (k_sem_take(&wlan_sem, K_MSEC(CMD_WAIT_TIME)) != STATUS_OK) {
				printf("\r\nIP config failed\r\n");
				state = SIWX91X_IP_CONFIG_STATE;
				break;
			}
			state = SIWX91X_PING_HOSTNAME_STATE;
		} break;
		case SIWX91X_PING_HOSTNAME_STATE: {
			sl_ip_address_t dns_ip = { 0 };

			sli_net_register_event_handler(network_event_handler);
			dns_ip.type = SL_IPV4;
			status = sl_net_dns_resolve_hostname(HOSTNAME, DNS_TIMEOUT,
							     SL_NET_DNS_TYPE_IPV4, &dns_ip);
			if (status != STATUS_OK) {
				printf("\r\nDNS Query failed:0x%x\r\n", status);
				break;
			}
			printf("\r\nDNS Query successful\r\n");
			printf("\r\nIP address = %d.%d.%d.%d\r\n", dns_ip.ip.v4.bytes[0],
			       dns_ip.ip.v4.bytes[1], dns_ip.ip.v4.bytes[2], dns_ip.ip.v4.bytes[3]);
			while (ping_count < PING_PACKETS) {
				status = sl_si91x_send_ping(dns_ip, PING_PACKET_SIZE);
				if (status != SL_STATUS_IN_PROGRESS) {
					printf("\r\nPing request failed with status 0x%X\r\n",
					       status);
					return;
				}
				ping_count++;
				k_sleep(K_MSEC(1000));
			}
			return;
		} break;
		}
	}
}

static void wifi_mgmt_event_handler(struct net_mgmt_event_callback *cb, uint32_t mgmt_event,
				    struct net_if *iface)
{
	switch (mgmt_event) {
	case NET_EVENT_WIFI_CONNECT_RESULT:
		handle_wifi_connect_result(cb);
		break;
	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		break;
	default:
		break;
	}
}

static void handle_wifi_connect_result(struct net_mgmt_event_callback *cb)
{
	const struct wifi_status *status = (const struct wifi_status *)cb->info;
	int st = status->status;

	if (st) {
		if (st < 0) {
			/* Errno values are negative, try to map to
			 * wifi status values.
			 */
			if (st == -ETIMEDOUT) {
				st = WIFI_STATUS_CONN_TIMEOUT;
			}
		}

		printf("Connection request failed (%s/%d)\n", wifi_conn_status_txt(st), st);
		state = SIWX91X_WIFI_CONNECT_STATE;
	} else {
		printf("Connected to Wi-Fi\n");
		state = SIWX91X_IP_CONFIG_STATE;
	}

	context.connecting = false;
	k_sem_give(&wlan_sem);
}

static sl_status_t network_event_handler(sl_net_event_t event, sl_status_t status, void *data,
					 uint32_t data_length)
{
	UNUSED_PARAMETER(data_length);
	switch (event) {
	case SL_NET_PING_RESPONSE_EVENT: {
		sl_si91x_ping_response_t *response = (sl_si91x_ping_response_t *)data;

		if (status != SL_STATUS_OK) {
			printf("\r\nPing request failed!\r\n");
			return status;
		}
		printf("\n%u bytes received from %u.%u.%u.%u\n", response->ping_size,
		       response->ping_address.ipv4_address[0],
		       response->ping_address.ipv4_address[1],
		       response->ping_address.ipv4_address[2],
		       response->ping_address.ipv4_address[3]);
		break;
	}
	default:
		break;
	}

	return SL_STATUS_OK;
}

int main(void)
{
	struct net_mgmt_event_callback wifi_mgmt_cb;
	struct net_mgmt_event_callback mgmt_cb;
	struct k_thread wifi_task_handle;

	printf("siwx91x sample application for ICMP\r\n");
	net_mgmt_init_event_callback(&wifi_mgmt_cb, wifi_mgmt_event_handler,
				     (WIFI_SHELL_MGMT_EVENTS));
	net_mgmt_add_event_callback(&wifi_mgmt_cb);
	net_mgmt_init_event_callback(&mgmt_cb, dhcp_callback_handler, NET_EVENT_IPV4_ADDR_ADD);

	net_mgmt_add_event_callback(&mgmt_cb);

	k_tid_t application_tid = k_thread_create(&wifi_task_handle, wifi_stack, STACK_SIZE,
						  (k_thread_entry_t)application_start, NULL, NULL,
						  NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);

	k_thread_start(application_tid);
	k_thread_join(application_tid, K_FOREVER);
	printf("\r\nApplication exit\r\n");
	return 0;
}
