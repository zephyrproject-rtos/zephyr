/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/classic/avrcp.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

#include "avctp_internal.h"

static void *setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "Dummy shell did not start");

	/* Initialize the transport without starting an HCI controller. */
	bt_avctp_init();
	return NULL;
}

ZTEST(avrcp, test_register_control_callbacks)
{
	if (IS_ENABLED(CONFIG_BT_AVRCP_CONTROLLER)) {
		static const struct bt_avrcp_ct_cb ct_cb;

		zassert_ok(bt_avrcp_ct_register_cb(&ct_cb));
	}
	if (IS_ENABLED(CONFIG_BT_AVRCP_TARGET)) {
		static const struct bt_avrcp_tg_cb tg_cb;

		zassert_ok(bt_avrcp_tg_register_cb(&tg_cb));
	}
}

static void check_commands(const char *command, const char *control, const char *const *browsing,
			   size_t count)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	const char *output;
	size_t size;
	int ret;

	shell_backend_dummy_clear_output(sh);
	ret = shell_execute_cmd(sh, command);

	zassert_equal(ret, 1, "%s returned %d", command, ret);
	output = shell_backend_dummy_get_output(sh, &size);
	zassert_not_null(strstr(output, control), "Control command %s missing", control);
	for (size_t i = 0; i < count; i++) {
		zassert_equal(strstr(output, browsing[i]) != NULL,
			      IS_ENABLED(CONFIG_BT_AVRCP_BROWSING), "Unexpected availability of %s",
			      browsing[i]);
	}
}

ZTEST(avrcp, test_shell_browsing_connection_commands)
{
	static const char *const commands[] = {"browsing_connect", "browsing_disconnect"};

	check_commands("avrcp", "connect", commands, ARRAY_SIZE(commands));
}

ZTEST(avrcp, test_shell_controller_browsing_commands)
{
	static const char *const commands[] = {
		"set_browsed_player", "get_folder_items",          "change_path",
		"get_item_attrs",     "get_total_number_of_items", "search",
	};

	check_commands("avrcp ct", "get_caps", commands, ARRAY_SIZE(commands));
}

ZTEST(avrcp, test_shell_target_browsing_commands)
{
	static const char *const commands[] = {
		"send_browsed_player_rsp",
		"send_get_folder_items_rsp",
		"send_change_path_rsp",
		"send_get_item_attrs_rsp",
		"send_get_total_number_of_items_rsp",
		"send_search_rsp",
		"send_browsing_general_reject",
	};

	check_commands("avrcp tg", "send_unit_rsp", commands, ARRAY_SIZE(commands));
}

ZTEST_SUITE(avrcp, NULL, setup, NULL, NULL, NULL);
