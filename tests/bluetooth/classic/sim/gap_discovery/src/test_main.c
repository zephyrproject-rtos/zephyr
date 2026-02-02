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
#include "soc.h"
#include "cmdline.h" /* native_sim command line options header */

#define LOG_MODULE_NAME test_gap_discovery_main
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_TEST_GAP_DISCOVERY_LOG_LEVEL);

extern bt_addr_t peer_addr;

static void cmd_peer_bd_address_found(char *argv, int offset)
{
	char *addr_str = &argv[offset];
	int err;

	err = bt_addr_from_str(addr_str, &peer_addr);
	if (err != 0) {
		LOG_ERR("Failed to parse peer Bluetooth address: %s (err %d)", addr_str, err);
	}
}

static void gap_discovery_args(void)
{
	static struct args_struct_t args[] = {
		{
			.manual = false,
			.is_mandatory = true,
			.is_switch = false,
			.option = "peer_bd_address",
			.name = "XX:XX:XX:XX:XX:XX",
			.type = 's',
			.dest = NULL,
			.call_when_found = cmd_peer_bd_address_found,
			.descript = "Bluetooth peer device address for GAP discovery test",
		},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(args);
}

NATIVE_TASK(gap_discovery_args, PRE_BOOT_1, 20);
