/*
 * SPDX-FileCopyrightText: Copyright (c) 2021 BayLibre SAS
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/ethernet_bridge.h>
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
#include <zephyr/net/ethernet_bridge_fdb.h>
#endif
#if defined(CONFIG_NET_ETHERNET_BRIDGE_STP)
#include <zephyr/net/ethernet_bridge_stp.h>
#endif
#include <zephyr/sys/slist.h>

static int get_idx(const struct shell *sh, char *index_str)
{
	char *endptr;
	int idx;

	idx = strtol(index_str, &endptr, 10);
	if (*endptr != '\0') {
		shell_warn(sh, "Invalid index %s\n", index_str);
		return -ENOENT;
	}

	return idx;
}

/* The shell parses -h and --help itself only without CONFIG_SHELL_GETOPT */
static bool is_help_opt(const char *arg)
{
	return strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0;
}

/* The commands take -h in place of their arguments, so the shell checks only
 * the upper bound of the argument count and this checks the lower one.
 */
static int check_help(const struct shell *sh, size_t argc, char *argv[], size_t min_argc)
{
	if (argc == 2 && is_help_opt(argv[1])) {
		shell_help(sh);
		return SHELL_CMD_HELP_PRINTED;
	}

	if (argc < min_argc) {
		shell_error(sh, "%s: wrong parameter count", argv[0]);
		shell_help(sh);
		return -EINVAL;
	}

	return 0;
}

static int cmd_bridge_addif(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_DSA)
	struct ethernet_context *eth_ctx;
#endif
	int ret = 0, br_idx, if_idx;
	struct net_if *iface;
	struct net_if *br;

	ret = check_help(sh, argc, argv, 3);
	if (ret != 0) {
		return ret;
	}

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}

	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}

	for (int i = 2; i < argc; i++) {
		if_idx = get_idx(sh, argv[i]);
		if (if_idx < 0) {
			continue;
		}

		iface = net_if_get_by_index(if_idx);
		if (iface == NULL) {
			shell_warn(sh, "Interface %d not found\n", if_idx);
			continue;
		}

		if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
			shell_warn(sh, "Interface %d is not Ethernet\n", if_idx);
			continue;
		}

#if defined(CONFIG_NET_DSA)
		eth_ctx = net_if_l2_data(iface);
		if (eth_ctx->dsa_port != DSA_USER_PORT &&
		    !(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
			shell_warn(sh, "Interface %d cannot do promiscuous mode\n", if_idx);
			continue;
		}
#else
		if (!(net_eth_get_hw_capabilities(iface) & ETHERNET_PROMISC_MODE)) {
			shell_warn(sh, "Interface %d cannot do promiscuous mode\n", if_idx);
			continue;
		}
#endif

		ret = eth_bridge_iface_add(br, iface);
		if (ret < 0) {
			shell_error(sh, "error: bridge iface add (%d)\n", ret);
		}
	}

	return ret;
}

static int cmd_bridge_delif(const struct shell *sh, size_t argc, char *argv[])
{
	int ret = 0, br_idx, if_idx;
	struct net_if *iface;
	struct net_if *br;

	ret = check_help(sh, argc, argv, 3);
	if (ret != 0) {
		return ret;
	}

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}

	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL) {
		shell_warn(sh, "Bridge %d not found\n", br_idx);
		return -ENOENT;
	}

	for (int i = 2; i < argc; i++) {
		if_idx = get_idx(sh, argv[i]);
		if (if_idx < 0) {
			continue;
		}

		iface = net_if_get_by_index(if_idx);
		if (iface == NULL) {
			shell_warn(sh, "Interface %d not found\n", if_idx);
			continue;
		}

		ret = eth_bridge_iface_remove(br, iface);
		if (ret < 0) {
			shell_error(sh, "error: bridge iface remove (%d)\n", ret);
		}
	}

	return ret;
}

static void bridge_show(struct eth_bridge_iface_context *ctx, void *data)
{
	int br_idx = eth_bridge_get_index(ctx->iface);
	const struct shell *sh = data;

	shell_fprintf(sh, SHELL_NORMAL, "%-7d", br_idx);

	if (net_if_is_up(ctx->iface)) {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "up");
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "down");
	}

	if (ctx->is_setup) {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "ok");
	} else {
		shell_fprintf(sh, SHELL_NORMAL, "%-9s", "no");
	}

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ARRAY_FOR_EACH(ctx->ports, i) {
		int if_idx;

		if (ctx->ports[i].iface == NULL) {
			continue;
		}

		if_idx = net_if_get_by_iface(ctx->ports[i].iface);

		shell_fprintf(sh, SHELL_NORMAL, "%-2d", if_idx);
	}

	shell_fprintf(sh, SHELL_NORMAL, "\n");

	k_mutex_unlock(&ctx->lock);
}

static int cmd_bridge_show(const struct shell *sh, size_t argc, char *argv[])
{
	struct net_if *br = NULL;
	int br_idx;
	int ret;

	ret = check_help(sh, argc, argv, 1);
	if (ret != 0) {
		return ret;
	}

	if (argc == 2) {
		br_idx = get_idx(sh, argv[1]);
		if (br_idx < 0) {
			return br_idx;
		}

		br = eth_bridge_get_by_index(br_idx);
		if (br == NULL) {
			shell_warn(sh, "Bridge %d not found\n", br_idx);
			return -ENOENT;
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "Bridge %-9s%-9sInterfaces\n",
		      "Status", "Config");

	if (br != NULL) {
		bridge_show(net_if_get_device(br)->data, (void *)sh);
	} else {
		net_eth_bridge_foreach(bridge_show, (void *)sh);
	}

	return 0;
}

static int cmd_bridge_fdb_add(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
	struct net_eth_addr mac;
	struct net_if *iface;
	int iface_idx;
	int ret;

	ret = check_help(sh, argc, argv, 3);
	if (ret != 0) {
		return ret;
	}

	/* Parse MAC address */
	ret = net_bytes_from_str(mac.addr, sizeof(mac.addr), argv[1]);
	if (ret != 0) {
		shell_error(sh, "Wrong mac address format");
		return -EINVAL;
	}

	/* Parse interface */
	iface_idx = shell_strtol(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	iface = net_if_get_by_index(iface_idx);
	if (iface == NULL) {
		shell_error(sh, "Invalid interface: %d", iface_idx);
		return -EINVAL;
	}

	ret = eth_bridge_fdb_add(&mac, iface);
	if (ret != 0) {
		return ret;
	}

	shell_print(sh, "Add FDB entry successfully");
	return 0;
#else
	shell_warn(sh, "Set CONFIG_NET_ETHERNET_BRIDGE_FDB to enable Ethernet bridge FDB support.");
	return -ENOTSUP;
#endif /* CONFIG_NET_ETHERNET_BRIDGE_FDB */
}

static int cmd_bridge_fdb_del(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
	struct net_eth_addr mac;
	struct net_if *iface;
	int iface_idx;
	int ret;

	ret = check_help(sh, argc, argv, 3);
	if (ret != 0) {
		return ret;
	}

	/* Parse MAC address */
	ret = net_bytes_from_str(mac.addr, sizeof(mac.addr), argv[1]);
	if (ret != 0) {
		shell_error(sh, "Wrong mac address format");
		return -EINVAL;
	}

	/* Parse interface */
	iface_idx = shell_strtol(argv[2], 10, &ret);
	if (ret < 0) {
		return ret;
	}

	iface = net_if_get_by_index(iface_idx);
	if (iface == NULL) {
		shell_error(sh, "Invalid interface: %d", iface_idx);
		return -EINVAL;
	}

	ret = eth_bridge_fdb_del(&mac, iface);
	if (ret != 0) {
		return ret;
	}

	shell_print(sh, "Delete FDB entry successfully");
	return 0;
#else
	shell_warn(sh, "Set CONFIG_NET_ETHERNET_BRIDGE_FDB to enable Ethernet bridge FDB support.");
	return -ENOTSUP;
#endif /* CONFIG_NET_ETHERNET_BRIDGE_FDB */
}

#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
static void cmd_bridge_fdb_handler(struct eth_bridge_fdb_entry *entry, void *user_data)
{
	const struct shell *sh = (const struct shell *)user_data;

	shell_print(sh, "%02x:%02x:%02x:%02x:%02x:%02x  %-8d  %s",
		 entry->mac.addr[0], entry->mac.addr[1],
		 entry->mac.addr[2], entry->mac.addr[3],
		 entry->mac.addr[4], entry->mac.addr[5],
		 net_if_get_by_iface(entry->iface),
		 (entry->flags & ETHERNET_BRIDGE_FDB_FLAG_STATIC) ? "static" : "dynamic");
}
#endif

static int cmd_bridge_fdb_show(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB)
	int ret;

	ret = check_help(sh, argc, argv, 1);
	if (ret != 0) {
		return ret;
	}

	if (argc > 1) {
		shell_error(sh, "%s: wrong parameter count", argv[0]);
		shell_help(sh);
		return -EINVAL;
	}

	shell_print(sh, "FDB Table:");
	shell_print(sh, "MAC Address        Interface  Flags");
	shell_print(sh, "-------------------------------------------");

	eth_bridge_fdb_foreach(&cmd_bridge_fdb_handler, (void *)sh);
	return 0;
#else
	shell_warn(sh, "Set CONFIG_NET_ETHERNET_BRIDGE_FDB to enable Ethernet bridge FDB support.");
	return -ENOTSUP;
#endif /* CONFIG_NET_ETHERNET_BRIDGE_FDB */
}

/* Without a handler of its own, "fdb -h" would run the bridge command */
static int cmd_bridge_fdb(const struct shell *sh, size_t argc, char *argv[])
{
	shell_help(sh);

	if (argc == 2 && !is_help_opt(argv[1])) {
		return -EINVAL;
	}

	return SHELL_CMD_HELP_PRINTED;
}

#if defined(CONFIG_NET_ETHERNET_BRIDGE_STP)
#define STP_BRIDGE_USAGE                                                                           \
	"Usage: net bridge stp <bridge_index> [options]\n"                                         \
	"Without options, show the spanning tree state of the bridge.\n"                           \
	"  -e, --enable on|off       Start or stop the protocol\n"                                 \
	"  -p, --priority <0-61440>  Bridge Priority, in steps of 4096\n"                          \
	"  -a, --max-age <6-40>      Bridge Max Age in seconds\n"                                  \
	"  -f, --forward-delay <4-30> Bridge Forward Delay in seconds\n"                           \
	"  -t, --tx-hold <1-10>      Transmit Hold Count\n"                                        \
	"  -v, --version stp|rstp    Force Protocol Version\n"                                     \
	"  -h, --help                Show this help\n"

#define STP_PORT_USAGE                                                                             \
	"Usage: net bridge stp <bridge_index> port <iface_index> [options]\n"                      \
	"Without options, show the spanning tree state of the port.\n"                             \
	"  -p, --priority <0-240>    Port Priority, in steps of 16\n"                              \
	"  -c, --cost <0-200000000>  Port Path Cost, 0 for the link speed default\n"               \
	"  -e, --edge on|off         The port is an Edge Port from the start\n"                    \
	"  -a, --auto-edge on|off    Detect an Edge Port from the absence of BPDUs\n"              \
	"  -l, --link auto|p2p|shared Point-to-point link, or shared LAN\n"                        \
	"  -m, --mcheck              Check whether STP bridges are still attached\n"               \
	"  -h, --help                Show this help\n"

struct stp_bridge_opts {
	bool enable_set;
	bool enable;
	bool priority_set;
	uint16_t priority;
	bool max_age_set;
	uint8_t max_age;
	bool forward_delay_set;
	uint8_t forward_delay;
	bool tx_hold_set;
	uint8_t tx_hold;
	bool version_set;
	enum eth_bridge_stp_version version;
};

struct stp_port_opts {
	bool priority_set;
	uint8_t priority;
	bool cost_set;
	uint32_t cost;
	bool edge_set;
	bool edge;
	bool auto_edge_set;
	bool auto_edge;
	bool link_set;
	enum eth_bridge_stp_p2p link;
	bool mcheck;
};

static int parse_on_off(const char *str, bool *value)
{
	if (strcmp(str, "on") == 0) {
		*value = true;
	} else if (strcmp(str, "off") == 0) {
		*value = false;
	} else {
		return -EINVAL;
	}

	return 0;
}

static int parse_range(const char *str, unsigned long min, unsigned long max, unsigned long step,
		       unsigned long *value)
{
	int err = 0;

	*value = shell_strtoul(str, 10, &err);
	if (err != 0 || *value < min || *value > max || (*value % step) != 0U) {
		return -EINVAL;
	}

	return 0;
}

static const char *stp_role_str(enum eth_bridge_stp_role role)
{
	switch (role) {
	case ETH_BRIDGE_STP_ROLE_ROOT:
		return "root";
	case ETH_BRIDGE_STP_ROLE_DESIGNATED:
		return "designated";
	case ETH_BRIDGE_STP_ROLE_ALTERNATE:
		return "alternate";
	case ETH_BRIDGE_STP_ROLE_BACKUP:
		return "backup";
	default:
		return "disabled";
	}
}

static const char *stp_state_str(const struct eth_bridge_stp_port_status *status)
{
	if (status->forwarding) {
		return "forwarding";
	}

	return status->learning ? "learning" : "discarding";
}

static const char *stp_link_str(enum eth_bridge_stp_p2p p2p)
{
	switch (p2p) {
	case ETH_BRIDGE_STP_P2P_ON:
		return "p2p";
	case ETH_BRIDGE_STP_P2P_OFF:
		return "shared";
	default:
		return "auto";
	}
}

/* Bridge Identifier as priority.MAC address, the form used by other tools */
static const char *stp_id_str(uint64_t id, char *buf, size_t len)
{
	snprintk(buf, len, "%04x.%02x:%02x:%02x:%02x:%02x:%02x", (unsigned int)(id >> 48),
		 (unsigned int)(id >> 40) & 0xffU, (unsigned int)(id >> 32) & 0xffU,
		 (unsigned int)(id >> 24) & 0xffU, (unsigned int)(id >> 16) & 0xffU,
		 (unsigned int)(id >> 8) & 0xffU, (unsigned int)id & 0xffU);

	return buf;
}

#define STP_ID_LEN sizeof("ffff.ff:ff:ff:ff:ff:ff")

static void stp_port_line(const struct shell *sh, struct net_if *br, struct net_if *iface)
{
	struct eth_bridge_stp_port_status status;
	char id[STP_ID_LEN];

	if (eth_bridge_stp_port_get_status(br, iface, &status) != 0) {
		return;
	}

	shell_print(sh, "%-6d%-12s%-12s%04x  %-10u%-5s%-5s%-8s%-5s%s", net_if_get_by_iface(iface),
		    stp_role_str(status.role), stp_state_str(&status), status.port_id,
		    status.path_cost, status.oper_edge ? "yes" : "no",
		    status.oper_p2p ? "yes" : "no", stp_link_str(status.p2p),
		    status.send_rstp ? "RSTP" : "STP",
		    stp_id_str(status.designated_bridge, id, sizeof(id)));
}

static void stp_port_header(const struct shell *sh)
{
	shell_print(sh, "Iface Role        State       Port  Cost      Edge P2P  Link    BPDU "
			"Designated bridge");
}

static void stp_show(const struct shell *sh, struct net_if *br)
{
	struct eth_bridge_iface_context *ctx = net_if_get_device(br)->data;
	struct net_if *ports[NET_ETHERNET_BRIDGE_ETH_INTERFACE_COUNT];
	struct eth_bridge_stp_status status;
	char id[STP_ID_LEN];
	size_t count = 0;

	if (eth_bridge_stp_get_status(br, &status) != 0) {
		return;
	}

	shell_print(sh, "Bridge %d spanning tree %s, %s", eth_bridge_get_index(br),
		    status.enabled ? "on" : "off",
		    status.version == ETH_BRIDGE_STP_VERSION_STP ? "STP" : "RSTP");
	shell_print(sh, "Bridge ID     %s", stp_id_str(status.bridge_id, id, sizeof(id)));
	shell_print(sh, "Root ID       %s", stp_id_str(status.root_id, id, sizeof(id)));

	if (status.root_port != NULL) {
		shell_print(sh, "Root port     %d, path cost %u",
			    net_if_get_by_iface(status.root_port), status.root_path_cost);
	} else {
		shell_print(sh, "Root port     none, this bridge is the root");
	}

	shell_print(sh, "Times         max age %u s, forward delay %u s (bridge %u s, %u s)",
		    status.max_age, status.forward_delay, status.bridge_max_age,
		    status.bridge_forward_delay);
	shell_print(sh, "Tx hold count %u", status.tx_hold_count);

	k_mutex_lock(&ctx->lock, K_FOREVER);

	ARRAY_FOR_EACH(ctx->ports, i) {
		if (ctx->ports[i].iface != NULL) {
			ports[count++] = ctx->ports[i].iface;
		}
	}

	k_mutex_unlock(&ctx->lock);

	stp_port_header(sh);

	for (size_t i = 0; i < count; i++) {
		stp_port_line(sh, br, ports[i]);
	}
}

static int stp_parse_bridge_opts(const struct shell *sh, size_t argc, char *argv[],
				 struct stp_bridge_opts *opts)
{
	static const char shortopts[] = "e:p:a:f:t:v:h";
	static const struct sys_getopt_option longopts[] = {
		{"enable", sys_getopt_required_argument, NULL, 'e'},
		{"priority", sys_getopt_required_argument, NULL, 'p'},
		{"max-age", sys_getopt_required_argument, NULL, 'a'},
		{"forward-delay", sys_getopt_required_argument, NULL, 'f'},
		{"tx-hold", sys_getopt_required_argument, NULL, 't'},
		{"version", sys_getopt_required_argument, NULL, 'v'},
		{"help", sys_getopt_no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0},
	};
	struct sys_getopt_state *state;
	int nargc = (int)argc;
	unsigned long value;
	int opt_index = 0;
	int opt;
	int ret = 0;

	while ((opt = sys_getopt_long(nargc, argv, shortopts, longopts, &opt_index)) != -1) {
		state = sys_getopt_state_get();

		switch (opt) {
		case 'e':
			ret = parse_on_off(state->optarg, &opts->enable);
			opts->enable_set = true;
			break;
		case 'p':
			ret = parse_range(state->optarg, 0U, 61440U, 4096U, &value);
			opts->priority = (uint16_t)value;
			opts->priority_set = true;
			break;
		case 'a':
			ret = parse_range(state->optarg, 6U, 40U, 1U, &value);
			opts->max_age = (uint8_t)value;
			opts->max_age_set = true;
			break;
		case 'f':
			ret = parse_range(state->optarg, 4U, 30U, 1U, &value);
			opts->forward_delay = (uint8_t)value;
			opts->forward_delay_set = true;
			break;
		case 't':
			ret = parse_range(state->optarg, 1U, 10U, 1U, &value);
			opts->tx_hold = (uint8_t)value;
			opts->tx_hold_set = true;
			break;
		case 'v':
			if (strcmp(state->optarg, "stp") == 0) {
				opts->version = ETH_BRIDGE_STP_VERSION_STP;
			} else if (strcmp(state->optarg, "rstp") == 0) {
				opts->version = ETH_BRIDGE_STP_VERSION_RSTP;
			} else {
				ret = -EINVAL;
			}
			opts->version_set = true;
			break;
		case 'h':
			shell_print(sh, STP_BRIDGE_USAGE);
			return 1;
		default:
			ret = -EINVAL;
			break;
		}

		if (ret != 0) {
			shell_error(sh, "Invalid option or value");
			shell_print(sh, STP_BRIDGE_USAGE);
			return -EINVAL;
		}
	}

	if (sys_getopt_state_get()->optind < nargc) {
		shell_error(sh, "Unexpected argument %s", argv[sys_getopt_state_get()->optind]);
		return -EINVAL;
	}

	return 0;
}

static int stp_bridge_cmd(const struct shell *sh, struct net_if *br, size_t argc, char *argv[])
{
	struct stp_bridge_opts opts = {0};
	struct eth_bridge_stp_status status;
	uint8_t max_age;
	uint8_t forward_delay;
	int ret;

	ret = stp_parse_bridge_opts(sh, argc, argv, &opts);
	if (ret != 0) {
		return ret < 0 ? ret : 0;
	}

	if (!opts.enable_set && !opts.priority_set && !opts.max_age_set &&
	    !opts.forward_delay_set && !opts.tx_hold_set && !opts.version_set) {
		stp_show(sh, br);
		return 0;
	}

	ret = eth_bridge_stp_get_status(br, &status);
	if (ret != 0) {
		return ret;
	}

	max_age = opts.max_age_set ? opts.max_age : status.bridge_max_age;
	forward_delay = opts.forward_delay_set ? opts.forward_delay : status.bridge_forward_delay;

	/* Check every value before changing anything */
	if ((opts.max_age_set || opts.forward_delay_set) &&
	    (2U * (forward_delay - 1U) < max_age || max_age < 6U)) {
		shell_error(sh, "Need 2 * (forward delay - 1) >= max age >= 6");
		return -EINVAL;
	}

	if (opts.version_set) {
		ret = eth_bridge_stp_set_version(br, opts.version);
	}

	if (ret == 0 && opts.priority_set) {
		ret = eth_bridge_stp_set_bridge_priority(br, opts.priority);
	}

	if (ret == 0 && (opts.max_age_set || opts.forward_delay_set)) {
		ret = eth_bridge_stp_set_times(br, max_age, forward_delay);
	}

	if (ret == 0 && opts.tx_hold_set) {
		ret = eth_bridge_stp_set_tx_hold_count(br, opts.tx_hold);
	}

	if (ret == 0 && opts.enable_set) {
		ret = eth_bridge_stp_enable(br, opts.enable);
		if (ret == -EALREADY) {
			ret = 0;
		}
	}

	if (ret != 0) {
		shell_error(sh, "Cannot set the spanning tree parameters (%d)", ret);
	}

	return ret;
}

static int stp_parse_port_opts(const struct shell *sh, size_t argc, char *argv[],
			       struct stp_port_opts *opts)
{
	static const char shortopts[] = "p:c:e:a:l:mh";
	static const struct sys_getopt_option longopts[] = {
		{"priority", sys_getopt_required_argument, NULL, 'p'},
		{"cost", sys_getopt_required_argument, NULL, 'c'},
		{"edge", sys_getopt_required_argument, NULL, 'e'},
		{"auto-edge", sys_getopt_required_argument, NULL, 'a'},
		{"link", sys_getopt_required_argument, NULL, 'l'},
		{"mcheck", sys_getopt_no_argument, NULL, 'm'},
		{"help", sys_getopt_no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0},
	};
	struct sys_getopt_state *state;
	int nargc = (int)argc;
	unsigned long value;
	int opt_index = 0;
	int opt;
	int ret = 0;

	while ((opt = sys_getopt_long(nargc, argv, shortopts, longopts, &opt_index)) != -1) {
		state = sys_getopt_state_get();

		switch (opt) {
		case 'p':
			ret = parse_range(state->optarg, 0U, 240U, 16U, &value);
			opts->priority = (uint8_t)value;
			opts->priority_set = true;
			break;
		case 'c':
			ret = parse_range(state->optarg, 0U, 200000000U, 1U, &value);
			opts->cost = (uint32_t)value;
			opts->cost_set = true;
			break;
		case 'e':
			ret = parse_on_off(state->optarg, &opts->edge);
			opts->edge_set = true;
			break;
		case 'a':
			ret = parse_on_off(state->optarg, &opts->auto_edge);
			opts->auto_edge_set = true;
			break;
		case 'l':
			if (strcmp(state->optarg, "auto") == 0) {
				opts->link = ETH_BRIDGE_STP_P2P_AUTO;
			} else if (strcmp(state->optarg, "p2p") == 0) {
				opts->link = ETH_BRIDGE_STP_P2P_ON;
			} else if (strcmp(state->optarg, "shared") == 0) {
				opts->link = ETH_BRIDGE_STP_P2P_OFF;
			} else {
				ret = -EINVAL;
			}
			opts->link_set = true;
			break;
		case 'm':
			opts->mcheck = true;
			break;
		case 'h':
			shell_print(sh, STP_PORT_USAGE);
			return 1;
		default:
			ret = -EINVAL;
			break;
		}

		if (ret != 0) {
			shell_error(sh, "Invalid option or value");
			shell_print(sh, STP_PORT_USAGE);
			return -EINVAL;
		}
	}

	if (sys_getopt_state_get()->optind < nargc) {
		shell_error(sh, "Unexpected argument %s", argv[sys_getopt_state_get()->optind]);
		return -EINVAL;
	}

	return 0;
}

static int stp_port_cmd(const struct shell *sh, struct net_if *br, struct net_if *iface,
			size_t argc, char *argv[])
{
	struct eth_bridge_stp_port_status status;
	struct stp_port_opts opts = {0};
	int ret;

	ret = eth_bridge_stp_port_get_status(br, iface, &status);
	if (ret != 0) {
		shell_error(sh, "Interface %d is not a port of bridge %d",
			    net_if_get_by_iface(iface), eth_bridge_get_index(br));
		return ret;
	}

	ret = stp_parse_port_opts(sh, argc, argv, &opts);
	if (ret != 0) {
		return ret < 0 ? ret : 0;
	}

	if (!opts.priority_set && !opts.cost_set && !opts.edge_set && !opts.auto_edge_set &&
	    !opts.link_set && !opts.mcheck) {
		stp_port_header(sh);
		stp_port_line(sh, br, iface);
		shell_print(sh, "Admin edge %s, auto edge %s, path cost %s",
			    status.admin_edge ? "on" : "off", status.auto_edge ? "on" : "off",
			    status.path_cost_auto ? "from link speed" : "set");
		return 0;
	}

	if (opts.priority_set) {
		ret = eth_bridge_stp_port_set_priority(br, iface, opts.priority);
	}

	if (ret == 0 && opts.cost_set) {
		ret = eth_bridge_stp_port_set_path_cost(br, iface, opts.cost);
	}

	if (ret == 0 && (opts.edge_set || opts.auto_edge_set)) {
		ret = eth_bridge_stp_port_set_edge(
			br, iface, opts.edge_set ? opts.edge : status.admin_edge,
			opts.auto_edge_set ? opts.auto_edge : status.auto_edge);
	}

	if (ret == 0 && opts.link_set) {
		ret = eth_bridge_stp_port_set_p2p(br, iface, opts.link);
	}

	if (ret == 0 && opts.mcheck) {
		ret = eth_bridge_stp_port_mcheck(br, iface);
	}

	if (ret != 0) {
		shell_error(sh, "Cannot set the port parameters (%d)", ret);
	}

	return ret;
}
#endif /* CONFIG_NET_ETHERNET_BRIDGE_STP */

static int cmd_bridge_stp(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_ETHERNET_BRIDGE_STP)
	struct eth_bridge_stp_status status;
	struct net_if *iface;
	struct net_if *br;
	int br_idx;
	int if_idx;

	if (is_help_opt(argv[1])) {
		shell_print(sh, STP_BRIDGE_USAGE);
		shell_print(sh, STP_PORT_USAGE);
		return 0;
	}

	br_idx = get_idx(sh, argv[1]);
	if (br_idx < 0) {
		return br_idx;
	}

	br = eth_bridge_get_by_index(br_idx);
	if (br == NULL || eth_bridge_stp_get_status(br, &status) != 0) {
		shell_warn(sh, "Bridge %d not found", br_idx);
		return -ENOENT;
	}

	if (argc >= 3 && strcmp(argv[2], "port") == 0) {
		if (argc < 4) {
			shell_print(sh, STP_PORT_USAGE);
			return -EINVAL;
		}

		if (is_help_opt(argv[3])) {
			shell_print(sh, STP_PORT_USAGE);
			return 0;
		}

		if_idx = get_idx(sh, argv[3]);
		if (if_idx < 0) {
			return if_idx;
		}

		iface = net_if_get_by_index(if_idx);
		if (iface == NULL) {
			shell_warn(sh, "Interface %d not found", if_idx);
			return -ENOENT;
		}

		/* The options follow the interface index, which getopt takes as argv[0] */
		return stp_port_cmd(sh, br, iface, argc - 3, &argv[3]);
	}

	/* The options follow the bridge index, which getopt takes as argv[0] */
	return stp_bridge_cmd(sh, br, argc - 1, &argv[1]);
#else
	shell_warn(sh, "Set CONFIG_NET_ETHERNET_BRIDGE_STP to enable the spanning tree protocol.");
	return -ENOTSUP;
#endif /* CONFIG_NET_ETHERNET_BRIDGE_STP */
}

SHELL_STATIC_SUBCMD_SET_CREATE(bridge_fdb_commands,
	SHELL_CMD_ARG(add, NULL,
		SHELL_HELP("Add fdb table entry", "<mac address> <interface index>"),
		cmd_bridge_fdb_add, 2, 1),
	SHELL_CMD_ARG(del, NULL,
		SHELL_HELP("Delete fdb table entry", "<mac address> <interface index>"),
		cmd_bridge_fdb_del, 2, 1),
	SHELL_CMD_ARG(show, NULL,
		SHELL_HELP("Show fdb table", ""),
		cmd_bridge_fdb_show, 1, 1),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(bridge_commands,
	SHELL_CMD_ARG(addif, NULL,
		SHELL_HELP("Add a network interface to a bridge.",
			"<bridge_index> <one or more interface index>"),
		cmd_bridge_addif, 2, 6),
	SHELL_CMD_ARG(delif, NULL,
		SHELL_HELP("Delete a network interface from a bridge.",
			"<bridge_index> <one or more interface index>"),
		cmd_bridge_delif, 2, 6),
	SHELL_CMD_ARG(show, NULL,
		SHELL_HELP("Show bridge information.", "<bridge_index>"),
		cmd_bridge_show, 1, 1),
	SHELL_CMD_ARG(fdb, &bridge_fdb_commands,
		SHELL_HELP("Manage FDB table. Check usage with -h.", NULL),
		cmd_bridge_fdb, 1, 1),
	SHELL_CMD_ARG(stp, NULL,
		SHELL_HELP("Show or set the spanning tree protocol of a bridge or a port.",
			   "<bridge_index> [port <iface_index>] [options], -h for the options"),
		cmd_bridge_stp, 2, 16),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), bridge, &bridge_commands,
		 "Ethernet bridge commands.",
		 cmd_bridge_show, 1, 1);
