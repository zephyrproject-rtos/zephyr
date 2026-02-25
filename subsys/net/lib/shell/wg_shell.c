/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <getopt.h>
#include <zephyr/sys/base64.h>

#include <zephyr/sys/sys_getopt.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
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

static void wg_peer_detail_cb(struct wg_peer *peer, void *user_data)
{
	struct net_shell_user_data *data = user_data;
	const struct shell *sh = data->sh;
	int *count = data->user_data;
	/* +7 for []/len */
	char addr[ADDR_LEN + 7], *paddr;
	char addr_mask[sizeof(addr) + sizeof("/128 ")];
	char public_key[WG_PUBLIC_KEY_LEN * 2];
	uint32_t now, diff;
	uint64_t gt_sec;
	uint32_t gt_nsec;
	size_t olen;

	if (peer->id != *count) {
		return;
	}

	(void)base64_encode(public_key, sizeof(public_key),
			    &olen, peer->key.public_key,
			    sizeof(peer->key.public_key));

	now = sys_clock_tick_get_32();

	PR("Peer id             : %d\n", peer->id);
	PR("Peer interface      : %d\n", net_if_get_by_iface(peer->iface));
	PR("Public key          : %s\n", public_key);
	PR("Endpoint (cfg)      : %s:%d\n",
	   net_addr_ntop(peer->cfg_endpoint.ss_family,
			 &net_sin(net_sad(&peer->cfg_endpoint))->sin_addr,
			 addr, sizeof(addr)),
	   net_ntohs(net_sin(net_sad(&peer->cfg_endpoint))->sin_port));

	paddr = net_addr_ntop(peer->endpoint.ss_family,
			      &net_sin(net_sad(&peer->endpoint))->sin_addr,
			      addr, sizeof(addr));
	if (paddr == NULL) {
		PR("Endpoint (real)     : %s\n", "<not connected>");
	} else {
		PR("Endpoint (real)     : %s:%d\n",
		   paddr,
		   net_ntohs(net_sin(net_sad(&peer->endpoint))->sin_port));
	}

	PR("Allowed IPs         : ");
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

	gt_sec = sys_get_be64(peer->greatest_timestamp);
	gt_nsec = sys_get_be32(peer->greatest_timestamp + 8);
	PR("Greatest timestamp  : %llu.%u\n", gt_sec, gt_nsec);

	diff = now - peer->last_initiation_rx;
	PR("Last init RX        : %d sec\n", k_ticks_to_sec_floor32(diff));

	diff = now - peer->last_initiation_tx;
	PR("Last init TX        : %d sec\n", k_ticks_to_sec_floor32(diff));

	diff = now - peer->last_rx;
	PR("Last RX             : %d sec\n", k_ticks_to_sec_floor32(diff));

	diff = now - peer->last_tx;
	PR("Last TX             : %d sec\n", k_ticks_to_sec_floor32(diff));

	PR("Keepalive interval  : %d sec\n", peer->keepalive_interval);
	PR("Keepalive expires   : %d sec\n",
	   k_ticks_to_sec_floor32(
		   sys_timepoint_timeout(peer->keepalive_expires).ticks));

	PR("Key pair (curr)     : %s\n",
	   peer->session.keypair.current.is_valid ? "valid" : "invalid");
	if (peer->session.keypair.current.is_valid) {
		PR("         sending ok : %s\n",
		   peer->session.keypair.current.is_sending_valid ? "yes" : "no");
		PR("       receiving ok : %s\n",
		   peer->session.keypair.current.is_receiving_valid ? "yes" : "no");
		PR("          initiator : %s\n",
		   peer->session.keypair.current.is_initiator ? "yes" : "no");
		PR("            expires : %d sec\n",
		   k_ticks_to_sec_floor32(
			   sys_timepoint_timeout(
				   peer->session.keypair.current.expires).ticks));
		PR("           rejected : %d sec\n",
		   k_ticks_to_sec_floor32(
			   sys_timepoint_timeout(
				   peer->session.keypair.current.rejected).ticks));
		PR("    sending counter : %llu\n",
		   peer->session.keypair.current.sending_counter);
		PR("     replay counter : %llu\n",
		   peer->session.keypair.current.replay_counter);
		PR("        local index : 0x%08X\n",
		   peer->session.keypair.current.local_index);
		PR("       remote index : 0x%08X\n",
		   peer->session.keypair.current.remote_index);

		diff = now - peer->session.keypair.current.last_tx;
		PR("       last TX time : %u sec\n", k_ticks_to_sec_floor32(diff));

		diff = now - peer->session.keypair.current.last_rx;
		PR("       last RX time : %u sec\n", k_ticks_to_sec_floor32(diff));
	}

	PR("Rekey expires       : %d sec\n",
	   k_ticks_to_sec_floor32(sys_timepoint_timeout(peer->rekey_expires).ticks));
	PR("Cookie expires      : %d sec\n",
	   k_ticks_to_sec_floor32(sys_timepoint_timeout(
					  peer->cookie_secret_expires).ticks));
	PR("Handshake           : %s\n", peer->handshake.is_valid ? "valid" : "invalid");
	PR("        initiator   : %s\n", peer->handshake.is_initiator ? "yes" : "no");
	PR("      local index   : 0x%08X\n", peer->handshake.local_index);
	PR("     remote index   : 0x%08X\n", peer->handshake.remote_index);
	PR("Handshake MAC1      : %s\n",
	   peer->handshake_mac1_valid ? "valid" : "invalid");
	PR("Send handshake      : %s\n", peer->send_handshake ? "yes" : "no");
	PR("\n");
}
#endif /* CONFIG_WIREGUARD_SHELL */

static int cmd_net_wg(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_WIREGUARD_SHELL)
	struct net_shell_user_data user_data;
	int count = 0;

	if (argv[1] != NULL && (strcmp(argv[1], "-h") == 0 ||
				(strcmp(argv[1], "--help") == 0))) {
		shell_help(sh);
		return 0;
	}

	if (argc < 2) {
		user_data.sh = sh;
		user_data.user_data = &count;

		wireguard_peer_foreach(wg_peer_cb, &user_data);

		if (count == 0) {
			PR("No connections\n");
		}
	} else {
		/* Show detailed information about a specific peer */
		int err = 0;

		count = shell_strtol(argv[1], 10, &err);
		if (err != 0) {
			PR_WARNING("Invalid id \"%s\" (%d)\n", argv[1], err);
			return -EINVAL;
		}

		user_data.sh = sh;
		user_data.user_data = &count;

		wireguard_peer_foreach(wg_peer_detail_cb, &user_data);
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

static int parse_peer_add_args_to_params(const struct shell *sh,
					 int argc, char *argv[],
					 struct wireguard_peer_config *peer,
					 char *public_key,
					 size_t public_key_len,
					 struct net_sockaddr **paddr)
{
	struct sys_getopt_state *state;
	int option_index = 1;
	int opt;
	int ret;

	static const struct sys_getopt_option long_options[] = {
		{ "public-key", sys_getopt_required_argument, 0, 'k' },
		{ "allowed-ips", sys_getopt_required_argument, 0, 'a' },
		{ "endpoint", sys_getopt_required_argument, 0, 'e' },
		{ "keepalive", sys_getopt_required_argument, 0, 't' },
		{ "help", sys_getopt_no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while ((opt = sys_getopt_long(argc, argv, "k:a:e:t:h", long_options,
				      &option_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'k':
			strncpy(public_key, state->optarg, public_key_len - 1U);
			break;
		case 'a':
			ret = parse_allowed_ip_addr(sh, state->optarg, peer);
			if (ret < 0) {
				return ret;
			}
			break;
		case 'e':
			if (!net_ipaddr_parse(state->optarg, strlen(state->optarg),
					      *paddr)) {
				LOG_ERR("Cannot parse peer IP address \"%s\"",
					state->optarg);
				return -EINVAL;
			}

			if ((*paddr)->sa_family == NET_AF_INET6) {
				memcpy(&peer->endpoint_ip, *paddr,
				       sizeof(struct net_sockaddr_in6));
			} else {
				memcpy(&peer->endpoint_ip, *paddr,
				       sizeof(struct net_sockaddr_in));
			}
			break;
		case 't': {
			int err = 0;
			int keepalive;

			keepalive = shell_strtol(state->optarg, 10, &err);
			if (err != 0) {
				PR_WARNING("Invalid keepalive time \"%s\" (%d)\n",
					   state->optarg, err);
				return -EINVAL;
			}

			peer->keepalive_interval = keepalive;
			break;
		}
		case 'h':
		case '?':
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
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
	char public_key[WG_PUBLIC_KEY_LEN * 2] = { 0 };
	struct net_if *peer_iface = NULL;
	struct net_sockaddr_storage addr = { 0 };
	struct net_sockaddr *paddr = (struct net_sockaddr *)&addr;
	int ret;

	if (argc < 2) {
		PR_ERROR("Invalid number of arguments\n");
		return -EINVAL;
	}

	if (parse_peer_add_args_to_params(sh, argc, argv, &peer_config,
					  public_key, sizeof(public_key),
					  &paddr) != 0) {
		return -ENOEXEC;
	}

	if (paddr->sa_family == 0) {
		PR_ERROR("Endpoint address is required\n");
		return -EINVAL;
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
		{ "id", sys_getopt_required_argument, 0, 'i' },
		{ "help", sys_getopt_no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while ((opt = sys_getopt_long(argc, argv, "i:h", long_options, &option_index)) != -1) {
		state = sys_getopt_state_get();
		switch (opt) {
		case 'i': {
			int err = 0;
			int tmp_id;

			tmp_id = shell_strtol(state->optarg, 10, &err);
			if (err != 0) {
				PR_WARNING("Invalid id \"%s\" (%d)\n", state->optarg, err);
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

#if defined(CONFIG_WIREGUARD_SHELL)
static int parse_setup_args_to_params(const struct shell *sh,
				      int argc, char *argv[],
				      char *private_key,
				      size_t private_key_len,
				      struct net_if **vpn_iface)
{
	struct sys_getopt_state *state;
	struct net_if *iface;
	int option_index = 1;
	int iface_idx;
	int opt;
	int ret;

	static const struct sys_getopt_option long_options[] = {
		{ "private-key", sys_getopt_required_argument, 0, 'k' },
		{ "iface", sys_getopt_required_argument, 0, 'i' },
		{ "help", sys_getopt_no_argument, 0, 'h' },
		{ 0, 0, 0, 0 }
	};

	while ((opt = sys_getopt_long(argc, argv, "k:i:h",
				      long_options, &option_index)) != -1) {
		state = sys_getopt_state_get();

		switch (opt) {
		case 'k':
			strncpy(private_key, state->optarg, private_key_len);
			break;
		case 'i':
			ret = 0;
			iface_idx = shell_strtol(state->optarg, 10, &ret);
			if (ret < 0 || iface_idx == 0) {
				PR_ERROR("Invalid interface index \"%s\"\n",
					 state->optarg);
				return ret;
			}

			iface = net_if_get_by_index(iface_idx);
			if (iface == NULL) {
				PR_ERROR("Invalid interface: %d\n", iface_idx);
				return -EINVAL;
			}

			if (!(net_virtual_get_iface_capabilities(iface) & VIRTUAL_INTERFACE_VPN)) {
				PR_ERROR("Interface %d is not a VPN one\n", iface_idx);
				return -ENOENT;
			}

			*vpn_iface = iface;
			break;
		case 'h':
		case '?':
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		default:
			PR_ERROR("Invalid option %c\n", state->optopt);
			shell_help(sh);
			return SHELL_CMD_HELP_PRINTED;
		}
	}

	return 0;
}

/* User data for the interface callback */
struct ud {
	struct net_if *vpn[CONFIG_WIREGUARD_MAX_PEER];
};

static void wg_iface_cb(struct net_if *iface, void *user_data)
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

static void crypto_zero(void *dest, size_t len)
{
	volatile uint8_t *p = (uint8_t *)dest;

	while (len--) {
		*p++ = 0;
	}
}
#endif

static int cmd_wg_setup(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_WIREGUARD_SHELL)
	struct virtual_interface_req_params params = { 0 };
	uint8_t my_private_key[WG_PRIVATE_KEY_LEN] = { 0 };
	char private_key[WG_PRIVATE_KEY_LEN * 2] = { 0 };
	struct net_if *iface = NULL;
	struct ud user_data = { 0 };
	size_t olen = 0;
	int ret;

	if (argc < 2) {
		PR_ERROR("Invalid number of arguments\n");
		return -EINVAL;
	}

	if (parse_setup_args_to_params(sh, argc, argv,
				       private_key, sizeof(private_key),
				       &iface) != 0) {
		return -ENOEXEC;
	}

	/* public_key contains the private key in base64 format at this point */
	ret = base64_decode(my_private_key, sizeof(my_private_key),
			    &olen, private_key, strlen(private_key));
	if (ret < 0 || olen != sizeof(my_private_key)) {
		PR_ERROR("Invalid private key (len %d)\n", olen);
		return -EINVAL;
	}

	params.private_key.data = my_private_key;
	params.private_key.len = sizeof(my_private_key);

	if (iface == NULL) {
		net_if_foreach(wg_iface_cb, &user_data);

		if (user_data.vpn[0] == NULL) {
			PR_ERROR("No available VPN interface for the new peer\n");
			return -ENODEV;
		}

		iface = user_data.vpn[0];
		if (iface == NULL) {
			PR_ERROR("No available VPN interface for the new peer\n");
			return -ENODEV;
		}
	}

	ret = net_mgmt(NET_REQUEST_VIRTUAL_INTERFACE_SET_PRIVATE_KEY,
		       iface, &params, sizeof(params));

	crypto_zero(private_key, sizeof(private_key));
	crypto_zero(my_private_key, sizeof(my_private_key));

	if (ret < 0) {
		LOG_ERR("Cannot set private key (%d)", ret);
		return ret;
	}

	PR_INFO("VPN interface %d is set up with the provided private key. "
		"You can add peers using the 'net wg add' command.\n",
		net_if_get_by_iface(iface));
#else
	PR_INFO("Set %s to enable %s support.\n", "CONFIG_WIREGUARD",
		"Wireguard VPN");
#endif /* CONFIG_WIREGUARD_SHELL */

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_wg,
	SHELL_CMD_ARG(add, NULL,
		      SHELL_HELP("Add a peer in order to establish a VPN connection",
				 "-k, --public-key <key>\n"
				 "-e, --endpoint <ip address and port>\n"
				 "[-a, --allowed-ips <ipaddr/mask-len>]\n"
				 "[-t, --keepalive <seconds>]\n"
				 "[<peer endpoint IPv4/6 address>:<port>]\n"
				 "Peer public/private key must be in base64 format.\n"
				 "Allowed IPv4/6 addresses can be specified with optional mask "
				 "length (default 32 for IPv4 and 128 for IPv6). "
				 "Separate multiple addresses by comma or space\n"
				 "Peer endpoint is specified in the format\n"
				 "<IPv4 address>:<port> or [<IPv6 address>]:<port>\n"
				 "Keepalive time is specified in seconds. "
				 "If set to 0 (default), no keepalive messages are sent.\n"
				 "Examples:\n"
				 "net wg add -k bm90IHNldA== "
				 "-a 198.51.100.1/24,2001:db8:100::1/64 "
				 "-e [2001:db8::2]:51820 -t 60\n"
				 "net wg add -k ZGV2IG51bGw= -a 198.51.100.1/24 "
				 "-e 192.0.2.2:51820"),
		      cmd_wg_add, 1, 9),
	SHELL_CMD_ARG(del, NULL,
		      SHELL_HELP("Delete a peer. Any existing connection is terminated",
				 "[-i, --id <peer-id>]"),
		      cmd_wg_del, 1, 4),
	SHELL_CMD_ARG(keepalive, NULL,
		      SHELL_HELP("Send a keepalive message to peer", ""),
		      cmd_wg_keepalive, 1, 1),
	SHELL_CMD_ARG(show, NULL,
		      SHELL_HELP("Show information about the Wireguard VPN connections. "
				 "To get detailed information about a specific connection, "
				 "use the 'show <id>' command", "[<id>]"),
		      cmd_net_wg, 1, 1),
	SHELL_CMD_ARG(setup, NULL,
		      SHELL_HELP("Setup local VPN network interface",
				 "[-i, --iface=<interface index>]: Interface index\n"
				 "-k, --private-key <key>: Private key in base64 format"),
		      cmd_wg_setup, 1, 7),
	SHELL_CMD_ARG(stats, NULL,
		      SHELL_HELP("Show statistics information about the Wireguard VPN connections."
				 " The statistics can be reset by using the 'reset' command",
				 "[reset]"),
		      cmd_wg_stats, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), wg, &net_cmd_wg,
		 "Show information about the Wireguard VPN connections.",
		 cmd_net_wg, 1, 1);
