/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief IEEE 802.15.4 shell module
 */

#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <misc/printk.h>

#include <net/net_if.h>
#include <net/ieee802154.h>

#include "ieee802154_mgmt.h"
#include "ieee802154_frame.h"

#define IEEE802154_SHELL_MODULE "ieee15_4"

struct ieee802154_req_params params;
static struct net_mgmt_event_callback scan_cb;

static int shell_cmd_ack(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	if (!strcmp(argv[1], "set") || !strcmp(argv[1], "1")) {
		net_mgmt(NET_REQUEST_IEEE802154_SET_ACK, iface, NULL, 0);
		printk("ACK flag set on outgoing packets\n");

		return 0;
	}

	if (!strcmp(argv[1], "unset") || !strcmp(argv[1], "0")) {
		net_mgmt(NET_REQUEST_IEEE802154_SET_ACK, iface, NULL, 0);
		printk("ACK flag unset on outgoing packets\n");

		return 0;
	}

	return -EINVAL;
}

static inline void parse_extended_address(char *addr, uint8_t *ext_addr)
{
	char *p, *n;
	int i = 0;

	p = addr;

	do {
		n = strchr(p, ':');
		if (n) {
			*n = '\0';
		}

		ext_addr[i] = strtol(p, NULL, 16);
		p = n ? n + 1 : n;
	} while (n);
}

static int shell_cmd_associate(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();

	params.pan_id = atoi(argv[1]);

	if (strlen(argv[2]) == 23) {
		parse_extended_address(argv[2], params.addr);
		params.len = IEEE802154_EXT_ADDR_LENGTH;
	} else {
		params.short_addr = (uint16_t) atoi(argv[2]);
		params.len = IEEE802154_SHORT_ADDR_LENGTH;
	}

	if (net_mgmt(NET_REQUEST_IEEE802154_ASSOCIATE, iface,
		     &params, sizeof(struct ieee802154_req_params))) {
		printk("Could not associate to %s on PAN ID %u",
		       argv[2], params.pan_id);
	} else {
		printk("Associated to PAN ID %u", params.pan_id);
	}

	return 0;
}

static int shell_cmd_disassociate(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	int ret;

	ret = net_mgmt(NET_REQUEST_IEEE802154_DISASSOCIATE, iface, NULL, 0);
	if (ret == -EALREADY) {
		printk("Interface is not associated\n");
	} else if (ret) {
		printk("Could not disassociate? (status: %i)\n", ret);
	} else {
		printk("Interface is now disassociated\n");
	}

	return 0;
}

static inline uint32_t parse_channel_set(char *str_set)
{
	uint32_t channel_set = 0;
	char *p, *n;

	p = str_set;

	do {
		uint32_t chan;

		n = strchr(p, ':');
		if (n) {
			*n = '\0';
		}

		chan = atoi(p);
		channel_set |= BIT(chan - 1);
		p = n ? n + 1 : n;
	} while (n);

	return channel_set;
}

static inline void print_coordinator_address(void)
{
	if (params.len == IEEE802154_EXT_ADDR_LENGTH) {
		int i;

		printk("(extended) ");

		for (i = 0; i < IEEE802154_EXT_ADDR_LENGTH; i++) {
			printk("%02X", params.addr[i]);

			if (i < (IEEE802154_EXT_ADDR_LENGTH - 1)) {
				printk(":");
			}
		}
	} else {
		printk("(short) %u", params.short_addr);
	}
}

static void scan_result_cb(struct net_mgmt_event_callback *cb,
			   uint32_t mgmt_event, struct net_if *iface)
{
	printk("\nChannel: %u\tPAN ID: %u\tCoordinator Address: ",
	       params.channel, params.pan_id);
	print_coordinator_address();
	printk("LQI: %u\n", params.lqi);
}

static int shell_cmd_scan(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint32_t scan_type;
	int ret;

	memset(&params, 0, sizeof(struct ieee802154_req_params));

	net_mgmt_init_event_callback(&scan_cb, scan_result_cb,
				     NET_EVENT_IEEE802154_SCAN_RESULT);

	if (!strcmp(argv[1], "active")) {
		scan_type = NET_REQUEST_IEEE802154_ACTIVE_SCAN;
	} else if (!strcmp(argv[1], "passive")) {
		scan_type = NET_REQUEST_IEEE802154_PASSIVE_SCAN;
	} else {
		return -EINVAL;
	}

	if (!strcmp(argv[2], "all")) {
		params.channel_set = IEEE802154_ALL_CHANNELS;
	} else {
		params.channel_set = parse_channel_set(argv[2]);
	}

	if (!params.channel_set) {
		return -EINVAL;
	}

	params.duration = atoi(argv[3]);

	printk("%s Scanning (channel set: 0x%08x, duration %u ms)...",
	       scan_type == NET_REQUEST_IEEE802154_ACTIVE_SCAN ?
	       "Active" : "Passive", params.channel_set, params.duration);

	if (scan_type == NET_REQUEST_IEEE802154_ACTIVE_SCAN) {
		ret = net_mgmt(NET_REQUEST_IEEE802154_ACTIVE_SCAN, iface,
			       &params, sizeof(struct ieee802154_req_params));
	} else {
		ret = net_mgmt(NET_REQUEST_IEEE802154_PASSIVE_SCAN, iface,
			       &params, sizeof(struct ieee802154_req_params));
	}

	if (ret) {
		printk("Could not raise a scan (status: %i)\n", ret);
	} else {
		printk("Done\n");
	}

	return 0;
}

static int shell_cmd_set_chan(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint16_t channel = (uint16_t) atoi(argv[1]);

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_CHAN, iface,
		     &channel, sizeof(uint16_t))) {
		printk("Could not set channel %u\n", channel);
	} else {
		printk("Channel %u set\n", channel);
	}

	return 0;
}

static int shell_cmd_get_chan(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint16_t channel;

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_CHAN, iface,
		     &channel, sizeof(uint16_t))) {
		printk("Could not get channel\n");
	} else {
		printk("Channel %u\n", channel);
	}

	return 0;
}

static int shell_cmd_set_pan_id(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint16_t pan_id = (uint16_t) atoi(argv[1]);

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_PAN_ID, iface,
		     &pan_id, sizeof(uint16_t))) {
		printk("Could not set PAN ID %u\n", pan_id);
	} else {
		printk("PAN ID %u set\n", pan_id);
	}

	return 0;
}

static int shell_cmd_get_pan_id(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint16_t pan_id;

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_PAN_ID, iface,
		     &pan_id, sizeof(uint16_t))) {
		printk("Could not get PAN ID\n");
	} else {
		printk("PAN ID %u\n", pan_id);
	}

	return 0;
}

static int shell_cmd_set_ext_addr(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint8_t addr[IEEE802154_EXT_ADDR_LENGTH];

	if (strlen(argv[2]) != 23) {
		printk("23 characters needed");
		return 0;
	}

	parse_extended_address(argv[2], addr);

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_EXT_ADDR, iface,
		     addr, IEEE802154_EXT_ADDR_LENGTH)) {
		printk("Could not set extended address\n");
	} else {
		printk("Extended address %s set\n", argv[2]);
	}

	return 0;
}

static int shell_cmd_get_ext_addr(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint8_t addr[IEEE802154_EXT_ADDR_LENGTH];

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_EXT_ADDR, iface,
		     addr, IEEE802154_EXT_ADDR_LENGTH)) {
		printk("Could not get extended address\n");
	} else {
		int i;

		printk("Extended address: ");
		for (i = 0; i < IEEE802154_EXT_ADDR_LENGTH; i++) {
			printk("%02X", addr[i]);

			if (i < (IEEE802154_EXT_ADDR_LENGTH - 1)) {
				printk(":");
			}
		}
	}

	return 0;
}

static int shell_cmd_set_short_addr(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint16_t short_addr = (uint16_t) atoi(argv[1]);

	if (net_mgmt(NET_REQUEST_IEEE802154_SET_SHORT_ADDR, iface,
		     &short_addr, sizeof(uint16_t))) {
		printk("Could not set short address %u\n", short_addr);
	} else {
		printk("Short address %u set\n", short_addr);
	}

	return 0;
}

static int shell_cmd_get_short_addr(int argc, char *argv[])
{
	struct net_if *iface = net_if_get_default();
	uint16_t short_addr;

	if (net_mgmt(NET_REQUEST_IEEE802154_GET_SHORT_ADDR, iface,
		     &short_addr, sizeof(uint16_t))) {
		printk("Could not get short address\n");
	} else {
		printk("Short address %u\n", short_addr);
	}

	return 0;
}

static struct shell_cmd ieee802154_commands[] = {
	{ "ack",		shell_cmd_ack,
	  "<set/1 | unset/0>" },
	{ "associate",		shell_cmd_associate,
	  "<pan_id> <PAN coordinator short or long address>" },
	{ "disassociate",	shell_cmd_disassociate,
	  NULL },
	{ "scan",		shell_cmd_scan,
	  "<passive|active> <channels set n[:m:...]:x|all>"
	  " <per-channel duration in ms>" },
	{ "set_chan",		shell_cmd_set_chan,
	  "<channel>" },
	{ "get_chan",		shell_cmd_get_chan,
	  NULL },
	{ "set_pan_id",		shell_cmd_set_pan_id,
	  "<pan_id>" },
	{ "get_pan_id",		shell_cmd_get_pan_id,
	  NULL },
	{ "set_ext_addr",	shell_cmd_set_ext_addr,
	  "<long/extended address>" },
	{ "get_ext_addr",	shell_cmd_get_ext_addr,
	  NULL },
	{ "set_short_addr",	shell_cmd_set_short_addr,
	  "<short address>" },
	{ "get_short_addr",	shell_cmd_get_short_addr,
	  NULL },
	{ NULL, NULL, NULL },
};

void ieee802154_shell_init(void)
{
	SHELL_REGISTER(IEEE802154_SHELL_MODULE, ieee802154_commands);
}
