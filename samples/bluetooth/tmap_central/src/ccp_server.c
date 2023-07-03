/** @file
 *  @brief Bluetooth Call Control Profile (CCP) Server role.
 *
 *  Copyright 2023 NXP
 *
 *  SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <stdlib.h>
#include <stdio.h>

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/audio/tbs.h>

#define URI_LIST_LEN	2

static const char *uri_list[URI_LIST_LEN] = {"skype", "tel"};

static bool tbs_originate_call_cb(struct bt_conn *conn, uint8_t call_index,
				  const char *caller_id)
{
	printk("CCP: Placing call to remote with id %u to %s\n",
	       call_index, caller_id);
	return true;
}

static void tbs_terminate_call_cb(struct bt_conn *conn, uint8_t call_index, uint8_t reason)
{
	printk("CCP: Call terminated for id %u with reason %u\n",
	       call_index, reason);
}

static struct bt_tbs_cb tbs_cbs = {
	.originate_call = tbs_originate_call_cb,
	.terminate_call = tbs_terminate_call_cb,
	.hold_call = NULL,
	.accept_call = NULL,
	.retrieve_call = NULL,
	.join_calls = NULL,
	.authorize = NULL,
};

int ccp_server_init(void)
{
	int err;

	bt_tbs_register_cb(&tbs_cbs);

	err = bt_tbs_set_uri_scheme_list(0, (const char **)&uri_list, URI_LIST_LEN);

	return err;
}
