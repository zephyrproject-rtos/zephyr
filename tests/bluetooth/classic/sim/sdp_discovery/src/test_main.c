/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

#include "nsi_host_trampolines.h"
#include "nsi_errno.h"
#include "nsi_main.h"
#include "soc.h"
#include "cmdline.h" /* native_sim command line options header */

#define LOG_MODULE_NAME test_sdp_discovery_main
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_SDP_DISCOVERY_LOG_LEVEL);

extern bt_addr_t peer_addr;

static void cmd_peer_bd_address_found(char *argv, int offset)
{
	char *addr_str = &argv[offset];
	int err;

	err = bt_addr_from_str(addr_str, &peer_addr);
	if (err != 0) {
		LOG_ERR("Failed to parse peer Bluetooth address: %s (err %d)", addr_str, err);
		nsi_exit(err);
	}
}

static void sdp_discovery_args(void)
{
	static struct args_struct_t args[] = {
		{
			.is_mandatory = true,
			.option = "peer_bd_address",
			.name = "XX:XX:XX:XX:XX:XX",
			.type = 's',
			.call_when_found = cmd_peer_bd_address_found,
			.descript = "Bluetooth peer device address for SDP discovery test",
		},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(args);
}

NATIVE_TASK(sdp_discovery_args, PRE_BOOT_1, 20);
