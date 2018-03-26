/*
 * Copyright (c) 2018, Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Console shell to test SimpleLink WiFi control plane APIs.
 * Shell inspired by the Zephyr shell and irc-bot.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <zephyr.h>
#include <misc/printk.h>
#include <console.h>
#include <stdint.h>

#include <errno.h>
#include <net/wifi_mgmt.h>

#define ARGC_MAX 10
#define COMMAND_MAX_LEN 50
#define APP_CONNECT_TIMEOUT 20000  /* 20 Seconds */

/* Needed to keep track of event handler/callback state machine: */
enum wlan_operation {
	WLAN_OPERATION_NONE = 0,
	WLAN_OPERATION_CONNECTING,
	WLAN_OPERATION_DISCONNECTING,
};

static enum wlan_operation wlan_op;

static struct net_mgmt_event_callback l2_mgmt_cb;
static struct net_mgmt_event_callback l3_mgmt_cb;
static struct k_sem connect_sem;
static struct k_sem disconnect_sem;
static struct k_sem ip4acquire_sem;
static int wlan_status;

static void handler(struct net_mgmt_event_callback *cb,
		    u32_t mgmt_event,
		    struct net_if *iface)
{
	struct wifi_scan_result *sr;
	static char buf[NET_IPV4_ADDR_LEN];

	switch (mgmt_event) {
	case NET_EVENT_WIFI_SCAN_RESULT:
		/* Print scan result, per index: */
		sr = (struct wifi_scan_result *)cb->info;
		if (sr->idx == 0) {
			printk("%-3s | %-32s | %-4s | %-4s | %-5s\n",
			       "Ind", "SSID", "Ch", "RSSI", "Sec");
		}
		if (sr->idx != WIFI_END_SCAN_RESULTS) {
			printk("%-3d | %-32s | %-4u | %-4d | ",
			       sr->idx, sr->ssid, sr->channel, sr->rssi);
			printk("%-5s\n",
			       (sr->security == WIFI_SECURITY_TYPE_PSK ?
				"PSK" : "Open"));
		} else {
			printk("%10s\n", "----------");
		}
		break;

	case NET_EVENT_WIFI_CONNECT_RESULT:
		wlan_status = ((struct wifi_status *)cb->info)->status;
		if (wlan_op == WLAN_OPERATION_CONNECTING) {
			k_sem_give(&connect_sem);
		}
		break;

	case NET_EVENT_WIFI_DISCONNECT_RESULT:
		wlan_status = ((struct wifi_status *)cb->info)->status;
		/*
		 * Simplelink NWP can disconnect during a connect, during a
		 * user requested disconnect, or asynchronously due to network
		 * or peer error.
		 */
		if (wlan_status && wlan_op == WLAN_OPERATION_CONNECTING) {
			k_sem_give(&connect_sem);
		} else if (wlan_op == WLAN_OPERATION_DISCONNECTING) {
			k_sem_give(&disconnect_sem);
		} else {
			printk("Disconnected: %d\n", wlan_status);
		}
		break;

	case NET_EVENT_IPV4_ADDR_ADD:
		/* IPV4 address does not come with cb->info!
		 * Must pull from iface itself.
		 */
		printk("IPv4 address: %s\n",
		       net_addr_ntop(AF_INET,
				     &iface->ipv4.unicast[0].address.in_addr,
				     (char *)buf, sizeof(buf)));
		printk("IPv4 gateway: %s\n",
		       net_addr_ntop(AF_INET, &iface->ipv4.gw, (char *)buf,
				     sizeof(buf)));
		break;

	default:
		printk("Unrecognzed mgmt event received: 0x%x\n", mgmt_event);
	}
}

static int parse_connect_args(int argc, char *argv[],
			      struct wifi_connect_req_params *params)
{
	int err = 0;
	u8_t len;

	/* Parse SSID, and length. */
	len = strlen(argv[1]);
	if (len <= WIFI_SSID_MAX_LEN) {
		params->ssid = argv[1];
		params->ssid_length = len;
	} else {
		err = -EINVAL;
		printk("Invalid SSID length: > %d\n", WIFI_SSID_MAX_LEN);
		goto exit;
	}

	/* Parameter not used, set to any: */
	params->channel = WIFI_CHANNEL_ANY;

	/* Check for optional password: */
	if (argc == 3) {
		len = strlen(argv[2]);
		if (len <= WIFI_PSK_MAX_LEN) {
			params->psk = argv[2];
			params->psk_length = len;
			params->security = WIFI_SECURITY_TYPE_PSK;
		} else {
			err = -EINVAL;
			printk("Invalid Password length: > %d\n",
			       WIFI_PSK_MAX_LEN);
			goto exit;
		}
	} else {
		params->security = WIFI_SECURITY_TYPE_NONE;
	}

exit:
	return err;
}

static void on_cmd_disconnect(size_t argc, char *argv[])
{
	int err = 0;
	struct net_if *iface = net_if_get_default();

	wlan_op = WLAN_OPERATION_DISCONNECTING;
	err = net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface, NULL, 0);

	if (err) {
		printk("Disconnect failed: %d\n", err);
	} else {
		/* Wait for WLAN disconnect: */
		if (k_sem_take(&disconnect_sem, APP_CONNECT_TIMEOUT)) {
			printk("Disconnect Timeout: %d ms\n",
			       APP_CONNECT_TIMEOUT);
		} else if (wlan_status) {
			printk("Disconnect failed: %d\n", wlan_status);
		} else {
			printk("Disconnect succeeded\n");
		}
	}
	wlan_op = WLAN_OPERATION_NONE;
}

static void on_cmd_connect(size_t argc, char **argv)
{
	int err = 0;
	struct wifi_connect_req_params params;
	struct net_if *iface = net_if_get_default();

	if (argc < 2) {
		err = -EINVAL;
		return;
	}

	err = parse_connect_args(argc, argv, &params);
	if (err) {
		return;
	}

	wlan_op = WLAN_OPERATION_CONNECTING;
	err = net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, (void *)&params,
		       sizeof(params));
	if (err) {
		printk("Connect failed: %d\n", err);
	} else {
		/* Wait for WLAN connect: */
		if (k_sem_take(&connect_sem, APP_CONNECT_TIMEOUT)) {
			printk("Connect Timeout: %d ms\n", APP_CONNECT_TIMEOUT);
		} else if (wlan_status) {
			/* A disconnect event occurred while connecting;
			 * must explicitly disconnect to prevent further
			 * disconnect events.
			 */
			printk("Connect failed: %d.\n", wlan_status);
			(void)net_mgmt(NET_REQUEST_WIFI_DISCONNECT, iface,
				       NULL, 0);
		} else {
			printk("Connect succeeded\n");
		}
	}
	wlan_op = WLAN_OPERATION_NONE;
}

static void on_cmd_scan(size_t argc, char **argv)
{
	int err = 0;
	struct net_if *iface = net_if_get_default();

	err = net_mgmt(NET_REQUEST_WIFI_SCAN, iface, NULL, 0);

	/* Results printed one entry per callback, via the l2_handler() */
}

static size_t line2argv(char *str, char *argv[], size_t size)
{
	size_t argc = 0;

	if (!strlen(str)) {
		return 0;
	}

	while (*str && *str == ' ') {
		str++;
	}

	if (!*str) {
		return 0;
	}

	argv[argc++] = str;

	while ((str = strchr(str, ' '))) {
		*str++ = '\0';

		while (*str && *str == ' ') {
			str++;
		}

		if (!*str) {
			break;
		}

		argv[argc++] = str;

		if (argc == size) {
			printk("Too many parameters (max %zu)\n", size - 1);
			return 0;
		}
	}

	/* keep it POSIX style where argv[argc] is required to be NULL */
	argv[argc] = NULL;

	return argc;
}

#define CMD(c) { \
	.cmd = #c, \
	.cmd_len = sizeof(#c) - 1, \
	.func = on_cmd_ ## c \
}

static const struct {
	const char *cmd;
	size_t cmd_len;
	void (*func)(size_t argc, char **argv);
} commands[] = {
	CMD(connect),
	CMD(disconnect),
	CMD(scan),
};

void main(void)
{
	int done = false;
	char *cmdline;
	int i;
	char *argv[ARGC_MAX + 1];
	size_t argc;

	wlan_op = WLAN_OPERATION_NONE;
	k_sem_init(&connect_sem,    0, 1);
	k_sem_init(&disconnect_sem, 0, 1);
	k_sem_init(&ip4acquire_sem, 0, 1);

	/* Register our handler and the net_mgmt events for which to listen.
	 * Note: notifications are done on a layer basis, so we are forced
	 * to create two mgmt_cb objects to deal with both L2 and L3 events.
	 */
	/* WiFi events are on L2: */
	net_mgmt_init_event_callback(&l2_mgmt_cb, handler,
				     (NET_EVENT_WIFI_SCAN_RESULT |
				      NET_EVENT_WIFI_CONNECT_RESULT |
				      NET_EVENT_WIFI_DISCONNECT_RESULT));
	net_mgmt_add_event_callback(&l2_mgmt_cb);

	/* IPV4 events are on L3: */
	net_mgmt_init_event_callback(&l3_mgmt_cb, handler,
				     NET_EVENT_IPV4_ADDR_ADD);
	net_mgmt_add_event_callback(&l3_mgmt_cb);

	console_getline_init();

	while (!done) {
		printk("cc3220sf wifi> ");
		cmdline = console_getline();
		argc = line2argv(cmdline, argv, ARRAY_SIZE(argv));
		if (!argc) {
			continue;
		}

		for (i = 0; i < ARRAY_SIZE(commands); i++) {
			if (!strncmp(argv[0], commands[i].cmd,
				     commands[i].cmd_len)) {
				printk("Executing: %s\n", (char *)cmdline);
				commands[i].func(argc, argv);
			}
		}
	}
}
