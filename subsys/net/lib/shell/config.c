/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdlib.h>

#include "net_shell_private.h"

#include <zephyr/net/net_config.h>

#if defined(CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS)
static struct networking config;
#endif

#define CHANGED(my, t, x) ((my)->__##x##_changed ? "+ " : ((t)->__##x##_changed ? "* " : ""))

static int cmd_net_config(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* Print current configuration */

#if defined(CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS)
	static struct networking current_config;
	struct networking *cfg;
	int ret;

	cfg = &current_config;

	ret = net_config_get(cfg);
	if (ret < 0) {
		PR_ERROR("Failed to %s network configuration (%d)", "get", ret);
		return ret;
	}

	ARRAY_FOR_EACH(cfg->interfaces, i) {
		struct net_cfg_interfaces *iface_cfg, *my_cfg;

		PR("Network interface: %d\n", i + 1);

		iface_cfg = &cfg->interfaces[i];
		my_cfg = &config.interfaces[i];

		if (iface_cfg->bind_to > 0) {
			PR("\t%sbind_to %d\n", CHANGED(my_cfg, iface_cfg, bind_to),
			   iface_cfg->bind_to);
		}

		if (iface_cfg->name[0] != '\0') {
			PR("\t%sname %s\n", CHANGED(my_cfg, iface_cfg, name),
			   iface_cfg->name);
		}

		if (iface_cfg->device_name != NULL) {
			PR("\t%sdevice_name %s\n",
			   CHANGED(my_cfg, iface_cfg, device_name),
			   iface_cfg->device_name);
		}

		if (iface_cfg->set_name[0] != '\0') {
			PR("\t%sset_name %s\n",
			   CHANGED(my_cfg, iface_cfg, set_name),
			   iface_cfg->set_name);
		}

		PR("\t%sset_default %s\n", CHANGED(my_cfg, iface_cfg, set_default),
		   iface_cfg->set_default ? "yes" : "no");

		if (iface_cfg->flags[0].value[0] != '\0') {
			PR("\t%sflags %s\n",
			   CHANGED(&my_cfg->flags[0], &iface_cfg->flags[0], value),
			   iface_cfg->flags[0].value);
		}

		if (IS_ENABLED(MY_CFG_NET_IPV6)) {
			PR("\tIPv6 configuration\n");

			if (!iface_cfg->ipv6.status) {
				PR("\t\t%sipv6.status %s\n",
				   CHANGED(&my_cfg->ipv6, &iface_cfg->ipv6, status),
				   "disabled");
				goto skip_ipv6;
			}

			PR("\t\t%sipv6.status %s\n",
			   CHANGED(&my_cfg->ipv6, &iface_cfg->ipv6, status),
			   "enabled");
			PR("\t\t%sipv6.hop_limit %d\n",
			   CHANGED(&my_cfg->ipv6, &iface_cfg->ipv6, hop_limit),
			   iface_cfg->ipv6.hop_limit);
			PR("\t\t%sipv6.multicast_hop_limit %d\n",
			   CHANGED(&my_cfg->ipv6, &iface_cfg->ipv6, multicast_hop_limit),
			   iface_cfg->ipv6.multicast_hop_limit);

			ARRAY_FOR_EACH(iface_cfg->ipv6.ipv6_addresses, j) {

				if (iface_cfg->ipv6.ipv6_addresses[j].value[0] == '\0') {
					continue;
				}

				PR("\t\t%s-j %d ipv6.ipv6_addresses %s\n",
				   CHANGED(&my_cfg->ipv6.ipv6_addresses[j],
					   &iface_cfg->ipv6.ipv6_addresses[j], value), j,
				   iface_cfg->ipv6.ipv6_addresses[j].value);
			}

			ARRAY_FOR_EACH(iface_cfg->ipv6.ipv6_multicast_addresses, j) {

				if (iface_cfg->ipv6.ipv6_multicast_addresses[j].value[0] == '\0') {
					continue;
				}

				PR("\t\t%s-j %d ipv6.ipv6_multicast_addresses %s\n",
				   CHANGED(&my_cfg->ipv6.ipv6_multicast_addresses[j],
					   &iface_cfg->ipv6.ipv6_multicast_addresses[j],
					   value), j,
				   iface_cfg->ipv6.ipv6_multicast_addresses[j].value);
			}

			ARRAY_FOR_EACH(iface_cfg->ipv6.prefixes, j) {
				if (iface_cfg->ipv6.prefixes[j].address[0] == '\0') {
					continue;
				}

				PR("\t\t%s-j %d ipv6.prefixes.address %s\n"
				   "\t\t%s-j %d ipv6.prefixes.len %d\n"
				   "\t\t%s-j %d ipv6.prefixes.lifetime %d\n",
				   CHANGED(&my_cfg->ipv6.prefixes[j],
					   &iface_cfg->ipv6.prefixes[j], address), j,
				   iface_cfg->ipv6.prefixes[j].address,
				   CHANGED(&my_cfg->ipv6.prefixes[j],
					   &iface_cfg->ipv6.prefixes[j], len), j,
				   iface_cfg->ipv6.prefixes[j].len,
				   CHANGED(&my_cfg->ipv6.prefixes[j],
					   &iface_cfg->ipv6.prefixes[j], lifetime), j,
				   iface_cfg->ipv6.prefixes[j].lifetime);
			}

			if (IS_ENABLED(CONFIG_NET_DHCPV6)) {
				PR("\t\t%sipv6.dhcpv6 %s\n",
				   CHANGED(&my_cfg->ipv6.dhcpv6, &iface_cfg->ipv6.dhcpv6, status),
				   iface_cfg->ipv6.dhcpv6.status ? "enabled" : "disabled");

				if (iface_cfg->ipv6.dhcpv6.status) {
					PR("\t\t\t%sipv6.do_request_address %s\n",
					   CHANGED(&my_cfg->ipv6.dhcpv6, &iface_cfg->ipv6.dhcpv6,
						   do_request_address),
					   iface_cfg->ipv6.dhcpv6.do_request_address ?
					   "yes" : "no");
					PR("\t\t\t%sipv6.do_request_prefix %s\n",
					   CHANGED(&my_cfg->ipv6.dhcpv6, &iface_cfg->ipv6.dhcpv6,
						   do_request_prefix),
					   iface_cfg->ipv6.dhcpv6.do_request_prefix ?
					   "yes" : "no");
				}
			}
		}

skip_ipv6:
		if (IS_ENABLED(CONFIG_NET_IPV4) && iface_cfg->ipv4.status) {
			PR("\tIPv4 configuration\n");

			if (!iface_cfg->ipv4.status) {
				PR("\t\t%sipv4.status %s\n",
				   CHANGED(&my_cfg->ipv4, &iface_cfg->ipv4, status),
				   "disabled");
				goto skip_ipv4;
			}

			PR("\t\t%sipv4.status %s\n",
			   CHANGED(&my_cfg->ipv4, &iface_cfg->ipv4, status),
			   "enabled");
			PR("\t\t%sipv4.time_to_live %d\n",
			   CHANGED(&my_cfg->ipv4, &iface_cfg->ipv4, time_to_live),
			   iface_cfg->ipv4.time_to_live);
			PR("\t\t%sipv4.multicast_time_to_live %d\n",
			   CHANGED(&my_cfg->ipv4, &iface_cfg->ipv4, multicast_time_to_live),
			   iface_cfg->ipv4.multicast_time_to_live);

			if (iface_cfg->ipv4.gateway[0] != '\0') {
				PR("\t\t%sipv4.gateway %s\n",
				   CHANGED(&my_cfg->ipv4, &iface_cfg->ipv4, gateway),
				   iface_cfg->ipv4.gateway);
			}

			if (IS_ENABLED(CONFIG_NET_DHCPV4_SERVER)) {
				PR("\t\t%sipv4.dhcpv4_server.status %s\n",
				   CHANGED(&my_cfg->ipv4.dhcpv4_server,
					   &iface_cfg->ipv4.dhcpv4_server, status),
				   iface_cfg->ipv4.dhcpv4_server.status ? "enabled" : "disabled");
				PR("\t\t%sipv4.dhcpv4_server.base_address %s\n",
				   CHANGED(&my_cfg->ipv4.dhcpv4_server,
					   &iface_cfg->ipv4.dhcpv4_server,
					   base_address),
				   iface_cfg->ipv4.dhcpv4_server.base_address);
			}

			if (IS_ENABLED(CONFIG_NET_DHCPV4)) {
				PR("\t\t%sipv4.dhcpv4.status %s\n",
				   CHANGED(&my_cfg->ipv4.dhcpv4,
					   &iface_cfg->ipv4.dhcpv4, status),
				   iface_cfg->ipv4.dhcpv4.status ? "enabled" : "disabled");
			}

			if (IS_ENABLED(CONFIG_NET_IPV4_AUTO)) {
				PR("\t\t%sipv4.ipv4_autoconf.status %s\n",
				   CHANGED(&my_cfg->ipv4.ipv4_autoconf,
					   &iface_cfg->ipv4.ipv4_autoconf,
					   status),
				   iface_cfg->ipv4.ipv4_autoconf.status ? "enabled" : "disabled");
			}

			ARRAY_FOR_EACH(iface_cfg->ipv4.ipv4_addresses, j) {

				if (iface_cfg->ipv4.ipv4_addresses[j].value[0] == '\0') {
					continue;
				}

				PR("\t\t%s-j %d ipv4.ipv4_addresses %s\n",
				   CHANGED(&my_cfg->ipv4.ipv4_addresses[j],
					   &iface_cfg->ipv4.ipv4_addresses[j], value), j,
				   iface_cfg->ipv4.ipv4_addresses[j].value);
			}

			ARRAY_FOR_EACH(iface_cfg->ipv4.ipv4_multicast_addresses, j) {

				if (iface_cfg->ipv4.ipv4_multicast_addresses[j].value[0] == '\0') {
					continue;
				}

				PR("\t\t%s-j %d ipv4.ipv4_multicast_addresses %s\n",
				   CHANGED(&my_cfg->ipv4.ipv4_multicast_addresses[j],
					   &iface_cfg->ipv4.ipv4_multicast_addresses[j],
					   value), j,
				   iface_cfg->ipv4.ipv4_multicast_addresses[j].value);
			}
		}

skip_ipv4:
		if (IS_ENABLED(CONFIG_NET_VLAN)) {
			PR("\tVLAN configuration\n");

			if (!iface_cfg->vlan.status) {
				PR("\t\t%sstatus %s\n",
				   CHANGED(&my_cfg->vlan, &iface_cfg->vlan, status),
				   "disabled");
				goto skip_vlan;
			}

			PR("\t\t%sstatus %s\n",
			   CHANGED(&my_cfg->vlan, &iface_cfg->vlan, status),
			   "enabled");
			PR("\t\t%stag %d\n",
			   CHANGED(&my_cfg->vlan, &iface_cfg->vlan, tag),
			   iface_cfg->vlan.tag);
		}
skip_vlan:
	}

	if (IS_ENABLED(CONFIG_NET_L2_IEEE802154)) {
		PR("IEEE 802.15.4 configuration\n");

		if (!cfg->ieee_802_15_4.status) {
			PR("\t%sstatus %s\n",
			   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, status),
			   "disabled");
			goto skip_ieee802154;
		}

		PR("\t%sstatus %s\n",
		   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, status),
		   "enabled");

		if (cfg->ieee_802_15_4.bind_to > 0) {
			PR("\t%sbind_to %d\n",
			   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, bind_to),
			   cfg->ieee_802_15_4.bind_to);
		}

		PR("\t%span_id 0x%04X\n",
		   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, pan_id),
		   cfg->ieee_802_15_4.pan_id);
		PR("\t%schannel %d\n",
		   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, channel),
		   cfg->ieee_802_15_4.channel);
		PR("\t%stx_power %d dBm\n",
		   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, tx_power),
		   cfg->ieee_802_15_4.tx_power);
		PR("\t%sack_required %s\n",
		   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, ack_required),
		   cfg->ieee_802_15_4.ack_required ? "yes" : "no");
		PR("\t%ssecurity_key_mode %d\n",
		   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, security_key_mode),
		   cfg->ieee_802_15_4.security_key_mode);
		PR("\t%ssecurity_level %d\n",
		   CHANGED(&config.ieee_802_15_4, &cfg->ieee_802_15_4, security_level),
		   cfg->ieee_802_15_4.security_level);
		PR("\t%ssecurity_key %d\n",
		   CHANGED(&config.ieee_802_15_4.security_key[0],
			   &cfg->ieee_802_15_4.security_key[0], value),
		   cfg->ieee_802_15_4.security_key[0].value);
	}

skip_ieee802154:

	if (IS_ENABLED(CONFIG_NET_CONFIG_CLOCK_SNTP_INIT)) {
		PR("SNTP configuration\n");

		if (!cfg->sntp.status) {
			PR("\t%sstatus %s\n",
			   CHANGED(&config.sntp, &cfg->sntp, status),
			   "disabled");
			goto skip_sntp;
		}

		PR("\t%sstatus %s\n",
		   CHANGED(&config.sntp, &cfg->sntp, status),
		   "enabled");

		if (cfg->sntp.bind_to > 0) {
			PR("\t%sbind_to %d\n",
			   CHANGED(&config.sntp, &cfg->sntp, bind_to),
			   cfg->sntp.bind_to);
		}

		if (cfg->sntp.server[0] != '\0') {
			PR("\t%sserver %s\n",
			   CHANGED(&config.sntp, &cfg->sntp, server),
			   cfg->sntp.server);

			PR("\t%stimeout %d ms\n",
			   CHANGED(&config.sntp, &cfg->sntp, timeout),
			   cfg->sntp.timeout);
		}
	}

skip_sntp:
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS",
		"network stack settings configuration");
#endif
	return 0;
}

static int cmd_net_config_remove(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS)
	int ret;

	memset(&config, 0, sizeof(config));

	ret = net_config_clear();
	if (ret < 0) {
		PR_ERROR("Failed to %s network configuration (%d)", "remove", ret);
		return ret;
	}

	PR("User configured network settings removed.\n");
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS",
		"network stack settings configuration");
#endif

	return 0;
}

#define CHECK_BASE_OPTION(opt, val, cfg)	       \
	if (strcmp(option, #opt) == 0) {	       \
		(cfg)->opt = val;		       \
		(cfg)->__##opt##_changed = true;       \
		goto option_found;		       \
	}

#define CHECK_BASE_STR_OPTION(opt, val, cfg)	       \
	if (strcmp(option, #opt) == 0) {	       \
		strcpy((cfg)->opt, val);	       \
		(cfg)->__##opt##_changed = true;       \
		goto option_found;		       \
	}

#define CHECK_BASE_INT_OPTION(opt, val, cfg)			\
	if (strcmp(option, #opt) == 0) {			\
		err = 0;					\
		(cfg)->opt = shell_strtol(val, 10, &err);	\
		if (err != 0) {					\
			PR_WARNING("Parse error: %s\n", val);	\
			return -ENOEXEC;			\
		}						\
								\
		(cfg)->__##opt##_changed = true;		\
		goto option_found;				\
	}

#define CHECK_BASE_BOOL_OPTION(opt, val, cfg)			\
	if (strcmp(option, #opt) == 0) {			\
		if (strcmp(val, "yes") == 0 ||			\
		    strcmp(val, "enabled") == 0 ||		\
		    strcmp(val, "1") == 0 ||			\
		    strcmp(val, "true") == 0) {			\
			(cfg)->opt = true;			\
		} else if (strcmp(val, "no") == 0 ||		\
			   strcmp(val, "disabled") == 0 ||	\
			   strcmp(val, "0") == 0 ||		\
			   strcmp(val, "false") == 0) {		\
			(cfg)->opt = false;			\
		} else {					\
			PR_WARNING("Invalid boolean value: %s\n", \
				   val);			\
			return -ENOEXEC;			\
		}						\
								\
		(cfg)->__##opt##_changed = true;		\
		goto option_found;				\
	}

#define CHECK_SUB_OPTION(opt, name, val, cfg)	       \
	if (strcmp(option, #opt "." #name) == 0) {     \
		(cfg)->opt.name = val;		       \
		(cfg)->opt.__##name##_changed = true;  \
		goto option_found;		       \
	}

#define CHECK_SUB_STR_OPTION(opt, name, val, cfg)      \
	if (strcmp(option, #opt "." #name) == 0) {     \
		strcpy((cfg)->opt.name, val);	       \
		(cfg)->opt.__##name##_changed = true;  \
		goto option_found;		       \
	}

#define CHECK_SUB_BOOL_OPTION(opt, name, val, cfg)		\
	if (strcmp(option, #opt "." #name) == 0) {		\
		if (strcmp(val, "yes") == 0 ||			\
		    strcmp(val, "enabled") == 0 ||		\
		    strcmp(val, "1") == 0 ||			\
		    strcmp(val, "true") == 0) {			\
			(cfg)->opt . name = true;		\
		} else if (strcmp(val, "no") == 0 ||		\
			   strcmp(val, "disabled") == 0 ||	\
			   strcmp(val, "0") == 0 ||		\
			   strcmp(val, "false") == 0) {		\
			(cfg)->opt . name = false;		\
		} else {					\
			PR_WARNING("Invalid boolean value: %s\n", \
				   val);			\
			return -ENOEXEC;			\
		}						\
								\
		(cfg)->opt.__##name##_changed = true;		\
		goto option_found;				\
	}

#define CHECK_SUB_INT_OPTION(opt, name, val, cfg)		\
	if (strcmp(option, #opt "." #name) == 0) {		\
		err = 0;					\
		(cfg)->opt . name = shell_strtol(val, 10, &err);\
		if (err != 0) {					\
			PR_WARNING("Parse error: %s\n", val);	\
			return -ENOEXEC;			\
		}						\
								\
		(cfg)->opt.__##name##_changed = true;		\
		goto option_found;				\
	}

#define CHECK_SUB_OPTION_ARRAY(opt, name, var, val, cfg)		\
	if (strcmp(option, #opt "." #name) == 0) {			\
		if (array_idx < 0 || array_idx >= ARRAY_SIZE((cfg)->opt.name)) { \
			PR_WARNING("Invalid array index: %d, should be >= 0 && < %d\n", \
				   array_idx, ARRAY_SIZE((cfg)->opt.name)); \
		} else {						\
			strcpy((cfg)->opt.name[array_idx].var, val);	\
			(cfg)->opt.name[array_idx].__##var##_changed = true; \
			goto option_found;				\
		}							\
	}

static int cmd_net_config_set(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS)
	struct net_cfg_interfaces *iface_cfg;
	char *option = NULL, *value = NULL;
	int iface_idx = -1, array_idx = -1;
	int err;

	for (size_t i = 1; i < argc; ++i) {

		if (*argv[i] != '-') {
			/* First non-option argument is the option to set,
			 * second is the value. Skip others.
			 */
			if (option == NULL) {
				option = argv[i];
			} else if (value == NULL) {
				value = argv[i];
			}

			continue;
		}

		switch (argv[i][1]) {
		case 'i':
			err = 0;

			iface_idx = shell_strtol(argv[++i], 10, &err);
			if (err != 0) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			if (iface_idx > NET_CONFIG_NETWORK_INTERFACE_COUNT ||
			    iface_idx <= 0) {
				PR_WARNING("Invalid interface index: %d, "
					   "should be > 0 && <= %d\n",
					   iface_idx,
					   NET_CONFIG_NETWORK_INTERFACE_COUNT);
				return -ENOEXEC;
			}

			break;

		case 'j':
			err = 0;

			array_idx = shell_strtol(argv[++i], 10, &err);
			if (err != 0) {
				PR_WARNING("Parse error: %s\n", argv[i]);
				return -ENOEXEC;
			}

			if (array_idx < 0) {
				PR_WARNING("Invalid array index: %d, should be >= 0\n",
					   array_idx);
				return -ENOEXEC;
			}

			break;

		default:
			PR_WARNING("Unrecognized argument: %s\n", argv[i]);
			return -ENOEXEC;
		}
	}

	if (option == NULL || value == NULL) {
		PR_WARNING("Option name and value must be specified.\n");
		return -ENOEXEC;
	}

	if (iface_idx < 0) {
		/* If user has not set the iface index, we assume 1 (the first interface).
		 */
		PR("Interface index not set, assuming interface 1.\n");
		iface_idx = 1;
	}

	iface_cfg = &config.interfaces[iface_idx - 1];

	CHECK_BASE_STR_OPTION(name, value, iface_cfg);
	CHECK_BASE_OPTION(device_name, value, iface_cfg);
	CHECK_BASE_STR_OPTION(set_name, value, iface_cfg);
	CHECK_BASE_INT_OPTION(bind_to, value, iface_cfg);
	CHECK_BASE_BOOL_OPTION(set_default, value, iface_cfg);

	if (strcmp(option, "flags") == 0) {
		strcpy(iface_cfg->flags[0].value, value);
		iface_cfg->flags[0].__value_changed = true;
		goto option_found;
	}

	CHECK_SUB_BOOL_OPTION(ipv6, status, value, iface_cfg);
	CHECK_SUB_INT_OPTION(ipv6, hop_limit, value, iface_cfg);
	CHECK_SUB_INT_OPTION(ipv6, multicast_hop_limit, value, iface_cfg);
	CHECK_SUB_BOOL_OPTION(ipv6.dhcpv6, status, value, iface_cfg);
	CHECK_SUB_BOOL_OPTION(ipv6.dhcpv6, do_request_address, value, iface_cfg);
	CHECK_SUB_BOOL_OPTION(ipv6.dhcpv6, do_request_prefix, value, iface_cfg);

	CHECK_SUB_OPTION_ARRAY(ipv6, ipv6_addresses, value, value, iface_cfg);
	CHECK_SUB_OPTION_ARRAY(ipv6, ipv6_multicast_addresses, value, value, iface_cfg);

	CHECK_SUB_BOOL_OPTION(ipv4, status, value, iface_cfg);
	CHECK_SUB_INT_OPTION(ipv4, time_to_live, value, iface_cfg);
	CHECK_SUB_INT_OPTION(ipv4, multicast_time_to_live, value, iface_cfg);
	CHECK_SUB_STR_OPTION(ipv4, gateway, value, iface_cfg);
	CHECK_SUB_BOOL_OPTION(ipv4.dhcpv4, status, value, iface_cfg);
	CHECK_SUB_BOOL_OPTION(ipv4.ipv4_autoconf, status, value, iface_cfg);
	CHECK_SUB_BOOL_OPTION(ipv4.dhcpv4_server, status, value, iface_cfg);
	CHECK_SUB_STR_OPTION(ipv4.dhcpv4_server, base_address, value, iface_cfg);

	CHECK_SUB_OPTION_ARRAY(ipv4, ipv4_addresses, value, value, iface_cfg);
	CHECK_SUB_OPTION_ARRAY(ipv4, ipv4_multicast_addresses, value, value, iface_cfg);

	CHECK_SUB_BOOL_OPTION(vlan, status, value, iface_cfg);
	CHECK_SUB_INT_OPTION(vlan, tag, value, iface_cfg);

	CHECK_SUB_BOOL_OPTION(ieee_802_15_4, status, value, &config);
	CHECK_SUB_INT_OPTION(ieee_802_15_4, bind_to, value, &config);
	CHECK_SUB_INT_OPTION(ieee_802_15_4, pan_id, value, &config);
	CHECK_SUB_INT_OPTION(ieee_802_15_4, channel, value, &config);
	CHECK_SUB_INT_OPTION(ieee_802_15_4, tx_power, value, &config);
	CHECK_SUB_BOOL_OPTION(ieee_802_15_4, ack_required, value, &config);
	CHECK_SUB_INT_OPTION(ieee_802_15_4, security_key_mode, value, &config);
	CHECK_SUB_INT_OPTION(ieee_802_15_4, security_level, value, &config);

	if (strcmp(option, "ieee_802_15_4.security_key") == 0) {
		int val;

		err = 0;
		val = shell_strtol(value, 10, &err);
		if (err != 0) {
			PR_WARNING("Parse error: %s\n", value);
			return -ENOEXEC;
		}

		config.ieee_802_15_4.security_key[0].value = val;
		config.ieee_802_15_4.security_key[0].__value_changed = true;
		goto option_found;
	}

	CHECK_SUB_BOOL_OPTION(sntp, status, value, &config);
	CHECK_SUB_INT_OPTION(sntp, bind_to, value, &config);
	CHECK_SUB_STR_OPTION(sntp, server, value, &config);
	CHECK_SUB_INT_OPTION(sntp, timeout, value, &config);

	PR_WARNING("Unrecognized option: %s\n", option);
	return -ENOEXEC;

option_found:

	PR("User configured network setting set.\n");
	PR("Do 'net config commit' to save the changes to permanent storage.\n");
#else
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS",
		"network stack settings configuration");
#endif

	return 0;
}

static int cmd_net_config_commit(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

#if defined(CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS)
	int ret;

	ret = net_config_set(&config);
	if (ret < 0) {
		PR_ERROR("Failed to %s network configuration (%d)", "commit", ret);
		return ret;
	}

	PR("User configured network settings saved.\n");

	memset(&config, 0, sizeof(config));
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_CONFIG_SETTINGS_SHELL_ACCESS",
		"network stack settings configuration");
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_config,
	SHELL_CMD(remove, NULL, "Remove user configured network settings.",
		  cmd_net_config_remove),
	SHELL_CMD(set, NULL,
		  "'net config set [-i network interface in configuration] [-j array index] "
		  "<option> <value>' "
		  "Set a user configured network setting.",
		  cmd_net_config_set),
	SHELL_CMD(commit, NULL, "Commit user configured network settings.",
		  cmd_net_config_commit),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), config, &net_cmd_config,
		 "Configure/view network stack settings.\n"
		 "The '*' indicates user changed settings in the config listing.\n"
		 "The '+' indicates user changed settings but not yet committed it.\n"
		 , cmd_net_config, 1, 0);
