/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief IEEE 802.15.4 shell module
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_shell, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/ieee802154_mgmt.h>

#include "ieee802154_frame.h"

#define EXT_ADDR_STR_SIZE sizeof("xx:xx:xx:xx:xx:xx:xx:xx")
#define EXT_ADDR_STR_LEN (EXT_ADDR_STR_SIZE - 1U)

struct ieee802154_req_params params;
static struct net_mgmt_event_callback scan_cb;
static const struct shell *cb_shell;

static int cmd_ieee802154_ack(const struct shell *sh,
			      size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();

	ARG_UNUSED(argc);

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	if (!strcmp(argv[1], "set") || !strcmp(argv[1], "1")) {
		net_mgmt(NET_REQUEST_IEEE802154_SET_ACK, iface, NULL, 0);
		shell_fprintf(sh, SHELL_NORMAL,
			      "ACK flag set on outgoing packets\n");

		return 0;
	}

	if (!strcmp(argv[1], "unset") || !strcmp(argv[1], "0")) {
		net_mgmt(NET_REQUEST_IEEE802154_UNSET_ACK, iface, NULL, 0);
		shell_fprintf(sh, SHELL_NORMAL,
			      "ACK flag unset on outgoing packets\n");

		return 0;
	}

	return 0;
}

/**
 * Parse a string representing an extended address in ASCII HEX
 * format into a big endian binary representation of the address.
 *
 * @param addr Extended address as a string.
 * @param ext_addr Extended address in big endian byte ordering.
 *
 * @return 0 on success, negative error code otherwise
 */
static inline int parse_extended_address(char *addr, uint8_t *ext_addr)
{
	return net_bytes_from_str(ext_addr, IEEE802154_EXT_ADDR_LENGTH, addr);
}

static int cmd_ieee802154_associate(const struct shell *sh,
				    size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	char ext_addr[EXT_ADDR_STR_SIZE];

	if (argc < 3) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	if (strlen(argv[2]) > EXT_ADDR_STR_LEN) {
		shell_fprintf(sh, SHELL_INFO, "Address too long\n");
		return -ENOEXEC;
	}

	params = (struct ieee802154_req_params){0};
	params.pan_id = atoi(argv[1]);
	strncpy(ext_addr, argv[2], sizeof(ext_addr));

	if (strlen(ext_addr) == EXT_ADDR_STR_LEN) {
		if (parse_extended_address(ext_addr, params.addr) < 0) {
			shell_fprintf(sh, SHELL_INFO,
				      "Failed to parse extended address\n");
			return -ENOEXEC;
		}
		params.len = IEEE802154_EXT_ADDR_LENGTH;
	} else {
		params.short_addr = (uint16_t) atoi(ext_addr);
		params.len = IEEE802154_SHORT_ADDR_LENGTH;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_ASSOCIATE, iface,
		     &params, sizeof(struct ieee802154_req_params))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not associate to %s on PAN ID %u\n",
			      argv[2], params.pan_id);

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Associated to PAN ID %u\n", params.pan_id);
	}

	return 0;
}

static int cmd_ieee802154_disassociate(const struct shell *sh,
				       size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	int ret;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	ret = net_mgmt(NET_REQUEST_IEEE802154_DISASSOCIATE, iface, NULL, 0);
	if (ret == -EALREADY) {
		shell_fprintf(sh, SHELL_INFO,
			      "Interface is not associated\n");

		return -ENOEXEC;
	} else if (ret) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not disassociate? (status: %i)\n", ret);

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Interface is now disassociated\n");
	}

	return 0;
}

static inline uint32_t parse_channel_set(char *str_set)
{
	uint32_t channel_set = 0U;
	char *p, *n;

	p = str_set;

	do {
		uint32_t chan;

		n = strchr(p, ':');
		if (n) {
			*n = '\0';
		}

		chan = atoi(p);
		if (chan > 0 && chan < 32) {
			channel_set |= BIT(chan - 1);
		}

		p = n ? n + 1 : n;
	} while (n);

	return channel_set;
}

static inline char *print_coordinator_address(char *buf, int buf_len)
{
	if (params.len == IEEE802154_EXT_ADDR_LENGTH) {
		int i, pos = 0;

		pos += snprintk(buf + pos, buf_len - pos, "(extended) ");

		for (i = 0; i < IEEE802154_EXT_ADDR_LENGTH; i++) {
			pos += snprintk(buf + pos, buf_len - pos,
					"%02X:", params.addr[i]);
		}

		buf[pos - 1] = '\0';
	} else {
		snprintk(buf, buf_len, "(short) %u", params.short_addr);
	}

	return buf;
}

static void scan_result_cb(struct net_mgmt_event_callback *cb,
			   uint32_t mgmt_event, struct net_if *iface)
{
	char buf[64];

	shell_fprintf(cb_shell, SHELL_NORMAL,
		      "Channel: %u\tPAN ID: %u\tCoordinator Address: %s\t "
		      "LQI: %u Associable: %s\n", params.channel, params.pan_id,
		      print_coordinator_address(buf, sizeof(buf)), params.lqi,
		      params.association_permitted ? "yes" : "no");
}

static int cmd_ieee802154_scan(const struct shell *sh,
			       size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint32_t scan_type;
	int ret = 0;

	if (argc < 3) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	params = (struct ieee802154_req_params){0};

	net_mgmt_init_event_callback(&scan_cb, scan_result_cb,
				     NET_EVENT_IEEE802154_SCAN_RESULT);
	net_mgmt_add_event_callback(&scan_cb);

	if (!strcmp(argv[1], "active")) {
		scan_type = NET_REQUEST_IEEE802154_ACTIVE_SCAN;
	} else if (!strcmp(argv[1], "passive")) {
		scan_type = NET_REQUEST_IEEE802154_PASSIVE_SCAN;
	} else {
		ret = -ENOEXEC;
		goto release_event_cb;
	}

	if (!strcmp(argv[2], "all")) {
		params.channel_set = IEEE802154_ALL_CHANNELS;
	} else {
		params.channel_set = parse_channel_set(argv[2]);
	}

	if (!params.channel_set) {
		ret = -ENOEXEC;
		goto release_event_cb;
	}

	params.duration = atoi(argv[3]);

	shell_fprintf(sh, SHELL_NORMAL,
		      "%s Scanning (channel set: 0x%08x, duration %u ms)...\n",
		      scan_type == NET_REQUEST_IEEE802154_ACTIVE_SCAN ?
		      "Active" : "Passive", params.channel_set,
		      params.duration);

	cb_shell = sh;

	if (scan_type == NET_REQUEST_IEEE802154_ACTIVE_SCAN) {
		ret = net_mgmt(NET_REQUEST_IEEE802154_ACTIVE_SCAN, iface,
			       &params, sizeof(struct ieee802154_req_params));
	} else {
		ret = net_mgmt(NET_REQUEST_IEEE802154_PASSIVE_SCAN, iface,
			       &params, sizeof(struct ieee802154_req_params));
	}

	if (ret) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not raise a scan (status: %i)\n", ret);

		ret = -ENOEXEC;
		goto release_event_cb;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Done\n");
	}

release_event_cb:
	net_mgmt_del_event_callback(&scan_cb);
	return ret;
}

static int cmd_ieee802154_set_chan(const struct shell *sh,
				   size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint16_t channel;

	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	channel = (uint16_t)atoi(argv[1]);

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_CHANNEL, iface,
		     &channel, sizeof(uint16_t))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not set channel %u\n", channel);

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Channel %u set\n", channel);
	}

	return 0;
}

static int cmd_ieee802154_get_chan(const struct shell *sh,
				   size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint16_t channel;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_CHANNEL, iface,
		     &channel, sizeof(uint16_t))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not get channel\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Channel %u\n", channel);
	}

	return 0;
}

static int cmd_ieee802154_set_pan_id(const struct shell *sh,
				     size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint16_t pan_id;

	ARG_UNUSED(argc);

	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	pan_id = (uint16_t)atoi(argv[1]);

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_PAN_ID, iface,
		     &pan_id, sizeof(uint16_t))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not set PAN ID %u\n", pan_id);

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "PAN ID %u set\n", pan_id);
	}

	return 0;
}

static int cmd_ieee802154_get_pan_id(const struct shell *sh,
				     size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint16_t pan_id;

	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_PAN_ID, iface,
		     &pan_id, sizeof(uint16_t))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not get PAN ID\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "PAN ID %u (0x%x)\n", pan_id, pan_id);
	}

	return 0;
}

static int cmd_ieee802154_set_ext_addr(const struct shell *sh,
				       size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint8_t addr[IEEE802154_EXT_ADDR_LENGTH]; /* in big endian */

	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	if (strlen(argv[1]) != EXT_ADDR_STR_LEN) {
		shell_fprintf(sh, SHELL_INFO,
			      "%zd characters needed\n", EXT_ADDR_STR_LEN);
		return -ENOEXEC;
	}

	if (parse_extended_address(argv[1], addr) < 0) {
		shell_fprintf(sh, SHELL_INFO,
			      "Failed to parse extended address\n");
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_EXT_ADDR, iface,
		     addr, IEEE802154_EXT_ADDR_LENGTH)) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not set extended address\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Extended address set\n");
	}

	return 0;
}

static int cmd_ieee802154_get_ext_addr(const struct shell *sh,
				       size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint8_t addr[IEEE802154_EXT_ADDR_LENGTH]; /* in big endian */

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_EXT_ADDR, iface,
		     addr, IEEE802154_EXT_ADDR_LENGTH)) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not get extended address\n");
		return -ENOEXEC;
	} else {
		static char ext_addr[EXT_ADDR_STR_SIZE];
		int i, pos = 0;

		for (i = 0; i < IEEE802154_EXT_ADDR_LENGTH; i++) {
			pos += snprintk(ext_addr + pos,
					EXT_ADDR_STR_SIZE - pos,
					"%02X:", addr[i]);
		}

		ext_addr[EXT_ADDR_STR_SIZE - 1] = '\0';

		shell_fprintf(sh, SHELL_NORMAL,
			      "Extended address: %s\n", ext_addr);
	}

	return 0;
}

static int cmd_ieee802154_set_short_addr(const struct shell *sh,
					 size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint16_t short_addr; /* in CPU byte order */

	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	short_addr = (uint16_t)atoi(argv[1]);

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_SHORT_ADDR, iface,
		     &short_addr, sizeof(uint16_t))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not set short address %u\n", short_addr);

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Short address %u set\n", short_addr);
	}

	return 0;
}

static int cmd_ieee802154_get_short_addr(const struct shell *sh,
					 size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	uint16_t short_addr; /* in CPU byte order */

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_SHORT_ADDR, iface,
		     &short_addr, sizeof(uint16_t))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not get short address\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "Short address %u\n", short_addr);
	}

	return 0;
}

static int cmd_ieee802154_set_tx_power(const struct shell *sh,
				       size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	int16_t tx_power;

	if (argc < 2) {
		shell_help(sh);
		return -ENOEXEC;
	}

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	tx_power = (int16_t)atoi(argv[1]);

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_TX_POWER, iface,
		     &tx_power, sizeof(int16_t))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not set TX power %d\n", tx_power);

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "TX power %d set\n", tx_power);
	}

	return 0;
}

static int cmd_ieee802154_get_tx_power(const struct shell *sh,
				       size_t argc, char *argv[])
{
	struct net_if *iface = net_if_get_ieee802154();
	int16_t tx_power;

	if (!iface) {
		shell_fprintf(sh, SHELL_INFO,
			      "No IEEE 802.15.4 interface found.\n");
		return -ENOEXEC;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_TX_POWER, iface,
		     &tx_power, sizeof(int16_t))) {
		shell_fprintf(sh, SHELL_WARNING,
			      "Could not get TX power\n");

		return -ENOEXEC;
	} else {
		shell_fprintf(sh, SHELL_NORMAL,
			      "TX power (in dbm) %d\n", tx_power);
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(ieee802154_commands,
	SHELL_CMD(ack, NULL,
		  "<set/1 | unset/0> Set auto-ack flag",
		  cmd_ieee802154_ack),
	SHELL_CMD(associate, NULL,
		  "<pan_id> <PAN coordinator short or long address (EUI-64)>",
		  cmd_ieee802154_associate),
	SHELL_CMD(disassociate,	NULL,
		  "Disassociate from network",
		  cmd_ieee802154_disassociate),
	SHELL_CMD(get_chan, NULL,
		  "Get currently used channel",
		  cmd_ieee802154_get_chan),
	SHELL_CMD(get_ext_addr,	NULL,
		  "Get currently used extended address",
		  cmd_ieee802154_get_ext_addr),
	SHELL_CMD(get_pan_id, NULL,
		  "Get currently used PAN id",
		  cmd_ieee802154_get_pan_id),
	SHELL_CMD(get_short_addr, NULL,
		  "Get currently used short address",
		  cmd_ieee802154_get_short_addr),
	SHELL_CMD(get_tx_power,	NULL,
		  "Get currently used TX power",
		  cmd_ieee802154_get_tx_power),
	SHELL_CMD(scan,	NULL,
		  "<passive|active> <channels set n[:m:...]:x|all>"
		  " <per-channel duration in ms>",
		  cmd_ieee802154_scan),
	SHELL_CMD(set_chan, NULL,
		  "<channel> Set used channel",
		  cmd_ieee802154_set_chan),
	SHELL_CMD(set_ext_addr,	NULL,
		  "<long/extended address (EUI-64)> Set extended address",
		  cmd_ieee802154_set_ext_addr),
	SHELL_CMD(set_pan_id, NULL,
		  "<pan_id> Set used PAN id",
		  cmd_ieee802154_set_pan_id),
	SHELL_CMD(set_short_addr, NULL,
		  "<short address> Set short address",
		  cmd_ieee802154_set_short_addr),
	SHELL_CMD(set_tx_power,	NULL,
		  "<-18/-7/-4/-2/0/1/2/3/5> Set TX power",
		  cmd_ieee802154_set_tx_power),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(ieee802154, &ieee802154_commands, "IEEE 802.15.4 commands",
		   NULL);
