/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <getopt.h>
#include <zephyr/sys/base64.h>

#include <zephyr/net/virtual.h>
#include <zephyr/net/wireguard.h>

#include "net_shell_private.h"

#if defined(CONFIG_WIREGUARD)
#include "wg_internal.h"
#endif

#if defined(CONFIG_WIREGUARD_SHELL)
static void wg_peer_cb(struct wg_peer *peer, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	/* +7 for []/len */
	char addr[ADDR_LEN + 7];
	char addr_mask[sizeof(addr) + sizeof("/128 ")];
	char public_key[WG_PUBLIC_KEY_LEN * 2];
	size_t olen;

	if ((*count) == 0) {
		PR("Id   Iface %-40s\t %s\t\t%s\n", "Public key", "Endpoint", "Allowed IPs");
	}

	(void)base64_encode(public_key, sizeof(public_key),
			    &olen, peer->key.public_key,
			    sizeof(peer->key.public_key));

	PR("[%2d] %d     %-40s\t %s:%d\t",
	   peer->id, net_if_get_by_iface(peer->iface), public_key,
	   net_addr_ntop(peer->cfg_endpoint.ss_family,
			 &net_sin(net_sad(&peer->cfg_endpoint))->sin_addr,
			 addr, sizeof(addr)),
	   net_ntohs(net_sin(net_sad(&peer->cfg_endpoint))->sin_port));

	ARRAY_FOR_EACH(peer->allowed_ip, i) {
		if (!peer->allowed_ip[i].is_valid) {
			continue;
		}

		net_addr_ntop(peer->allowed_ip[i].addr.family,
			      &peer->allowed_ip[i].addr.in_addr,
			      addr, sizeof(addr));
		snprintk(addr_mask, sizeof(addr_mask), "%s/%d",
			 addr, peer->allowed_ip[i].mask_len);
		PR("%s ", addr_mask);
	}

	PR("\n");

	(*count)++;
}
#endif /* CONFIG_WIREGUARD_SHELL */

static int cmd_net_wg(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_WIREGUARD_SHELL)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;
	user_data.user_data = &count;

	wireguard_peer_foreach(wg_peer_cb, &user_data);

	if (count == 0) {
		PR("No connections\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_WIREGUARD",
		"Wireguard VPN");
#endif /* CONFIG_WIREGUARD_SHELL */

	return 0;
}

#if defined(CONFIG_WIREGUARD_SHELL)
static int parse_allowed_ip_addr(const struct shell *sh, char *allowed_ip_addr,
				 struct wireguard_peer_config *peer_config)
{
	struct net_sockaddr_storage addr = { 0 };
	struct net_sockaddr *paddr = (struct net_sockaddr *)&addr;
	const char *addr_str, *next;
	uint8_t mask_len = 0;
	bool found;

	addr_str = allowed_ip_addr;
	found = false;

	do {
		next = net_ipaddr_parse_mask(addr_str, strlen(addr_str),
						 paddr, &mask_len);
		if (next == NULL) {
			PR_ERROR("Cannot parse IP address \"%s\"\n", allowed_ip_addr);
			return -EINVAL;
		}

		ARRAY_FOR_EACH(peer_config->allowed_ip, i) {
			if (peer_config->allowed_ip[i].is_valid) {
				continue;
			}

			if (paddr->sa_family == NET_AF_INET) {
				struct net_sockaddr_in *addr4 = (struct net_sockaddr_in *)&addr;

				memcpy(&peer_config->allowed_ip[i].addr.in_addr,
				       &addr4->sin_addr,
				       sizeof(struct net_in_addr));
				peer_config->allowed_ip[i].addr.family = NET_AF_INET;
				peer_config->allowed_ip[i].is_valid = true;
				peer_config->allowed_ip[i].mask_len = mask_len;
				found = true;

			} else if (addr.ss_family == NET_AF_INET6) {
				struct net_sockaddr_in6 *addr6 = (struct net_sockaddr_in6 *)&addr;

				memcpy(&peer_config->allowed_ip[i].addr.in6_addr,
				       &addr6->sin6_addr,
				       sizeof(struct net_in6_addr));
				peer_config->allowed_ip[i].addr.family = NET_AF_INET6;
				peer_config->allowed_ip[i].is_valid = true;
				peer_config->allowed_ip[i].mask_len = mask_len;
				found = true;

			} else {
				PR_ERROR("Cannot parse IP address \"%s\"\n", allowed_ip_addr);
				return -EAFNOSUPPORT;
			}

			break;
		}

		addr_str = next;
	} while (addr_str != NULL && *addr_str != '\0');

	if (!found) {
		PR_ERROR("Not enough space for allowed IP addresses (max %d)\n",
			 ARRAY_SIZE(peer_config->allowed_ip));
		return -ENOMEM;
	}

	return 0;
}

static int parse_peer_add_args_to_params(const struct shell *sh, int argc,
					 char *argv[],
					 struct wireguard_peer_config *peer,
					 char *public_key,
					 size_t public_key_len)
{
	struct sys_getopt_state *state;
	int option_index = 0;
	int opt;

	static const struct sys_getopt_option long_options[] = {
		{ "public-key", required_argument, 0, 'k' },
		{ "allowed-ips", optional_argument, 0, 'a' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while ((opt = sys_getopt_long(argc, argv, "k:a:p:h", long_options, &option_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'k':
			strncpy(public_key, state->optarg, public_key_len);
			break;
		case 'a': {
			int ret;

			ret = parse_allowed_ip_addr(sh, state->optarg, peer);
			if (ret < 0) {
				return ret;
			}

			break;
		}
		case 'h':
		case '?':
		default:
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	return 0;
}
#endif /* CONFIG_WIREGUARD_SHELL */

static int cmd_wg_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_WIREGUARD_SHELL)
	struct wireguard_peer_config peer_config = { 0 };
	struct net_if *peer_iface = NULL;
	char public_key[WG_PUBLIC_KEY_LEN * 2];
	int ret;

	if (argc < 2) {
		PR_ERROR("Invalid number of arguments\n");
		return -EINVAL;
	}

	if (parse_peer_add_args_to_params(sh, argc, argv, &peer_config,
					  public_key, sizeof(public_key)) != 0) {
		return -ENOEXEC;
	}

	peer_config.public_key = public_key;

	ret = wireguard_peer_add(&peer_config, &peer_iface);
	if (ret < 0) {
		PR_WARNING("Cannot %s peer (%d)\n", "add", ret);
	} else if (ret > 0) {
		if (peer_iface != NULL) {
			PR("Added peer id %d using interface %d\n", ret,
			   net_if_get_by_iface(peer_iface));
		} else {
			PR("%s peer id %d\n", "Added", ret);
		}
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_WIREGUARD",
		"Wireguard VPN");
#endif /* CONFIG_WIREGUARD_SHELL */

	return 0;
}

#if defined(CONFIG_WIREGUARD_SHELL)
static int parse_peer_del_args_to_params(const struct shell *sh, int argc,
					 char *argv[], int *id)
{
	struct sys_getopt_state *state;
	int option_index = 0;
	int opt;

	static const struct sys_getopt_option long_options[] = {
		{ "id", required_argument, 0, 'i' },
		{ "help", no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while ((opt = sys_getopt_long(argc, argv, "i:h", long_options, &option_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'i': {
			char *endptr;
			int tmp_id;

			tmp_id = strtol(state->optarg, &endptr, 10);
			if (*endptr != '\0') {
				PR_WARNING("Invalid id \"%s\"\n", state->optarg);
				return -EINVAL;
			}

			*id = tmp_id;
			break;
		}
		case 'h':
		case '?':
		default:
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	return 0;
}
#endif /* CONFIG_WIREGUARD_SHELL */

static int cmd_wg_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_WIREGUARD_SHELL)
	int ret, id = 0;

	if (argc < 2) {
		PR_ERROR("Invalid number of arguments\n");
		return -EINVAL;
	}

	if (parse_peer_del_args_to_params(sh, argc, argv, &id) != 0) {
		return -ENOEXEC;
	}

	ret = wireguard_peer_remove(id);
	if (ret < 0) {
		PR_WARNING("Cannot %s peer (%d)\n", "delete", ret);
	} else {
		PR("%s peer id %d\n", "Deleted", ret);
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_WIREGUARD",
		"Wireguard VPN");
#endif /* CONFIG_WIREGUARD_SHELL */

	return 0;
}

#if defined(CONFIG_WIREGUARD_SHELL)
struct keepalive_user_data {
	int count;
	int error;
};

static void wg_keepalive_cb(struct wg_peer *peer, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	struct keepalive_user_data *ud = data->user_data;
	int ret;

	ret = wireguard_peer_keepalive(peer->id);
	if (ret < 0) {
		ud->error++;
	} else {
		ud->count++;
	}
}
#endif /* CONFIG_WIREGUARD_SHELL */

static int cmd_wg_keepalive(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_WIREGUARD_SHELL)
	struct net_shell_user_data user_data;
	struct keepalive_user_data ud = { 0 };

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;
	user_data.user_data = &ud;

	wireguard_peer_foreach(wg_keepalive_cb, &user_data);

	if (ud.count == 0 && ud.error == 0) {
		PR("No connections\n");
	} else {
		if (ud.error > 0 && ud.count > 0) {
			PR_WARNING("Sent keepalive to %d peers, %d failed\n",
				   ud.count, ud.error);
		} else if (ud.error > 0 && ud.count == 0) {
			PR_WARNING("Failed to send keepalive to %d peer%s.\n", ud.error,
				   ud.error > 1 ? "s" : "");
		} else {
			PR("Sent keepalive to %d peers\n", ud.count);
		}
	}
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_WIREGUARD",
		"Wireguard VPN");
#endif /* CONFIG_WIREGUARD_SHELL */

	return 0;
}

#if defined(CONFIG_NET_STATISTICS_VPN) && defined(CONFIG_NET_STATISTICS_USER_API)
static void print_vpn_stats(struct net_if *iface, struct net_stats_vpn *data,
			    const struct shell *sh)
{
	PR("Statistics for VPN interface %p [%d]\n", iface,
	       net_if_get_by_iface(iface));

	PR("Keepalive RX       : %u\n", data->keepalive_rx);
	PR("Keepalive TX       : %u\n", data->keepalive_tx);
	PR("Handshake init RX  : %u\n", data->handshake_init_rx);
	PR("Handshake init TX  : %u\n", data->handshake_init_tx);
	PR("Handshake resp RX  : %u\n", data->handshake_resp_rx);
	PR("Handshake resp TX  : %u\n", data->handshake_resp_tx);
	PR("Peer not found     : %u\n", data->peer_not_found);
	PR("Key expired        : %u\n", data->key_expired);
	PR("Invalid packet     : %u\n", data->invalid_packet);
	PR("Invalid key        : %u\n", data->invalid_key);
	PR("Invalid packet len : %u\n", data->invalid_packet_len);
	PR("Invalid keepalive  : %u\n", data->invalid_keepalive);
	PR("Invalid handshake  : %u\n", data->invalid_handshake);
	PR("Invalid cookie     : %u\n", data->invalid_cookie);
	PR("Invalid MIC        : %u\n", data->invalid_mic);
	PR("Invalid MAC1       : %u\n", data->invalid_mac1);
	PR("Invalid MAC2       : %u\n", data->invalid_mac2);
	PR("Decrypt failed     : %u\n", data->decrypt_failed);
	PR("Dropped RX         : %u\n", data->drop_rx);
	PR("Dropped TX         : %u\n", data->drop_tx);
	PR("Allocation failed  : %u\n", data->alloc_failed);
	PR("Invalid IP version : %u\n", data->invalid_ip_version);
	PR("Invalid IP family  : %u\n", data->invalid_ip_family);
	PR("Denied IP address  : %u\n", data->denied_ip);
	PR("Replay error       : %u\n", data->replay_error);
	PR("RX data packets    : %u\n", data->valid_rx);
	PR("TX data packets    : %u\n", data->valid_tx);
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;

	if (iface && net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		struct net_stats_vpn vpn_data;
		int ret;

		if (net_virtual_get_iface_capabilities(iface) != VIRTUAL_INTERFACE_VPN) {
			return;
		}

		ret = net_mgmt(NET_REQUEST_STATS_GET_VPN, iface,
			       &vpn_data, sizeof(vpn_data));
		if (!ret) {
			print_vpn_stats(iface, &vpn_data, sh);
			(*count)++;
		}
	}
}
#endif /* CONFIG_NET_STATISTICS_VPN && CONFIG_NET_STATISTICS_USER_API */

static int cmd_wg_stats(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_STATISTICS_VPN) && defined(CONFIG_NET_STATISTICS_USER_API)
	struct net_shell_user_data user_data;
	int count = 0;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	user_data.sh = sh;
	user_data.user_data = &count;

	net_if_foreach(iface_cb, &user_data);

	if (count == 0) {
		PR("No connections\n");
	}
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_STATISTICS_VPN, CONFIG_NET_STATISTICS_USER_API and CONFIG_WIREGUARD",
		"Wireguard VPN statistics");
#endif /* CONFIG_NET_STATISTICS_VPN */

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_wg,
	SHELL_CMD_ARG(add, NULL,
		      "Add a peer in order to establish a VPN connection.\n"
		      "[-k, --public-key <key>] : Peer public key in base64 format\n"
		      "[-a, --allowed-ips <ipaddr/mask-len>] : Allowed IPv4/6 addresses. "
		      "Separate multiple addresses by comma or space.\n",
		      cmd_wg_add, 1, 8),
	SHELL_CMD_ARG(del, NULL,
		      "Delete a peer. Any existing connection is terminated.\n"
		      "[-i, --id <peer-id>] : Peer id\n",
		      cmd_wg_del, 1, 4),
	SHELL_CMD_ARG(keepalive, NULL,
		      "Send a keepalive message to peer.\n",
		      cmd_wg_keepalive, 1, 1),
	SHELL_CMD_ARG(show, NULL,
		      "Show information about the Wireguard VPN connections.\n",
		      cmd_net_wg, 1, 1),
	SHELL_CMD_ARG(stats, NULL,
		      "Show statistics information about the Wireguard VPN connections.\n"
		      "The statistics can be reset by using the 'reset' command.\n",
		      cmd_wg_stats, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), wg, &net_cmd_wg,
		 "Show information about the Wireguard VPN connections.",
		 cmd_net_wg, 1, 1);
