/*
 * Copyright (c) 2026 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(vpn_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <zephyr/sys/base64.h>
#include <zephyr/posix/sys/socket.h>
#include <zephyr/posix/unistd.h>
#include <zephyr/posix/arpa/inet.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/wireguard.h>

/* User data for the interface callback */
struct ud {
	struct net_if *vpn[CONFIG_WIREGUARD_MAX_PEER];
};

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct ud *ud = user_data;

	if (net_if_l2(iface) != &NET_L2_GET_NAME(VIRTUAL)) {
		return;
	}

	if (!(net_virtual_get_iface_capabilities(iface) & VIRTUAL_INTERFACE_VPN)) {
		return;
	}

	for (int i = 0; i < CONFIG_WIREGUARD_MAX_PEER; i++) {
		if (ud->vpn[i] != NULL) {
			continue;
		}

		ud->vpn[i] = iface;
		return;
	}
}

static int setup_iface(struct net_if *iface,
		       const char *my_addresses,
		       const char *allowed_ip_addr,
		       const char *my_private_key,
		       const char *peer_public_key,
		       int keepalive_interval)
{
	struct virtual_interface_req_params params = { 0 };
	struct wireguard_peer_config peer_config = { 0 };
	struct sockaddr_storage addr = { 0 };
	struct sockaddr *paddr = (struct sockaddr *)&addr;
	struct net_if *peer_iface = NULL;
	uint8_t mask_len = 0;
	uint8_t private_key[NET_VIRTUAL_MAX_PUBLIC_KEY_LEN];
	struct net_if_addr *ifaddr;
	const char *addr_str, *next;
	bool status, found;
	size_t olen;
	int ret;

	(void)base64_decode(private_key, sizeof(private_key), &olen,
			    my_private_key, strlen(my_private_key));

	params.private_key.data = private_key;
	params.private_key.len = sizeof(private_key);

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY,
		       iface, &params, sizeof(params));

	memset(private_key, 0, sizeof(private_key));

	if (ret < 0) {
		LOG_ERR("Cannot set private key (%d)", ret);
		return ret;
	}

	addr_str = my_addresses;

	do {
		char my_addr[INET6_ADDRSTRLEN] = { 'N', 'o', ' ', 'I', 'P', '\0'};

		next = net_ipaddr_parse_mask(addr_str, strlen(addr_str),
					     paddr, &mask_len);
		if (next == NULL) {
			LOG_ERR("Cannot parse IP address \"%s\"", my_addresses);
			return -EINVAL;
		}

		inet_ntop(paddr->sa_family, net_sin(paddr)->sin_addr.s4_addr,
			  my_addr, sizeof(my_addr));

		if (paddr->sa_family == AF_INET) {
			struct sockaddr_in *addr4 = (struct sockaddr_in *)paddr;
			struct sockaddr_in mask;

			ifaddr = net_if_ipv4_addr_add(iface, &addr4->sin_addr,
						      NET_ADDR_MANUAL, 0);

			ret = net_mask_len_to_netmask(AF_INET, mask_len,
						      (struct sockaddr *)&mask);
			if (ret < 0) {
				LOG_ERR("Invalid network mask length (%d)", ret);
				return ret;
			}

			status = net_if_ipv4_set_netmask_by_addr(iface,
								 &addr4->sin_addr,
								 &mask.sin_addr);

		} else if (paddr->sa_family == AF_INET6) {
			struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)paddr;
			struct in6_addr netaddr6;

			ifaddr = net_if_ipv6_addr_add(iface, &addr6->sin6_addr,
						      NET_ADDR_MANUAL, 0);

			net_ipv6_addr_prefix_mask((uint8_t *)&addr6->sin6_addr,
						  (uint8_t *)&netaddr6,
						  mask_len);

			if (!net_if_ipv6_prefix_add(iface, &netaddr6, mask_len,
						    (uint32_t)0xffffffff)) {
				LOG_ERR("Cannot add %s to interface %d", my_addr,
					net_if_get_by_iface(iface));
				return -EINVAL;
			}

		} else {
			LOG_ERR("Cannot parse IP address \"%s\"", my_addr);
			return -EAFNOSUPPORT;
		}

		if (ifaddr == NULL) {
			LOG_ERR("Cannot add IP address \"%s\" to interface %d",
				my_addr, net_if_get_by_iface(iface));
			return -ENOENT;
		}

		addr_str = next;
	} while (addr_str != NULL && *addr_str != '\0');

	peer_config.public_key = peer_public_key;

	addr_str = allowed_ip_addr;
	found = false;

	do {
		next = net_ipaddr_parse_mask(addr_str, strlen(addr_str),
					     paddr, &mask_len);
		if (next == NULL) {
			LOG_ERR("Cannot parse IP address \"%s\"", allowed_ip_addr);
			return -EINVAL;
		}

		ARRAY_FOR_EACH(peer_config.allowed_ip, i) {
			if (peer_config.allowed_ip[i].is_valid) {
				continue;
			}

			if (paddr->sa_family == AF_INET) {
				struct sockaddr_in *addr4 = (struct sockaddr_in *)paddr;

				memcpy(&peer_config.allowed_ip[i].addr.in_addr,
				       &addr4->sin_addr,
				       sizeof(struct in_addr));
				peer_config.allowed_ip[i].addr.family = AF_INET;
				peer_config.allowed_ip[i].is_valid = true;
				peer_config.allowed_ip[i].mask_len = mask_len;
				found = true;

			} else if (addr.ss_family == AF_INET6) {
				struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)paddr;

				memcpy(&peer_config.allowed_ip[i].addr.in6_addr,
				       &addr6->sin6_addr,
				       sizeof(struct in6_addr));
				peer_config.allowed_ip[i].addr.family = AF_INET6;
				peer_config.allowed_ip[i].is_valid = true;
				peer_config.allowed_ip[i].mask_len = mask_len;
				found = true;

			} else {
				LOG_ERR("Cannot parse IP address \"%s\"", allowed_ip_addr);
				return -EAFNOSUPPORT;
			}

			break;
		}

		addr_str = next;
	} while (addr_str != NULL && *addr_str != '\0');

	if (!found) {
		LOG_ERR("Not enough space for allowed IP addresses");
		return -ENOMEM;
	}

	if (CONFIG_NET_SAMPLE_COMMON_VPN_PEER_IP_ADDR[0] == '\0') {
		LOG_ERR("Peer IP address is not set");
		return -EINVAL;
	}

	if (!net_ipaddr_parse(CONFIG_NET_SAMPLE_COMMON_VPN_PEER_IP_ADDR,
			      strlen(CONFIG_NET_SAMPLE_COMMON_VPN_PEER_IP_ADDR),
			      paddr)) {
		LOG_ERR("Cannot parse peer IP address \"%s\"",
			CONFIG_NET_SAMPLE_COMMON_VPN_PEER_IP_ADDR);
		return -EINVAL;
	}

	if (paddr->sa_family == AF_INET6) {
		memcpy(&peer_config.endpoint_ip, paddr, sizeof(struct sockaddr_in6));
	} else {
		memcpy(&peer_config.endpoint_ip, paddr, sizeof(struct sockaddr_in));
	}

	peer_config.keepalive_interval = keepalive_interval;

	ret = wireguard_peer_add(&peer_config, &peer_iface);
	if (ret < 0) {
		LOG_ERR("Cannot add peer (%d)", ret);
	} else if (ret > 0) {
		if (peer_iface != NULL) {
			LOG_INF("Added peer id %d using interface %d", ret,
				 net_if_get_by_iface(peer_iface));
		} else {
			LOG_INF("Added peer id %d", ret);
		}
	}

	LOG_DBG("Interface %d VPN setup done.", net_if_get_by_iface(iface));

	return 0;
}

int init_vpn(void)
{
	struct ud user_data;
	int ret;

	memset(&user_data, 0, sizeof(user_data));

	net_if_foreach(iface_cb, &user_data);

	/* This sample has support for one VPN connection.
	 */
	ret = setup_iface(user_data.vpn[0],
			  CONFIG_NET_SAMPLE_COMMON_VPN_MY_ADDR,
			  CONFIG_NET_SAMPLE_COMMON_VPN_ALLOWED_PEER_ADDR,
			  CONFIG_NET_SAMPLE_COMMON_VPN_MY_PRIVATE_KEY,
			  CONFIG_NET_SAMPLE_COMMON_VPN_PEER_PUBLIC_KEY,
			  CONFIG_NET_SAMPLE_COMMON_VPN_KEEPALIVE_INTERVAL);
	if (ret < 0) {
		return ret;
	}

	return 0;
}
